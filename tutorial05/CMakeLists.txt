add_executable(tutorial05 tutorial05.c)
target_link_libraries(tutorial05 PRIVATE ${CONAN_LIBS})

add_executable(my_tutorial05 my_tutorial05.cpp)
target_link_libraries(my_tutorial05 PRIVATE my_utils ${CONAN_LIBS})
