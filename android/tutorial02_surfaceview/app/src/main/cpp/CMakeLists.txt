cmake_minimum_required(VERSION 3.22.1)

project("usesurfaceview")

set(CMAKE_CXX_STANDARD 17)

add_library(usesurfaceview
        SHARED
        usesurfaceview.cpp)

set(j_video_player_root_dir ${CMAKE_SOURCE_DIR}/../../../../../..)
set(j_video_player_src_dir ${j_video_player_root_dir}/src)
set(j_video_player_3rdparty_dir ${j_video_player_root_dir}/3rdparty)

add_subdirectory(${j_video_player_3rdparty_dir} "${CMAKE_BINARY_DIR}/3rdparty")
add_subdirectory(${j_video_player_src_dir} "${CMAKE_BINARY_DIR}/j_video_player")

target_link_libraries(
        usesurfaceview

        PRIVATE
        j_video_player
        android
        jnigraphics
)