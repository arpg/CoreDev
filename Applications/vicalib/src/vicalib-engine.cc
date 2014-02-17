// Copyright (c) George Washington University, all rights reserved.  See the
// accompanying LICENSE file for more information.

#include <time.h>
#include <thread>
#include <functional>

#include <calibu/cam/CameraXml.h>
#include <calibu/cam/CameraModelT.h>
#include <calibu/target/RandomGrid.h>
#include <HAL/Camera/CameraDevice.h>
#include <glog/logging.h>
#include <PbMsgs/Matrix.h>

#include <visual-inertial-calibration/eigen-alignment.h>
#include <visual-inertial-calibration/grid-definitions.h>
#include <visual-inertial-calibration/vicalib-task.h>
#include <visual-inertial-calibration/vicalib-engine.h>
#include <visual-inertial-calibration/vicalibrator.h>
#include <visual-inertial-calibration/calibration-stats.h>

static const int64_t kCalibrateAllPossibleFrames = -1;

#ifdef BUILD_GUI
DEFINE_bool(paused, false, "Start video paused");
#endif

DEFINE_bool(calibrate_intrinsics, true,
            "Calibrate the camera intrinsics as well as the extrinsics.");
DEFINE_string(device_serial, "-1", "Serial number of device.");
DEFINE_bool(exit_vicalib_on_finish, false,
            "Exit vicalib when the optimization finishes.");
DEFINE_int32(frame_skip, 0, "Number of frames to skip between constraints.");
DEFINE_int32(grid_height, 10, "Height of grid in circles.");
DEFINE_int32(grid_width, 19, "Width of grid in circles.");
DEFINE_double(grid_spacing, 0.01355/*0.254 / (19 - 1) meters */,
              "Distance between circles on grid.");
DEFINE_int32(grid_seed, 71, "seed used to generate the grid.");
DEFINE_bool(has_initial_guess, false,
            "Whether or not the given calibration file has a valid guess.");
DEFINE_int32(grid_preset, visual_inertial_calibration::GridPresetGWUSmall,
             "Which grid preset to use. "
             "Must be a visual_inertial_calibration::GridPreset.");
DEFINE_double(max_reprojection_error, 0.15,  // pixels
              "Maximum allowed reprojection error.");
DEFINE_int64(num_vicalib_frames, kCalibrateAllPossibleFrames,
             "Number of frames to process before calibration begins.");
DEFINE_string(output, "cameras.xml",
              "Output XML file to write camera models to.");
DEFINE_string(output_log_file, "vicalibrator.log",
              "Calibration result output log file.");
DEFINE_bool(scaled_ir_depth_cal, false,
            "Produce ir and depth calibration by rescaling RGB.");
DEFINE_double(static_accel_threshold, 0.08, "Threshold for acceleration at "
              "which we consider the device static.");
DEFINE_double(static_gyro_threshold, 0.04, "Threshold for angular velocity at "
              "which we consider the device static.");
DEFINE_int32(static_threshold_preset,
             visual_inertial_calibration::StaticThresholdManual,
             "Which grid preset to use. "
             "Must be a visual_inertial_calibration::StaticThresholdPreset.");
DEFINE_bool(use_grid_preset, false, "Use one of the predefined grid sizes.");
DEFINE_bool(use_only_when_static, true, "Only use frames where the device is "
            "stationary.");
DEFINE_bool(use_static_threshold_preset, false,
            "Use one of the predefined static thresholds.");
DEFINE_string(cam, "", "Camera URI");
DEFINE_string(imu, "", "IMU URI (if available)");
DEFINE_string(models, "",
              "Comma-separated list of camera models to calibrate. "
              "Must be in channel order.");

