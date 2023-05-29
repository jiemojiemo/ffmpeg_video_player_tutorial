//
// Created by user on 5/28/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_SAMPLE_DISPATCHER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_SAMPLE_DISPATCHER_H

#include "ringbuffer.hpp"
#include <thread>

namespace j_video_player {
class AudioSampleDispatcher {
public:
  AudioSampleDispatcher() { clearAudioCache(); }
  size_t getAvailableReadSize() { return audio_sample_buffer_.readAvailable(); }
  void clearAudioCache() { audio_sample_buffer_.producerClear(); }
  void waitAndPushSamples(int16_t *samples, size_t nb_samples, int64_t pts) {
    for (; audio_sample_buffer_.writeAvailable() < nb_samples;) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (auto i = 0u; i < nb_samples; ++i) {
      audio_sample_buffer_.insert({samples[i], pts});
    }
  }

  int64_t pullAudioSamples(int16_t *out_samples, size_t nb_samples) {

    size_t sample_index = 0;
    int64_t pts = 0;
    for (; nb_samples-- > 0;) {
      if (auto s = audio_sample_buffer_.peek()) {
        out_samples[sample_index++] = s->first;
        pts = s->second;
        audio_sample_buffer_.remove();
      } else {
        break;
      }
    }

    return pts;
  }

private:
  constexpr static int kNumChannels = 2;
  constexpr static int kMaxAudioSampleSize = 2048 * kNumChannels;
  constexpr static double kSampleRate = 44100;
  constexpr static int kTimebase = 1000000;
  using SampleType = std::pair<int16_t, int64_t>;
  jnk0le::Ringbuffer<SampleType, kMaxAudioSampleSize> audio_sample_buffer_;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_SAMPLE_DISPATCHER_H
