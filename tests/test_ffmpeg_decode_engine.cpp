//
// Created by user on 1/30/23.
//
#include <gmock/gmock.h>

#include "ffmpeg_utils/ffmpeg_decode_engine.h"

using namespace testing;
using namespace std::literals;
using namespace ffmpeg_utils;
class AFFMPEGDecodeEngine : public Test {
public:
  FFMPEGDecodeEngine e;
  const std::string file_path = "/Users/user/Downloads/encode-v1/juren-30s.mp4";
};

TEST_F(AFFMPEGDecodeEngine, CanOpenFile) {
  int ret = e.openFile(file_path);
  ASSERT_THAT(ret, Eq(0));
  ASSERT_TRUE(e.isOpenedOk());
}

TEST_F(AFFMPEGDecodeEngine, OpenFileFailedIfFilePathInvalid) {
  auto invalid_path = "xxx.mp4";

  ASSERT_THAT(e.openFile(invalid_path), Not(0));
  ASSERT_FALSE(e.isOpenedOk());
}

TEST_F(AFFMPEGDecodeEngine, InitDemuxerAfterOpenFile) {
  e.openFile(file_path);

  ASSERT_TRUE(e.demuxer.isValid());
}

TEST_F(AFFMPEGDecodeEngine, InitStreamsAfterOpenFile) {
  e.openFile(file_path);

  ASSERT_THAT(e.video_stream, NotNull());
  ASSERT_THAT(e.audio_stream, NotNull());
}

TEST_F(AFFMPEGDecodeEngine, InitCodecsAfterOpenFile) {
  e.openFile(file_path);

  ASSERT_THAT(e.video_codec_ctx, NotNull());
  ASSERT_THAT(e.audio_codec_ctx, NotNull());
}

TEST_F(AFFMPEGDecodeEngine, InStoppedStateWhenInit) {
  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kStopped));
}

TEST_F(AFFMPEGDecodeEngine, ChangeStateToDecodingAfterStart) {
  e.openFile(file_path);
  e.start();

  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kDecoding));
}

TEST_F(AFFMPEGDecodeEngine, StartFailedIfNeverOpenFile) {
  ASSERT_THAT(e.start(), Not(0));
}

TEST_F(AFFMPEGDecodeEngine, StartFailedNotChangeStateToDecoding) {
  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kStopped));
  int ret = e.start();

  ASSERT_THAT(ret, Not(0));
  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kStopped));
}

TEST_F(AFFMPEGDecodeEngine, StopChangeStateToStopped) {
  e.openFile(file_path);
  e.start();
  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kDecoding));

  e.stop();
  ASSERT_THAT(e.state(), Eq(DecodeEngineState::kStopped));
}

TEST_F(AFFMPEGDecodeEngine, CanPullVideoFrameAfterStart) {
  e.openFile(file_path);
  e.start();

  for (;;) {
    AVFrame *v_frame = nullptr;
    AVFrame *a_frame = nullptr;
    ON_SCOPE_EXIT([&] {
      if (v_frame != nullptr) {
        av_frame_unref(v_frame);
        av_frame_free(&v_frame);
      }

      if (a_frame != nullptr) {
        av_frame_unref(a_frame);
        av_frame_free(&a_frame);
      }
    });

    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (v_frame != nullptr) {
      ASSERT_THAT(v_frame, NotNull());
      break;
    }
  }
}

TEST_F(AFFMPEGDecodeEngine, CanPullAudioFrameAfterStart) {
  e.openFile(file_path);
  e.start();

  for (;;) {
    AVFrame *v_frame = nullptr;
    AVFrame *a_frame = nullptr;
    ON_SCOPE_EXIT([&] {
      if (v_frame != nullptr) {
        av_frame_unref(v_frame);
        av_frame_free(&v_frame);
      }

      if (a_frame != nullptr) {
        av_frame_unref(a_frame);
        av_frame_free(&a_frame);
      }
    });
    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (a_frame != nullptr) {
      ASSERT_THAT(a_frame, NotNull());
      break;
    } else
      std::this_thread::sleep_for(10ms);
  }
}

