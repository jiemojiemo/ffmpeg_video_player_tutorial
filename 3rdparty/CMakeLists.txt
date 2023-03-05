add_library(Ring-Buffer INTERFACE)
target_include_directories(Ring-Buffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Ring-Buffer)

add_subdirectory(ffmpeg)