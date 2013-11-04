project("RobotPerceptionCollection")
cmake_minimum_required(VERSION 2.8.10)

# Add auxiliary libraries
add_subdirectory(GLConsole)

option(DISABLE_COREDEV_EXPORTS
  "Should CoreDev subpackages *NOT* be globally exported." OFF)

if(DISABLE_COREDEV_EXPORTS)
  set(SUBPACKAGES Pangolin SceneGraph Calibu HAL PbMsgs NODE)
  foreach (package ${SUBPACKAGES})
    set(EXPORT_${package} OFF CACHE BOOL "Export ${package} package" FORCE)
  endforeach()
endif()

option(BUILD_TESTS "Build tests." OFF)
add_subdirectory(Sophus)

# Add GUI Libraries
add_subdirectory(Pangolin)
add_subdirectory(SceneGraph)

# Add calibration utils
add_subdirectory(Calibu)

# Add Camera / Device libraries
add_subdirectory(HAL)

# Build Example if requested
set( BUILD_COREDEV_EXAMPLES false CACHE BOOL force )
if(BUILD_COREDEV_EXAMPLES)
  add_subdirectory(Example)
endif()
