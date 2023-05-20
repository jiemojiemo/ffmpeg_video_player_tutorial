add_executable(tutorial05 tutorial05.c)
target_link_libraries(tutorial05 PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial05 my_tutorial05.cpp)
target_link_libraries(my_tutorial05 PRIVATE j_video_player ${CONAN_LIBS})

add_executable(my_tutorial05_01_clock my_tutorial05_01_clock.cpp)
target_link_libraries(my_tutorial05_01_clock PRIVATE j_video_player ${CONAN_LIBS})
