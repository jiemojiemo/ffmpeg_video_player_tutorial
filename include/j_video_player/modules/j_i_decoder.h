//
// Created by user on 5/20/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_I_DECODER_H

namespace j_video_player {
class IDecoder {
public:
  virtual ~IDecoder() = default;
  virtual void start() = 0;
  virtual void pause() = 0;
  virtual void stop() = 0;
  virtual void seek(float pos) = 0;
  virtual float getDuration() = 0;
  virtual float getCurrentPosition() = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_DECODER_H