//
// Created by user on 11/11/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
#define FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
#include "aloop.h"
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_i_source.h"
#include "j_video_player/utils/waitable_event.h"
#include "j_video_player/utils/waitable_queue.h"
#include <thread>

namespace j_video_player {

enum class SimpleSourceMsgType {
  kOpenFile = 0,
  kStartPlay,
  kDecoding,
  kStop,
  kPause,
  kSeeking
};

template <typename DecoderType>
class SimpleSource
    : public IVideoSource,
      public IAudioSource,
      public aloop::AHandler,
      public std::enable_shared_from_this<SimpleSource<DecoderType>> {
public:
  SimpleSource()
      : frame_queue_(std::make_unique<QueueType>(kQueueSize)),
        looper_(aloop::ALooper::create()) {}
  void prepare(std::shared_ptr<DecoderType> decoder) {
    decoder_ = std::move(decoder);
    auto ptr = this->shared_from_this();
    looper_->registerHandler(ptr);
    looper_->start();
  }
  ~SimpleSource() override { cleanup(); };
  int open(const std::string &file_path) override {
    auto ptr = this->shared_from_this();
    auto msg =
        aloop::AMessage::create(uint32_t(SimpleSourceMsgType::kOpenFile), ptr);
    msg->setString("file_path", file_path);

    auto rep = aloop::AMessage::createNull();
    msg->postAndAwaitResponse(&rep);

    int ret = -1;
    rep->findInt32("ret", &ret);
    return ret;
  }
  MediaFileInfo getMediaFileInfo() override {
    if (decoder_) {
      return decoder_->getMediaFileInfo();
    }
    return {};
  }

  int play() override {
    wait_event_.signal();
    auto msg = aloop::AMessage::create(
        (uint32_t)(SimpleSourceMsgType::kStartPlay), this->shared_from_this());
    auto rep = aloop::AMessage::createNull();
    msg->postAndAwaitResponse(&rep);
    return 0;
  }
  int pause() override {
    auto msg = aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kPause),
                                       this->shared_from_this());
    auto rep = aloop::AMessage::createNull();
    msg->postAndAwaitResponse(&rep);
    return 0;
  }
  int stop() override {
    frame_queue_->flush();
    wait_event_.signal();
    auto msg = aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kStop),
                                       this->shared_from_this());
    auto rep = aloop::AMessage::createNull();
    msg->postAndAwaitResponse(&rep);
    return 0;
  }
  int seek(int64_t timestamp) override {
    seek_timestamp_ = timestamp;
    state_ = SourceState::kSeeking;
    wait_event_.signal();
    auto msg = aloop::AMessage::create(
        (uint32_t)(SimpleSourceMsgType::kSeeking), this->shared_from_this());
    msg->setInt64("timestamp", timestamp);
    msg->post();
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

protected:
  void onMessageReceived(const std::shared_ptr<aloop::AMessage> &msg) override {
    auto type = (SimpleSourceMsgType)(msg->what());

    if (type == SimpleSourceMsgType::kOpenFile) {
      handleOpenFile(msg);
    } else if (type == SimpleSourceMsgType::kStartPlay) {
      handlePlay(msg);
    } else if (type == SimpleSourceMsgType::kDecoding) {
      handleDecoding(msg);
    } else if (type == SimpleSourceMsgType::kStop) {
      handleStop(msg);
    } else if (type == SimpleSourceMsgType::kPause) {
      handlePause(msg);
    } else if (type == SimpleSourceMsgType::kSeeking) {
      handleSeeking(msg);
    }
  }

private:
  void handleSeeking(const std::shared_ptr<aloop::AMessage> &msg) {
    state_ = SourceState::kSeeking;
    int64_t timestamp = 0;
    if (msg->findInt64("timestamp", &timestamp)) {
      seek_timestamp_ = timestamp;
      frame_queue_->flush();
      auto frame = decoder_->seekFramePrecise(seek_timestamp_);
      if (frame) {
        frame_queue_->wait_and_push(std::move(frame));
        aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kPause),
                                this->shared_from_this())
            ->post();
      } else {
        aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kStop),
                                this->shared_from_this())
            ->post();
      }
    }
  }
  void handlePause(const std::shared_ptr<aloop::AMessage> &msg) {
    state_ = SourceState::kPaused;
    std::shared_ptr<aloop::AReplyToken> reply_token;
    if (msg->senderAwaitsResponse(&reply_token)) {
      auto rep = aloop::AMessage::create();
      rep->postReply(reply_token);
    }
    wait_event_.wait(-1);
  }
  void handleDecoding(const std::shared_ptr<aloop::AMessage> &msg) {
    (void)(msg);
    auto frame = decoder_->decodeNextFrame();
    if (frame) {
      frame_queue_->wait_and_push(std::move(frame));
      aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kDecoding),
                              this->shared_from_this())
          ->post();
    } else {
      aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kStop),
                              this->shared_from_this())
          ->post();
    }
  }
  void handleStop(const std::shared_ptr<aloop::AMessage> &msg) {
    state_ = SourceState::kStopped;
    frame_queue_->flush();
    closeDecoder();

    std::shared_ptr<aloop::AReplyToken> reply_token;
    if (msg->senderAwaitsResponse(&reply_token)) {
      auto rep = aloop::AMessage::create();
      rep->postReply(reply_token);
    }
  }
  void handlePlay(const std::shared_ptr<aloop::AMessage> &msg) {
    state_ = SourceState::kPlaying;
    aloop::AMessage::create((uint32_t)(SimpleSourceMsgType::kDecoding),
                            this->shared_from_this())
        ->post();
    std::shared_ptr<aloop::AReplyToken> reply_token;
    if (msg->senderAwaitsResponse(&reply_token)) {
      auto rep = aloop::AMessage::create();
      rep->postReply(reply_token);
    }
  }
  void handleOpenFile(const std::shared_ptr<aloop::AMessage> &msg) {
    std::string file_path;
    msg->findString("file_path", &file_path);

    int ret = 0;
    if (decoder_ == nullptr) {
      ret = -1;
    } else {
      ret = decoder_->open(file_path);
    }

    std::shared_ptr<aloop::AReplyToken> reply_token;
    if (msg->senderAwaitsResponse(&reply_token)) {
      auto rep = aloop::AMessage::create();
      rep->setInt32("ret", ret);
      rep->postReply(reply_token);
    }
  }
  std::shared_ptr<Frame> tryPopAFrame() {
    std::shared_ptr<Frame> f = nullptr;
    frame_queue_->try_pop(f);
    return f;
  }

  void closeDecoder() {
    if (decoder_) {
      decoder_->close();
    }
  }

  void cleanup() {
    if (frame_queue_) {
      frame_queue_->flush();
    }
    wait_event_.signal();

    looper_->stop();
  }

  bool isValid() { return decoder_ && decoder_->isValid(); }
  std::shared_ptr<DecoderType> decoder_;
  std::atomic<SourceState> state_{SourceState::kIdle};
  WaitableEvent wait_event_;
  std::atomic<int64_t> seek_timestamp_{0};
  using QueueType = utils::WaitableQueue<std::shared_ptr<Frame>>;
  std::unique_ptr<QueueType> frame_queue_ = nullptr;
  constexpr static int kQueueSize = 3;

  std::shared_ptr<aloop::ALooper> looper_ = nullptr;
};

using SimpleVideoSource = SimpleSource<IVideoDecoder>;
using SimpleAudioSource = SimpleSource<IAudioDecoder>;

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SIMPLE_SOURCE_H
