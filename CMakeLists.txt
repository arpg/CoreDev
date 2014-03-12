project("RobotPerceptionCollection")
# CMake version 2.8.10 onwards is required for correct dependency tree build.
cmake_minimum_required(VERSION 2.8.10)

# Add auxiliary libraries
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

option(DISABLE_COREDEV_EXPORTS
  "Should CoreDev subpackages *NOT* be globally exported." OFF)

if(DISABLE_COREDEV_EXPORTS)
  set(SUBPACKAGES Pangolin SceneGraph Calibu HAL PbMsgs NODE Sophus CVars miniglog)
  foreach (package ${SUBPACKAGES})
    set(EXPORT_${package} OFF CACHE BOOL "Export ${package} package" FORCE)
  endforeach()
endif()

option(BUILD_TESTS "Build tests." OFF)

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
set(Calibu_DIR ${Calibu_BINARY_DIR}/src)

# Add Camera / Device libraries
add_subdirectory(HAL)
set(HAL_DIR ${HAL_BINARY_DIR})
set(PbMsgs_DIR ${PbMsgs_BINARY_DIR})

# Build Example if requested
set( BUILD_COREDEV_EXAMPLES false CACHE BOOL force )
if(BUILD_COREDEV_EXAMPLES)
  add_subdirectory(Example)
endif()

add_subdirectory(Applications)
