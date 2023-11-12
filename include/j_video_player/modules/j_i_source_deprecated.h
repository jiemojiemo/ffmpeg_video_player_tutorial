//
// Created by user on 5/20/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_SOURCE_DEPRECATED_H
#define FFMPEG_VIDEO_PLAYER_J_I_SOURCE_DEPRECATED_H

namespace j_video_player {
class ISourceDeprecated {
public:
  virtual ~ISourceDeprecated() = default;
  virtual void start() = 0;
  virtual void pause() = 0;
  virtual void stop() = 0;
  virtual void seek(float pos) = 0;
  virtual float getDuration() = 0;
  virtual float getCurrentPosition() = 0;
  virtual bool isPlaying() const = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_SOURCE_DEPRECATED_H
