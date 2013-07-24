project( CameraViewer )
cmake_minimum_required(VERSION 2.8)

find_package( Pangolin REQUIRED )
find_package( SceneGraph REQUIRED )
find_package( RPG REQUIRED )

include_directories(
    ${Pangolin_INCLUDE_DIRS}
    ${SceneGraph_INCLUDE_DIRS}
    ${RPG_INCLUDE_DIRS}
)

link_libraries(
    ${Pangolin_LIBRARIES}
    ${SceneGraph_LIBRARIES}
    ${RPG_LIBRARIES}
)

add_executable( CameraViewer viewer.cpp )

