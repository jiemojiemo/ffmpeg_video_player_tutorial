//
// Created by user on 2/6/23.
//
#include <gmock/gmock.h>

#include "j_video_player/ffmpeg_utils/ffmpeg_decode_engine.h"

using namespace testing;
using namespace ffmpeg_utils;
using namespace std::literals;

namespace utils {
class AccurateSeeker {
public:
  explicit AccurateSeeker(FFMPEGDecodeEngine &e) : decode_engine(e) {}

  void seek(double pos) {
    if (!seek_req_) {
      std::lock_guard lg(seek_mut_);

      seek_pos_ = (int64_t)(pos * AV_TIME_BASE);
      seek_flags_ = AVSEEK_FLAG_BACKWARD;
      seek_req_ = true;
    }

    decode_engine.seek(pos);
  }

  int start() { return decode_engine.start(); }

  int stop() {
    {
      std::lock_guard lg(seek_mut_);
      seek_pos_ = 0;
      seek_req_ = false;
    }
    return decode_engine.stop();
  }

  AVFrame *pullVideoFrame() {
    if (seek_req_) {
      return pullVideoFrameWhenSeek();
    } else {
      return decode_engine.pullVideoFrame();
    }
  }

  AVFrame *pullAudioFrame() {
    if (seek_req_) {
      return pullVideoFrameWhenSeek();
    } else {
      return decode_engine.pullVideoFrame();
    }
  }

  FFMPEGDecodeEngine &decode_engine;

  bool getSeekReq() const { return seek_req_.load(); }

private:
  std::mutex seek_mut_;
  std::atomic<bool> seek_req_{false};
  int seek_flags_{0};
  int64_t seek_pos_{0}; // AV_TIME_BASE based
  //  int64_t seek_rel_{0}; // AV_TIME_BASE based

  AVFrame *pullVideoFrameWhenSeek() {
    for (;;) {
      auto *frame = decode_engine.pullVideoFrame();
      if (frame == nullptr) {
        return frame;
      }

      if (discardThisFrame(frame, decode_engine.video_stream->time_base,
                           seek_pos_)) {
        continue;
      } else {
        seek_req_ = false;
        return frame;
      }
    }
  }

  static bool discardThisFrame(AVFrame *frame, const AVRational &frame_timebase,
                               int64_t target_pos) {
    auto cur_frame_pts =
        av_rescale_q(frame->pts, frame_timebase, AV_TIME_BASE_Q);
    if (cur_frame_pts < target_pos) {
      return true;
    } else {
      return false;
    }
  }
};
} // namespace utils

using namespace utils;
class AAccurateSeeker : public Test {
public:
  void SetUp() override { s = std::make_unique<AccurateSeeker>(e); }
  FFMPEGDecodeEngine e;
  std::unique_ptr<AccurateSeeker> s;
  const std::string file_path = "/Users/user/Downloads/encode-v1/juren-30s.mp4";
};

TEST_F(AAccurateSeeker, InitWithDecodeEngine) {
  s = std::make_unique<AccurateSeeker>(e);
}

TEST_F(AAccurateSeeker, SeekTriggeSeekRequestOn) {
  double target_pos = 5;
  s->seek(target_pos);

  ASSERT_THAT(s->getSeekReq(), Eq(true));
}

TEST_F(AAccurateSeeker, StopResetSeekReq) {
  double target_pos = 5;
  s->seek(target_pos);
  s->stop();

  ASSERT_THAT(s->getSeekReq(), Eq(false));
}

TEST_F(AAccurateSeeker,
       CanPullVideoFrameFromEngineWithAccurateSeekingPosition) {

  e.openFile(file_path);

  s->start();
  double target_pos = 5;
  s->seek(target_pos);

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

    v_frame = s->pullVideoFrame();
    a_frame = s->pullAudioFrame();
    if (v_frame != nullptr) {
      ASSERT_THAT(v_frame, NotNull());
      double frame_pts = v_frame->pts * av_q2d(e.video_stream->time_base);
      ASSERT_THAT(frame_pts, DoubleNear(target_pos, 0.1));
      break;
    } else
      std::this_thread::sleep_for(10ms);
  }
}
