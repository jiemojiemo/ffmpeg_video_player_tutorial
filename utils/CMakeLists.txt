add_library(my_utils INTERFACE)
target_include_directories(my_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(my_utils INTERFACE ${CONAN_LIBS})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(my_utils INTERFACE -Wall -Wextra -Wpedantic -Werror
            -Wno-c99-extensions
            )
endif()