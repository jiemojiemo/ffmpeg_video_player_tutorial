cmake_minimum_required(VERSION 3.22.1)

project("usesurfaceview")

add_library(usesurfaceview
        SHARED
        usesurfaceview.cpp)


find_library(log-lib log)

target_link_libraries(
        usesurfaceview

        PRIVATE
        android
        jnigraphics)

#add_library()