//
// Created by user on 11/14/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_AUDIO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_I_AUDIO_OUTPUT_H
#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/modules/j_i_source.h"
#include "j_video_player/utils/clock_manager.h"

namespace j_video_player {
enum class AudioOutputState { kIdle, kPlaying, kStopped };
class AudioOutputParameters {
public:
  int sample_rate{44100};
  int channels{2};
  int num_frames_of_buffer{1024};

  bool isValid() const {
    return sample_rate > 0 && channels > 0 && num_frames_of_buffer > 0;
  }
};

class IAudioOutput {
public:
  virtual ~IAudioOutput() = default;

  virtual int prepare(const AudioOutputParameters &params) = 0;
  virtual void attachSource(std::shared_ptr<ISource> source) = 0;
  virtual void attachResampler(
      std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler) = 0;
  virtual void
  attachAVSyncClock(std::shared_ptr<utils::ClockManager> clock) = 0;
  virtual int play() = 0;
  virtual int stop() = 0;
  virtual AudioOutputState getState() const = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_AUDIO_OUTPUT_H
