add_executable(tutorial03-resampled tutorial03-resampled.c)
target_link_libraries(tutorial03-resampled PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial03 my_tutorial03.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial03 PRIVATE my_utils ${CONAN_LIBS})
