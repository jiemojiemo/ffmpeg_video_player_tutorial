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

add_library(j_video_player INTERFACE)
target_include_directories(j_video_player INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(j_video_player INTERFACE Ring-Buffer farbot ffmpeg::ffmpeg SDL2::SDL2)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(j_video_player INTERFACE -Wall -Wextra -Wpedantic -Werror
            -Wno-c99-extensions
            -Wno-deprecated-declarations
            )
endif ()

add_subdirectory(tutorial00)
add_subdirectory(tutorial01)
add_subdirectory(tutorial02)
add_subdirectory(tutorial03)
add_subdirectory(tutorial04)
add_subdirectory(tutorial05)
add_subdirectory(tutorial07)
add_subdirectory(tutorial08)
add_subdirectory(tutorial09)

add_subdirectory(tests)