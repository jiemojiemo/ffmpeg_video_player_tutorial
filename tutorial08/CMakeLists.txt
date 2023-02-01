add_executable(my_tutorial08 my_tutorial08.cpp)
target_link_libraries(my_tutorial08 PRIVATE my_utils ${CONAN_LIBS})