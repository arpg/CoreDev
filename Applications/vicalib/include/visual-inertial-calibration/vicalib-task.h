// This file is part of the BA Project.
//
// Copyright (C) 2013 George Washington University,
// Nima Keivan,
// Steven Lovegrove,
// Gabe Sibley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef VISUAL_INERTIAL_CALIBRATION_VICALIB_TASK_H_
#define VISUAL_INERTIAL_CALIBRATION_VICALIB_TASK_H_

#include <string>
#include <vector>
#include <memory>

#include <Eigen/StdVector>
#include <calibu/cam/CameraModelT.h>
#include <calibu/conics/ConicFinder.h>
#include <calibu/image/ImageProcessing.h>
#include <calibu/target/TargetGridDot.h>
#include <PbMsgs/ImageArray.h>
#include <gflags/gflags.h>
#include <sophus/se3.hpp>

#include <visual-inertial-calibration/vicalibrator.h>

#ifdef BUILD_GUI
#include <pangolin/pangolin.h>
#include <visual-inertial-calibration/gl-line-strip.h>
#endif  // BUILD_GUI

namespace visual_inertial_calibration {

struct FrameStats {
  FrameStats() : tested_frames(0), tears(0), stutters(0), jumps(0) { }
  int tested_frames, tears, stutters, jumps;
};

struct VicalibGuiOptions;

class VicalibTask {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  VicalibTask(size_t num_cameras,
              const std::vector<size_t>& width,
              const std::vector<size_t>& height,
              int grid_seed,
              double gspacing, const Eigen::Vector2i& grid_size,
              bool fix_intrinsics,
              const aligned_vector<CameraAndPose>& input_cameras,
              const std::vector<double>& max_reproj_errors);
  VicalibTask(const VicalibTask&) = delete;
  ~VicalibTask();

  std::vector<bool> AddSuperFrame(const std::shared_ptr<pb::ImageArray>& imgs);

  void AddIMU(const pb::ImuMsg& imu);

  void Start(bool has_initial_guess);
  bool IsRunning();
  void Finish(const std::string& output_filename);
  void Draw();
  ViCalibrator& GetCalibrator() { return calibrator_; }

  double GetMeanSquaredError() {
    return calibrator_.MeanSquaredError();
  }

  bool IsSuccessful() const;
  size_t NumStreams() const { return nstreams_; }
  size_t width(size_t i = 0) const { return width_[i]; }
  size_t height(size_t i = 0) const { return height_[i]; }

 protected:
  static void* RunGUI(void* arg);
  void SetupGUI();
  void AddImageMeasurements(const std::vector<bool>& valid_frames);
  int AddFrame(double frame_time);
  bool IsFrameClear();
  void LogValidationStats();
  void Draw2d();
  void Draw3d();

 private:
  std::vector<calibu::ImageProcessing> image_processing_;
  aligned_vector<calibu::ConicFinder> conic_finder_;
  aligned_vector<calibu::TargetGridDot> target_;
  Eigen::Vector2i grid_size_;
  double frame_timestamp_offset_;
  double grid_spacing_;
  uint32_t grid_seed_;
  std::vector<int> calib_cams_;
  std::vector<double> frame_times_;
  double current_frame_time_;
  size_t nstreams_;
  std::vector<size_t> width_;
  std::vector<size_t> height_;
  int calib_frame_;
  std::vector<bool> tracking_good_;
  aligned_vector<Sophus::SE3d> t_cw_;
  FrameStats stats_;
  int num_frames_;
  ViCalibrator calibrator_;
  std::shared_ptr<pb::ImageArray> images_;
  aligned_vector<CameraAndPose> input_cameras_;
  std::vector<double> max_reproj_errors_;

#ifdef HAVE_PANGOLIN
  std::vector<pangolin::GlTexture> textures_;
  pangolin::OpenGlRenderState stacks_;
  std::vector<std::unique_ptr<GLLineStrip> > imu_strips_;
  pangolin::Handler3D handler_;
#endif  // HAVE_PANGOLIN

  std::unique_ptr<VicalibGuiOptions> options_;
};
}  // namespace visual_inertial_calibration
#endif  // VISUAL_INERTIAL_CALIBRATION_VICALIB_TASK_H_
