//
// Created by user on 11/13/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_VIDEO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_I_VIDEO_OUTPUT_H
#include "j_video_player/ffmpeg_utils/ffmpeg_image_converter.h"
#include "j_video_player/modules/j_i_source.h"
#include "j_video_player/utils/clock_manager.h"

namespace j_video_player {
class VideoOutputParameters {
public:
  int width{0};
  int height{0};
  int fps{0};
  int pixel_format{0}; // AVPixelFormat
};

enum class OutputState { kIdle, kPlaying, kPaused, kStopped };

class IVideoOutput {
public:
  virtual ~IVideoOutput() = default;

  virtual int prepare(const VideoOutputParameters &parameters) = 0;
  virtual void attachSource(std::shared_ptr<ISource> source) = 0;
  virtual void attachImageConverter(
      std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> converter) = 0;
  virtual void
  attachAVSyncClock(std::shared_ptr<utils::ClockManager> clock) = 0;
  virtual int play() = 0;
  virtual int pause() = 0;
  virtual int stop() = 0;
  virtual OutputState getState() const = 0;
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_VIDEO_OUTPUT_H
