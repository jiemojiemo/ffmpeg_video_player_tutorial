add_executable(tutorial04-resampled tutorial04-resampled.c)
target_link_libraries(tutorial04-resampled PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial04_01_timer my_tutorial04_01_timer.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial04_01_timer PRIVATE j_video_player ${CONAN_LIBS})

add_executable(my_tutorial04_02_threads my_tutorial04_02_threads.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial04_02_threads PRIVATE j_video_player ${CONAN_LIBS})

add_executable(my_tutorial04_03_fix_size_queue my_tutorial04_03_fix_size_queue.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial04_03_fix_size_queue PRIVATE j_video_player ${CONAN_LIBS})
