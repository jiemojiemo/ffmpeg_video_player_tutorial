//
// Created by user on 11/11/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_SOURCE_H
#define FFMPEG_VIDEO_PLAYER_J_I_SOURCE_H
#include "j_video_player/modules/j_frame.h"
#include "j_video_player/modules/j_media_file_info.h"

namespace j_video_player {
enum class SourceState {
  kIdle,
  kStopped,
  kPlaying,
  kSeeking,
  kPaused,
};
class ISource {
public:
  virtual ~ISource() = default;

  virtual int open(const std::string &file_path) = 0;
  virtual MediaFileInfo getMediaFileInfo() = 0;
  virtual int play() = 0;
  virtual int pause() = 0;
  virtual int stop() = 0;
  virtual int seek(int64_t timestamp) = 0;
  virtual SourceState getState() = 0;
  virtual int64_t getDuration() = 0;
  virtual int64_t getCurrentPosition() = 0;
  virtual std::shared_ptr<Frame> dequeueFrame() = 0;
  virtual int getQueueSize() = 0;
};

class IVideoSource : public ISource {
public:
  std::shared_ptr<Frame> dequeueFrame() override { return dequeueVideoFrame(); }
  virtual std::shared_ptr<Frame> dequeueVideoFrame() = 0;
};

class IAudioSource : public ISource {
public:
  std::shared_ptr<Frame> dequeueFrame() override { return dequeueAudioFrame(); }
  virtual std::shared_ptr<Frame> dequeueAudioFrame() = 0;
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_SOURCE_H
