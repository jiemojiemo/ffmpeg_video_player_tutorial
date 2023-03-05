cmake_minimum_required(VERSION 3.18.1)
project("tutorial01")

set(proj_root_dir "../../../../../..")
set(proj_3rd_dir "${proj_root_dir}/3rdparty")

add_subdirectory(${proj_3rd_dir} "${CMAKE_BINARY_DIR}/3rdparty")

add_library(
        tutorial01
        SHARED
        native-lib.cpp)

find_library(log-lib log)

target_link_libraries(
        tutorial01
        ffmpeg_libs
        ${log-lib})