TEST_F(AFFMPEGDecodeEngine, CanSeekAbsolutely) {
  e.openFile(file_path);
  e.start();

  double target_pos = 5;
  e.seek(target_pos);

  for (;;) {
    AVFrame *v_frame = nullptr;
    AVFrame *a_frame = nullptr;
    ON_SCOPE_EXIT([&] {
      if (v_frame != nullptr) {
        av_frame_unref(v_frame);
        av_frame_free(&v_frame);
      }

      if (a_frame != nullptr) {
        av_frame_unref(a_frame);
        av_frame_free(&a_frame);
      }
    });

    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (v_frame != nullptr) {
      ASSERT_THAT(v_frame, NotNull());
      double frame_pts = v_frame->pts * av_q2d(e.video_stream->time_base);
      ASSERT_THAT(frame_pts, DoubleNear(target_pos, 0.1));
      break;
    } else
      std::this_thread::sleep_for(10ms);
  }
}

TEST_F(AFFMPEGDecodeEngine, CanGetInputFileVideoConfigAfterOpenFile) {
  e.openFile(file_path);
  auto video_config = e.getInputFileVideoConfig();

  ASSERT_THAT(video_config.height, Eq(e.video_codec_ctx->height));
  ASSERT_THAT(video_config.width, Eq(e.video_codec_ctx->width));
  ASSERT_THAT(video_config.pixel_format, Eq(e.video_codec_ctx->pix_fmt));
}

TEST_F(AFFMPEGDecodeEngine, CanGetInputFileAudioConfigAfterOpenFile) {
  e.openFile(file_path);
  auto audio_config = e.getInputFileAudioConfig();

  ASSERT_THAT(audio_config.num_channels, Eq(e.audio_codec_ctx->channels));
  ASSERT_THAT(audio_config.channel_layout,
              Eq(e.audio_codec_ctx->channel_layout));
  ASSERT_THAT(audio_config.sample_rate, Eq(e.audio_codec_ctx->sample_rate));
}

TEST_F(AFFMPEGDecodeEngine, StartWithOutputConfigPullFrameWithTargetConfig) {
  e.openFile(file_path);
  auto video_config = e.getInputFileVideoConfig();
  video_config.width = 200;
  video_config.height = 200;
  video_config.pixel_format = AVPixelFormat::AV_PIX_FMT_YUV411P;

  e.start(&video_config, nullptr);

  for (;;) {
    AVFrame *v_frame = nullptr;
    AVFrame *a_frame = nullptr;
    ON_SCOPE_EXIT([&] {
      if (v_frame != nullptr) {
        av_frame_unref(v_frame);
        av_frame_free(&v_frame);
      }

      if (a_frame != nullptr) {
        av_frame_unref(a_frame);
        av_frame_free(&a_frame);
      }
    });

    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (v_frame != nullptr) {
      ASSERT_THAT(v_frame, NotNull());
      ASSERT_THAT(v_frame->width, Eq(video_config.width));
      ASSERT_THAT(v_frame->height, Eq(video_config.height));
      ASSERT_THAT(v_frame->format, Eq(video_config.pixel_format));
      break;
    } else
      std::this_thread::sleep_for(10ms);
  }
}

TEST_F(AFFMPEGDecodeEngine,
       PullAudioSamplesReturnsFetchedNumberSamplesAndLastFramePts) {
  e.openFile(file_path);
  auto audio_config = e.getInputFileAudioConfig();
  audio_config.sample_rate = 16000;
  audio_config.num_channels = 2;
  audio_config.channel_layout = AV_CH_LAYOUT_STEREO;

  e.start(nullptr, &audio_config);

  int num_samples_per_channel = 1024;
  std::vector<int16_t> out_buffer(audio_config.num_channels *
                                  num_samples_per_channel);

  for (;;) {
    AVFrame *v_frame = nullptr;
    ON_SCOPE_EXIT([&] {
      if (v_frame != nullptr) {
        av_frame_unref(v_frame);
        av_frame_free(&v_frame);
      }
    });

    v_frame = e.pullVideoFrame();
    auto [num_samples_out, last_pts] = e.pullAudioSamples(
        num_samples_per_channel, audio_config.num_channels, out_buffer.data());
    if (num_samples_out == num_samples_per_channel) {
      ASSERT_THAT(num_samples_out, Eq(num_samples_per_channel));
      break;
    } else {
      std::this_thread::sleep_for(10ms);
    }
  }
}