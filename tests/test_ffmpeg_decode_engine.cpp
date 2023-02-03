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

  for (;;) {
    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (v_frame != nullptr)
      break;
  }

  ASSERT_THAT(v_frame, NotNull());
}

TEST_F(AFFMPEGDecodeEngine, CanPullAudioFrameAfterStart) {
  e.openFile(file_path);
  e.start();

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

  for (;;) {
    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (a_frame != nullptr)
      break;
    else
      std::this_thread::sleep_for(10ms);
  }

  ASSERT_THAT(a_frame, NotNull());
}

TEST_F(AFFMPEGDecodeEngine, CanSeekAbsolutely) {
  e.openFile(file_path);
  e.start();

  double target_pos = 5;
  e.seek(target_pos);

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

  for (;;) {
    v_frame = e.pullVideoFrame();
    a_frame = e.pullAudioFrame();
    if (v_frame != nullptr)
      break;
    else
      std::this_thread::sleep_for(10ms);
  }

  ASSERT_THAT(v_frame, NotNull());
  double frame_pts = v_frame->pts * av_q2d(e.video_stream->time_base);
  ASSERT_THAT(frame_pts, DoubleNear(target_pos, 0.1));
}