add_executable(tutorial04-resampled tutorial04-resampled.c)
target_link_libraries(tutorial04-resampled PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial04 my_tutorial04_01_timer.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial04 PRIVATE my_utils ${CONAN_LIBS})
