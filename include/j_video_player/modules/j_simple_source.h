//
// Created by user on 11/11/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
#define FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_i_source.h"
#include "j_video_player/utils/waitable_event.h"
#include "j_video_player/utils/waitable_queue.h"
#include <thread>

namespace j_video_player {

template <typename DecoderType>
class SimpleSource : public IVideoSource, public IAudioSource {
public:
  explicit SimpleSource(std::shared_ptr<DecoderType> decoder)
      : decoder_(std::move(decoder)),
        frame_queue_(std::make_unique<QueueType>(kQueueSize)) {}
  ~SimpleSource() override { cleanup(); };
  int open(const std::string &file_path) override {
    if (decoder_) {
      return decoder_->open(file_path);
    } else {
      return -1;
    }
  }
  MediaFileInfo getMediaFileInfo() override {
    if (decoder_) {
      return decoder_->getMediaFileInfo();
    }
    return {};
  }

  int play() override {
    state_ = SourceState::kPlaying;
    if (!isThreadRunning()) {
      startDecodeThread();
    }
    wait_event_.signal();
    return 0;
  }
  int pause() override {
    state_ = SourceState::kPaused;
    return 0;
  }
  int stop() override {
    state_ = SourceState::kStopped;
    stopDecodeThread();
    closeDecoder();
    return 0;
  }
  int seek(int64_t timestamp) override {
    seek_timestamp_ = timestamp;
    state_ = SourceState::kSeeking;
    if (!isThreadRunning()) {
      startDecodeThread();
    }
    wait_event_.signal();
    return 0;
  }
  SourceState getState() override { return state_; }
  int64_t getDuration() override {
    if (decoder_ && decoder_->isValid()) {
      return decoder_->getMediaFileInfo().duration;
    }
    return 0;
  }
  int64_t getCurrentPosition() override {
    if (decoder_) {
      return decoder_->getPosition();
    }
    return 0;
  }
  std::shared_ptr<Frame> dequeueVideoFrame() override { return tryPopAFrame(); }

  std::shared_ptr<Frame> dequeueAudioFrame() override { return tryPopAFrame(); }

  int getQueueSize() override { return frame_queue_->size(); }

private:
  std::shared_ptr<Frame> tryPopAFrame() {
    std::shared_ptr<Frame> f = nullptr;
    frame_queue_->try_pop(f);
    return f;
  }
  void startDecodeThread() {
    decode_thread_ =
        std::make_unique<std::thread>(&SimpleSource::decodingThread, this);
  }

  void stopDecodeThread() {
    if (decode_thread_ && decode_thread_->joinable()) {
      frame_queue_->flush();
      wait_event_.signal();
      decode_thread_->join();
      decode_thread_ = nullptr;
    }
  }

  void closeDecoder() {
    if (decoder_) {
      decoder_->close();
    }
  }

  void decodingThread() {
    if (!isValid()) {
      LOGE("decoder is not valid, can not start decode thread");
      return;
    }
    for (;;) {
      if (state_ == SourceState::kPlaying) {
        auto frame = decoder_->decodeNextFrame();
        if (frame) {
          frame_queue_->wait_and_push(std::move(frame));
        } else {
          state_ = SourceState::kStopped;
        }
      } else if (state_ == SourceState::kSeeking) {
        frame_queue_->flush();
        auto frame = decoder_->seekFramePrecise(seek_timestamp_);
        if (frame) {
          frame_queue_->wait_and_push(std::move(frame));
          state_ = SourceState::kPaused;
        } else {
          state_ = SourceState::kStopped;
        }
      } else if (state_ == SourceState::kPaused) {
        wait_event_.wait(-1);
      } else if (state_ == SourceState::kStopped) {
        frame_queue_->flush();
        break;
      } else if (state_ == SourceState::kIdle) {
        break;
      }
    }
  }

  void cleanup() {
    stop();

    if (frame_queue_) {
      frame_queue_->flush();
    }

    wait_event_.signal();

    if (decode_thread_ && decode_thread_->joinable()) {
      decode_thread_->join();
    }
  }

  bool isThreadRunning() { return decode_thread_ != nullptr; }

  bool isValid() { return decoder_ && decoder_->isValid(); }
  std::shared_ptr<DecoderType> decoder_;
  std::atomic<SourceState> state_{SourceState::kIdle};
  std::unique_ptr<std::thread> decode_thread_;
  WaitableEvent wait_event_;
  std::atomic<int64_t> seek_timestamp_{0};
  using QueueType = utils::WaitableQueue<std::shared_ptr<Frame>>;
  std::unique_ptr<QueueType> frame_queue_ = nullptr;
  constexpr static int kQueueSize = 3;
};

using SimpleVideoSource = SimpleSource<IVideoDecoder>;
using SimpleAudioSource = SimpleSource<IAudioDecoder>;

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
