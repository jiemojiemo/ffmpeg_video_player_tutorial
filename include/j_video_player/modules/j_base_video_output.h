//
// Created by user on 11/13/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_BASE_VIDEO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_BASE_VIDEO_OUTPUT_H
#include "j_video_player/modules/j_i_video_output.h"
#include "j_video_player/utils/av_synchronizer.h"
#include <thread>
namespace j_video_player {
class BaseVideoOutput : public IVideoOutput {
public:
  ~BaseVideoOutput() override { cleanup(); }

  void attachSource(std::shared_ptr<ISource> source) override {
    source_ = std::move(source);
  }
  void attachImageConverter(
      std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> converter) override {
    converter_ = std::move(converter);
  }
  void attachAVSyncClock(std::shared_ptr<utils::ClockManager> clock) override {
    clock_ = std::move(clock);
  }
  int play() override {
    if (source_ == nullptr) {
      LOGW("source is null, can't play. Please attach source first");
      return -1;
    }
    state_ = OutputState::kPlaying;
    if (!isThreadRunning()) {
      startOutputThread();
    }
    return 0;
  }
  OutputState getState() const override { return state_.load(); }
  int pause() override {
    state_ = OutputState::kPaused;
    return 0;
  }
  int stop() override {
    state_ = OutputState::kStopped;
    return 0;
  }

protected:
  virtual int drawFrame(std::shared_ptr<Frame> frame) = 0;

  void startOutputThread() {
    output_thread_ = std::make_unique<std::thread>([this]() {
      for (;;) {
        if (state_ == OutputState::kStopped || state_ == OutputState::kIdle) {
          break;
        } else if (state_ == OutputState::kPaused) {
          continue;
        } else if (state_ != OutputState::kPlaying) {
          if (source_ == nullptr) {
            LOGW("source is null, can't play. Please attach source first");
            break;
          }
          auto frame = source_->dequeueFrame();
          if (frame == nullptr) {
            LOGW("frame is null, skit this frame");
            continue;
          }

          std::shared_ptr<Frame> frame_for_draw = convertFrame(frame);

          if (frame_for_draw != nullptr) {
            drawFrame(frame_for_draw);
            doAVSync(frame_for_draw->pts_d());
          }
        }
      }
    });
  }

  bool isThreadRunning() const {
    return output_thread_ != nullptr && output_thread_->joinable();
  }

  std::shared_ptr<Frame> convertFrame(std::shared_ptr<Frame> frame) {
    if (converter_) {
      return converter_->convert2(frame);
    }
    return frame;
  }

  void doAVSync(double pts_d) {
    if (clock_) {
      clock_->setVideoClock(pts_d);
      auto real_delay_ms = (int)(av_sync_.computeTargetDelay(*clock_) * 1000);
      std::this_thread::sleep_for(std::chrono::milliseconds(real_delay_ms));
    }
  }


  std::shared_ptr<ISource> source_;
  std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> converter_;
  std::shared_ptr<utils::ClockManager> clock_;
  std::unique_ptr<std::thread> output_thread_;
  std::atomic<OutputState> state_{OutputState::kIdle};
  utils::AVSynchronizer av_sync_;

private:
  void cleanup() {
    state_ = OutputState::kIdle;

    if (output_thread_ != nullptr && output_thread_->joinable()) {
      output_thread_->join();
      output_thread_ = nullptr;
    }
  }
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_BASE_VIDEO_OUTPUT_H
