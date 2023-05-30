add_executable(tutorial03-resampled tutorial03-resampled.c)
target_link_libraries(tutorial03-resampled PRIVATE ffmpeg::ffmpeg SDL2::SDL2)

add_executable(my_tutorial03 my_tutorial03.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial03 PRIVATE j_video_player)

add_executable(my_tutorial03_01 my_tutorial03_01.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial03_01 PRIVATE j_video_player)

add_executable(my_tutorial03_02 my_tutorial03_02.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial03_02 PRIVATE j_video_player)
