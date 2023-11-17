set(third_party_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
set(include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_library(j_video_player INTERFACE)

if (APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(ffmpeg CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)

    target_include_directories(j_video_player INTERFACE ${include_dir})
    target_link_libraries(j_video_player INTERFACE Ring-Buffer farbot ffmpeg::ffmpeg SDL2::SDL2)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(j_video_player INTERFACE -Wall -Wextra -Wpedantic -Werror
                -Wno-c99-extensions
                -Wno-deprecated-declarations
                )
    endif ()

elseif (ANDROID)
    find_library(log-lib log)

    target_link_libraries(j_video_player INTERFACE ffmpeg_libs Ring-Buffer ${log-lib})
    target_include_directories(j_video_player INTERFACE ${include_dir})
endif ()