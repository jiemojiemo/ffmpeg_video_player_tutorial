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

  int prepare(const VideoOutputParameters &parameters) override {
    if (parameters.width <= 0 || parameters.height <= 0) {
      LOGE("invalid width or height");
      return -1;
    }
    parameters_ = parameters;
    return 0;
  }

  void attachVideoSource(std::shared_ptr<IVideoSource> source) override {
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
    stopOutputThread();
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
        } else if (state_ == OutputState::kPlaying) {
          if (source_ == nullptr) {
            LOGW("source is null, can't play. Please attach source first");
            break;
          }
          auto frame = source_->dequeueVideoFrame();
          if (frame == nullptr) {
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
  void stopOutputThread() {
    if (output_thread_ != nullptr && output_thread_->joinable()) {
      output_thread_->join();
      output_thread_ = nullptr;
      LOGD("video output thread stopped\n");
    }
  }

  bool isThreadRunning() const { return output_thread_ != nullptr; }

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
    } else {
      auto delay_ms = parameters_.fps > 0 ? (int)(1000 / parameters_.fps) : 30;
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
  }

  void cleanup() {
    state_ = OutputState::kIdle;

    stopOutputThread();
  }

  std::shared_ptr<IVideoSource> source_;
  std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> converter_;
  std::shared_ptr<utils::ClockManager> clock_;
  std::unique_ptr<std::thread> output_thread_;
  std::atomic<OutputState> state_{OutputState::kIdle};
  utils::AVSynchronizer av_sync_;
  VideoOutputParameters parameters_;
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_BASE_VIDEO_OUTPUT_H
