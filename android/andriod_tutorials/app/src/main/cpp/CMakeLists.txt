cmake_minimum_required(VERSION 3.18.1)

project("usesurfaceview")

set(CMAKE_CXX_STANDARD 17)

set(j_video_player_root_dir ${CMAKE_SOURCE_DIR}/../../../../../..)
set(j_video_player_src_dir ${j_video_player_root_dir}/src)
set(j_video_player_3rdparty_dir ${j_video_player_root_dir}/3rdparty)

add_subdirectory(${j_video_player_3rdparty_dir} "${CMAKE_BINARY_DIR}/3rdparty")
add_subdirectory(${j_video_player_src_dir} "${CMAKE_BINARY_DIR}/j_video_player")

add_library(native_video_player
        SHARED
        jni_simple_player.cpp
        jni_tutorial01_hello_ffmpeg.cpp
        jni_tutorial02_usesurfaceview.cpp
        )

target_link_libraries(
        native_video_player

        PRIVATE
        j_video_player
        android
        jnigraphics
        OpenSLES
)