namespace visual_inertial_calibration {

static const timespec sleep_length = {0, 30000000};  // 30 ms

using std::placeholders::_1;

VicalibEngine::VicalibEngine(const std::function<void()>& stop_sensors_callback,
                             const std::function<void(
                                 const std::shared_ptr<CalibrationStats>&)>&
                             update_stats_callback) :
    frames_skipped_(0),
    stop_sensors_callback_(stop_sensors_callback),
    update_stats_callback_(update_stats_callback),
    sensors_finished_(false),
    gyro_filter_(10, FLAGS_static_gyro_threshold),
    accel_filter_(10, FLAGS_static_accel_threshold) {
  CHECK(!FLAGS_cam.empty());
  try {
    camera_.reset(new hal::Camera(hal::Uri(FLAGS_cam)));
  } catch (...) {
    LOG(ERROR) << "Could not create camera from URI: " << FLAGS_cam;
  }
  stats_.reset(new CalibrationStats(camera_->NumChannels()));

  if (!FLAGS_imu.empty()) {
    try {
      imu_.reset(new hal::IMU(FLAGS_imu));
      imu_->RegisterIMUDataCallback(
          std::bind(&VicalibEngine::ImuHandler, this, _1));
    } catch (...) {
      LOG(ERROR) << "Could not create IMU from URI: " << FLAGS_imu;
    }
  }
}

VicalibEngine::~VicalibEngine() {
  if (vicalib_) {
    vicalib_->Finish(FLAGS_output);
  }
}

std::shared_ptr<VicalibTask> VicalibEngine::InitTask() {
  std::vector<size_t> widths, heights;
  for (size_t i = 0; i < camera_->NumChannels(); ++i) {
    widths.push_back(camera_->Width(i));
    heights.push_back(camera_->Height(i));
  }

  std::vector<std::string> model_strings;
  std::stringstream ss(FLAGS_models);
  std::string item;
  while (std::getline(ss, item, ',')) {
    model_strings.push_back(item);
  }

  CHECK_EQ(model_strings.size(), camera_->NumChannels())
      << "Must declare a model for every camera channel";

  aligned_vector<CameraAndPose> input_cameras;
  for (size_t i = 0; i < model_strings.size(); ++i) {
    const std::string& type = model_strings[i];
    int w = camera_->Width(i);
    int h = camera_->Height(i);

    if (type == "fov") {
      calibu::CameraModelT<calibu::Fov> starting_cam(w, h);
      starting_cam.Params() << 300, 300, w/2.0, h/2.0, 0.2;
      input_cameras.emplace_back(starting_cam, Sophus::SE3d());
    } else if (type == "poly2") {
      calibu::CameraModelT<calibu::Poly2> starting_cam(w, h);
      starting_cam.Params() << 300, 300, w/2.0, h/2.0, 0.0, 0.0;
      input_cameras.emplace_back(starting_cam, Sophus::SE3d());
    } else if (type == "poly3" || type =="poly") {
      calibu::CameraModelT<calibu::Poly3> starting_cam(w, h);
      starting_cam.Params() << 300, 300, w/2.0, h/2.0, 0.0, 0.0, 0.0;
      input_cameras.emplace_back(starting_cam, Sophus::SE3d());
    } else if (type == "kb4") {
      calibu::CameraModelT<calibu::ProjectionKannalaBrandt> starting_cam(w, h);
      starting_cam.Params() << 300, 300, w/2.0, h/2.0, 0.0, 0.0, 0.0, 0.0;
      input_cameras.emplace_back(starting_cam, Sophus::SE3d());
    }
    input_cameras.back().camera.SetRDF(calibu::RdfRobotics.matrix());
  }

  Vector6d biases(Vector6d::Zero());
  Vector6d scale_factors(Vector6d::Ones());
  double grid_spacing = FLAGS_grid_spacing;

  Eigen::MatrixXi grid;
  if (FLAGS_use_grid_preset) {
    switch (FLAGS_grid_preset) {
      case GridPresetGWUSmall:
        grid = GWUSmallGrid();
        grid_spacing = 0.254 / 18;  // meters
        break;
      case GridPresetGoogleLarge:
        grid = GoogleLargeGrid();
        grid_spacing = 0.03156;  // meters
        break;
      default:
        LOG(FATAL) << "Unknown grid preset " << FLAGS_grid_preset;
        break;
    }
  } else {
    grid = calibu::MakePattern(
        FLAGS_grid_height, FLAGS_grid_width, FLAGS_grid_seed);
  }

  if (FLAGS_use_static_threshold_preset) {
    switch (FLAGS_static_threshold_preset) {
      case StaticThresholdManual:
        FLAGS_static_accel_threshold = 0.09;
        FLAGS_static_gyro_threshold = 0.05;
        break;
      case StaticThresholdStrict:
        FLAGS_static_accel_threshold = 0.05;
        FLAGS_static_gyro_threshold = 0.025;
        break;
      default:
        LOG(FATAL) << "Unknown static threshold preset "
                   << FLAGS_static_threshold_preset;
        break;
    }
  }

  std::vector<double> max_errors(camera_->NumChannels(),
                                 FLAGS_max_reprojection_error);
  std::shared_ptr<VicalibTask> task(
      new VicalibTask(camera_->NumChannels(), widths, heights,
                      grid_spacing, grid,
                      !FLAGS_calibrate_intrinsics,
                      input_cameras, max_errors));

  task->GetCalibrator().SetBiases(biases);
  task->GetCalibrator().SetScaleFactor(scale_factors);

#ifdef BUILD_GUI
  pangolin::RegisterKeyPressCallback(' ', [&]() {
      FLAGS_paused = !FLAGS_paused;
    });
#endif  // BUILD_GUI
  return task;
}

void Draw(const std::shared_ptr<VicalibTask>& task) {
  task->Draw();
#ifdef BUILD_GUI
  pangolin::FinishFrame();
#endif
}

void VicalibEngine::WriteCalibration() {
  vicalib_->GetCalibrator().WriteCameraModels(FLAGS_output);
}

void VicalibEngine::CalibrateAndDrawLoop() {
  if (!vicalib_->IsRunning()) {
    vicalib_->Start(FLAGS_has_initial_guess);
  }

  // Wait for the thread to start
  while (!vicalib_->IsRunning()) {
    usleep(500);
  }

  stats_->status = CalibrationStats::StatusOptimizing;
  bool finished = false;
  while (true) {
    stats_->total_mse = vicalib_->GetMeanSquaredError();
    stats_->reprojection_error = vicalib_->GetCalibrator().GetCameraProjRMSE();
    stats_->num_iterations = vicalib_->GetCalibrator().GetNumIterations();
    update_stats_callback_(std::make_shared<CalibrationStats>(*stats_));

    if (!finished && !vicalib_->IsRunning()) {
      LOG(INFO) << "Finished... " << std::endl;
      stats_->status = vicalib_->IsSuccessful() ?
          CalibrationStats::StatusSuccess : CalibrationStats::StatusFailure;
      vicalib_->Finish(FLAGS_output);
      WriteCalibration();
      finished = true;
      if (FLAGS_exit_vicalib_on_finish) {
        exit(0);
      }
    }
    Draw(vicalib_);

    nanosleep(&sleep_length, NULL);
  }
}

void VicalibEngine::Run() {
  while (CameraLoop() && !vicalib_->IsRunning() && !SeenEnough()) {}
  stop_sensors_callback_();
  CalibrateAndDrawLoop();
}

bool VicalibEngine::CameraLoop() {
  if (!vicalib_) {
    vicalib_ = InitTask();

    if (!vicalib_) {
      LOG(WARNING) << "Vicalib task still NULL. Skipping frame.";
      return false;
    }
  }

#ifdef BUILD_GUI
  while (FLAGS_paused) {
    Draw(vicalib_);
    nanosleep(&sleep_length, NULL);
  }
#endif

  std::shared_ptr<pb::ImageArray> images = pb::ImageArray::Create();
  bool captured = camera_->Capture(*images);
  bool should_use = true;
  if (FLAGS_use_only_when_static) {
    should_use = accel_filter_.IsStable() && gyro_filter_.IsStable();
  }

  if (frames_skipped_ >= FLAGS_frame_skip) {
    if (captured && should_use) {
      frames_skipped_ = 0;
      std::vector<bool> valid_frames = vicalib_->AddSuperFrame(images);
      for (size_t ii = 0; ii < valid_frames.size() ; ++ii) {
        if (valid_frames[ii]) {
          ++stats_->num_frames_processed[ii];
        }
      }
    }
  } else {
    ++frames_skipped_;
  }

  stats_->status = CalibrationStats::StatusCapturing;
  update_stats_callback_(std::make_shared<CalibrationStats>(*stats_));
  Draw(vicalib_);
  return captured;
}

void VicalibEngine::ImuHandler(const pb::ImuMsg& imu) {
  CHECK(imu.has_accel());
  CHECK(imu.has_gyro());

  Eigen::VectorXd gyro, accel;
  ReadVector(imu.accel(), &accel);
  ReadVector(imu.gyro(), &gyro);
  accel_filter_.Add(accel);
  gyro_filter_.Add(gyro);

  vicalib_->AddIMU(imu);
}

bool VicalibEngine::SeenEnough() const {
  return ((FLAGS_num_vicalib_frames != kCalibrateAllPossibleFrames &&
           *std::max_element(stats_->num_frames_processed.begin(),
                             stats_->num_frames_processed.end()) >=
           FLAGS_num_vicalib_frames) ||
          sensors_finished_);
}
}  // namespace visual_inertial_calibration
