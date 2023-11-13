add_subdirectory(googletest)

add_executable(unit_test
        ../lsan_suppressions.cpp
        test_accurate_seeker.cpp
        test_av_synchronizer.cpp
        test_clock_manager.cpp
        test_ffmpeg_audio_resampler.cpp
        test_ffmpeg_codec.cpp
        test_ffmpeg_decode_engine.cpp
        test_ffmpeg_demuxer.cpp
        test_ffmpeg_format_utils.cpp
        test_ffmpeg_frame_queue.cpp
        test_ffmpeg_image_converter.cpp
        test_ffmpeg_packet_queue.cpp
        test_ffmpeg_waitalbe_packet_queue.cpp
        test_ffmpeg_waitalbe_frame_queue.cpp
        test_waitable_queue.cpp

        modules/test_base_decoder.cpp
        modules/test_audio_decoder.cpp
        modules/test_audio_sample_dispatcher.cpp
        modules/test_ffmpeg_audio_decoder.cpp
        modules/test_ffmpeg_video_decoder.cpp
        modules/test_sdl2_render.cpp
        modules/test_base_video_output.cpp
        modules/test_video_decoder.cpp
        modules/test_simple_video_source.cpp
        )
target_link_libraries(unit_test PRIVATE gmock_main j_video_player)