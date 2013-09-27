project("RobotPerceptionCollection")
cmake_minimum_required(VERSION 2.8.10)

# Add auxiliary libraries
add_subdirectory(GLConsole)

option(BUILD_TESTS "Build tests." OFF)
add_subdirectory(Sophus)

# Add GUI Libraries
add_subdirectory(Pangolin)
add_subdirectory(SceneGraph)

# Add calibration utils
add_subdirectory(Calibu)

# Add Camera / Device libraries
add_subdirectory(HAL)

# Add Bundle Adjuster
option(BUILD_BA "Build BA." ON)
if(BUILD_BA)
add_subdirectory(ba)
endif()


# Build Example if requested
set( BUILD_COREDEV_EXAMPLES false CACHE BOOL force )
if(BUILD_COREDEV_EXAMPLES)
  add_subdirectory(Example)
endif()
