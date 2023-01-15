add_library(my_utils INTERFACE)
target_include_directories(my_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(my_utils INTERFACE ${CONAN_LIBS})