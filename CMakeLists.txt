project("RobotPerceptionCollection")
# CMake version 3.0 onwards is required for correct dependency tree build and proper versioning.
cmake_minimum_required( VERSION 3.0 )

# Add auxiliary libraries
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

option(DISABLE_COREDEV_EXPORTS
  "Should CoreDev subpackages *NOT* be globally exported." OFF)

if(DISABLE_COREDEV_EXPORTS)
  set(SUBPACKAGES Pangolin SceneGraph Calibu HAL PbMsgs Node Sophus CVARS MINIGLOG)
  foreach (package ${SUBPACKAGES})
    set(EXPORT_${package} OFF CACHE BOOL "Export ${package} package")

    # Just hide them from the user to not be overwhelming
    mark_as_advanced(EXPORT_${package})
  endforeach()
endif()

# Add Aux Libraries
add_subdirectory(miniglog)
set(MINIGLOG_DIR ${MINIGLOG_BINARY_DIR})

add_subdirectory(Sophus)
set(Sophus_DIR ${Sophus_BINARY_DIR})

# Add GUI Libraries
add_subdirectory(GLConsole)
set(CVARS_DIR ${CVARS_BINARY_DIR})

add_subdirectory(Pangolin)
set(Pangolin_DIR ${Pangolin_BINARY_DIR}/src)

add_subdirectory(SceneGraph)
set(SceneGraph_DIR ${SceneGraph_BINARY_DIR})

# Add calibration utils
add_subdirectory(Calibu)
set(Calibu_DIR ${Calibu_BINARY_DIR})

add_subdirectory(Node)
set(Node_DIR ${Node_BINARY_DIR})

# Add Camera / Device libraries
add_subdirectory(HAL)
set(HAL_DIR ${HAL_BINARY_DIR})
set(PbMsgs_DIR ${PbMsgs_BINARY_DIR})

# Build Example if requested
set( BUILD_COREDEV_EXAMPLES false CACHE BOOL force )
if(BUILD_COREDEV_EXAMPLES)
  add_subdirectory(Example)
endif()

