add_executable(tutorial07 tutorial07.c)
target_link_libraries(tutorial07 PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial07 my_tutorial07.cpp)
target_link_libraries(my_tutorial07 PRIVATE my_utils ${CONAN_LIBS})

add_executable(my_tutorial07_01_accurate_seek my_tutorial07_01_accurate_seek.cpp)
target_link_libraries(my_tutorial07_01_accurate_seek PRIVATE my_utils ${CONAN_LIBS})

