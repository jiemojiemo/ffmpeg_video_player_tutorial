cmake_minimum_required(VERSION 3.10)

project(ffmpeg_video_player)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

if (ENABLE_ASAN)
    message(STATUS "build with ASAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

add_subdirectory(utils)

add_subdirectory(tutorial01)


add_subdirectory(tests)