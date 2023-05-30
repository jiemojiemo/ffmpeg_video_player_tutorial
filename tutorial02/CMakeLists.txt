add_executable(tutorial02 tutorial02.c ../lsan_suppressions.cpp)
target_link_libraries(tutorial02 PRIVATE ffmpeg::ffmpeg SDL2::SDL2)

add_executable(my_tutorial02 my_tutorial02.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial02 PRIVATE j_video_player)
