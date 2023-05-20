cmake_minimum_required(VERSION 3.10)

project(ffmpeg_video_player)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 17)

if (ENABLE_ASAN)
    message(STATUS "build with ASAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

add_library(j_video_player INTERFACE)
target_include_directories(j_video_player INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(j_video_player INTERFACE Ring-Buffer ${CONAN_LIBS})
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(j_video_player INTERFACE -Wall -Wextra -Wpedantic -Werror
            -Wno-c99-extensions
            )
endif ()

add_subdirectory(3rdparty)

add_subdirectory(tutorial01)
add_subdirectory(tutorial02)
add_subdirectory(tutorial03)
add_subdirectory(tutorial04)
add_subdirectory(tutorial05)
add_subdirectory(tutorial07)
add_subdirectory(tutorial08)

add_subdirectory(tests)