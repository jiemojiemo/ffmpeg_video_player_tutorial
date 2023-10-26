add_library(ffmpeg_libs INTERFACE)

if (ANDROID)
    set(libs_dir "${CMAKE_CURRENT_SOURCE_DIR}/android/${CMAKE_ANDROID_ARCH_ABI}/lib")

    add_library(libavcodec IMPORTED SHARED)
    set_target_properties(libavcodec PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libavcodec.so")

    add_library(libavfilter IMPORTED SHARED)
    set_target_properties(libavfilter PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libavfilter.so")

    add_library(libavformat IMPORTED SHARED)
    set_target_properties(libavformat PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libavformat.so")

    add_library(libavutil IMPORTED SHARED)
    set_target_properties(libavutil PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libavutil.so")

    add_library(libswresample IMPORTED SHARED)
    set_target_properties(libswresample PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libswresample.so")

    add_library(libswscale IMPORTED SHARED)
    set_target_properties(libswscale PROPERTIES
            IMPORTED_LOCATION "${libs_dir}/libswscale.so")

    target_include_directories(ffmpeg_libs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/android/${CMAKE_ANDROID_ARCH_ABI}/include)
    target_link_libraries(ffmpeg_libs INTERFACE
            libavcodec
            libavfilter
            libavformat
            libavutil
            libswresample
            libswscale)

endif ()