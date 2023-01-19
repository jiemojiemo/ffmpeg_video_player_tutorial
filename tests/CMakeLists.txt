add_subdirectory(googletest)

add_executable(unit_test
        ../lsan_suppressions.cpp
        test_ffmpeg_audio_resampler.cpp
        test_ffmpeg_codec.cpp
        test_ffmpeg_demuxer.cpp
        test_ffmpeg_format_utils.cpp
        test_ffmpeg_frame_queue.cpp
        test_ffmpeg_image_converter.cpp
        test_ffmpeg_packet_queue.cpp
        )
target_link_libraries(unit_test PRIVATE gmock_main my_utils)