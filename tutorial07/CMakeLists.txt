add_executable(tutorial07 tutorial07.c)
target_link_libraries(tutorial07 PRIVATE ffmpeg::ffmpeg SDL2::SDL2)

add_executable(my_tutorial07 my_tutorial07.cpp)
target_link_libraries(my_tutorial07 PRIVATE j_video_player)

add_executable(my_tutorial07_01_accurate_seek my_tutorial07_01_accurate_seek.cpp)
target_link_libraries(my_tutorial07_01_accurate_seek PRIVATE j_video_player)

