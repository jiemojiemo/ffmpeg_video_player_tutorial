cmake_minimum_required(VERSION 3.10)

project(ffmpeg_video_player)

set(CMAKE_CXX_STANDARD 17)

find_package(ffmpeg CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)


if (ENABLE_ASAN)
    message(STATUS "build with ASAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

add_subdirectory(3rdparty)
add_subdirectory(src)

add_subdirectory(tutorial00)
add_subdirectory(tutorial01)
add_subdirectory(tutorial02)
add_subdirectory(tutorial03)
add_subdirectory(tutorial04)
add_subdirectory(tutorial05)
add_subdirectory(tutorial07)
add_subdirectory(tutorial08)

add_subdirectory(tests)