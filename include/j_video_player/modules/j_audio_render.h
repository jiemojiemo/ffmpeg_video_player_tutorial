//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_RENDER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_RENDER_H
#include <cstdint>
namespace j_video_player {
class IAudioRender {
public:
  virtual ~IAudioRender() = default;
  virtual void initAudioRender() = 0;
  virtual void uninit() = 0;
  virtual void clearAudioCache() = 0;
  virtual void renderAudioData(uint8_t *data, int size) = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_RENDER_H
