add_executable(tutorial01 tutorial01.cpp ../lsan_suppressions.cpp)
target_link_libraries(tutorial01 PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial01 my_tutorial01.cpp ../lsan_suppressions.cpp)
target_link_libraries(my_tutorial01 PRIVATE j_video_player ${CONAN_LIBS})
