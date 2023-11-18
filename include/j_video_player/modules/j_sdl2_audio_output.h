//
// Created by user on 11/14/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
#include "j_video_player/ffmpeg_utils/ffmpeg_common_utils.h"
#include "j_video_player/modules/j_i_audio_output.h"
#include "j_video_player/utils/simple_fifo.h"
#include <SDL.h>

namespace j_video_player {
class SDL2AudioOutput : public IAudioOutput {
public:
  ~SDL2AudioOutput() override { cleanSDL2(); }
  int prepare(const AudioOutputParameters &params) override {
    if (!params.isValid()) {
      LOGE("Invalid audio output parameters");
      return -1;
    }

    auto ret = initSDL2AudioSystem();
    RETURN_IF_ERROR_LOG(ret, "initSDL2AudioSystem failed");

    ret = initSDL2AudioDevice(params);
    RETURN_IF_ERROR_LOG(ret, "initSDL2AudioDevice failed");

    return 0;
  }
  void attachAudioSource(std::shared_ptr<IAudioSource> source) override {
    source_ = std::move(source);
  }

  void attachResampler(
      std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler) override {
    resampler_ = std::move(resampler);
  }

  void attachAVSyncClock(std::shared_ptr<utils::ClockManager> clock) override {
    clock_ = std::move(clock);
  }

  int play() override {
    if (source_ == nullptr) {
      LOGE("source is null, can't play. Please attach source first");
      return -1;
    }

    if (audio_device_id_ == 0) {
      LOGE("audio_device_id is 0, can't play. Please prepare first");
      return -1;
    }
    SDL_PauseAudioDevice(audio_device_id_, 0);
    state_ = AudioOutputState::kPlaying;
    return 0;
  }
  int stop() override {
    if (audio_device_id_ == 0) {
      LOGE("audio_device_id is 0, can't stop.");
      return -1;
    }
    SDL_PauseAudioDevice(audio_device_id_, 1);
    state_ = AudioOutputState::kStopped;
    return 0;
  }
  AudioOutputState getState() const override { return state_; }

private:
  static void sdl2AudioCallback(void *userdata, Uint8 *stream, int len) {
    auto *audio_output = static_cast<SDL2AudioOutput *>(userdata);
    auto *short_stream = reinterpret_cast<short int *>(stream);
    const int num_samples_of_stream = len / sizeof(int16_t);

    std::fill_n(short_stream, num_samples_of_stream, 0);

    audio_output->pullAudioSamples(short_stream, num_samples_of_stream);
  }

  void pullAudioSamples(int16_t *stream, int num_samples) {
    if (source_ == nullptr) {
      return;
    }

    int output_index = 0;
    int16_t sample;
    int64_t pts{-1};
    for (; output_index < num_samples;) {
      auto ok = sample_fifo_.pop(sample);
      if (ok) {
        if (pts == -1) {
          pts = last_frame_pts_;
        }
        stream[output_index++] = sample;
      } else {
        // deque a frame and resample it
        // then push to sample_fifo_
        auto frame = source_->dequeueAudioFrame();
        if (frame == nullptr) {
          break;
        } else {
          resampleAndPushToFIFO(frame);
        }
      }
    }

    // update audio clock
    if (clock_) {
      double pts_d = static_cast<double>(pts) / AV_TIME_BASE;
      clock_->setAudioClock(pts_d);
    }
  }

  void resampleAndPushToFIFO(const std::shared_ptr<Frame> &frame) {
    last_frame_pts_ = frame->pts();

    if (resampler_) {
      auto *f = frame->f;
      auto num_samples_out_per_channel =
          resampler_->convert((const uint8_t **)f->data, f->nb_samples);
      auto num_total_samples =
          num_samples_out_per_channel * resampler_->out_num_channels();
      auto *int16_resample_data =
          reinterpret_cast<int16_t *>(resampler_->resample_data[0]);
      for (auto i = 0; i < num_total_samples; ++i) {
        sample_fifo_.push(std::move(int16_resample_data[i]));
      }
    } else {
      auto *f = frame->f;
      auto num_total_samples = f->nb_samples * f->channels;
      for (auto i = 0; i < num_total_samples; ++i) {
        sample_fifo_.push(std::move(f->data[0][i]));
      }
    }
  }

  int initSDL2AudioSystem() {
    if (!is_sdl2_audio_system_init) {
      if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
      }
    }
    is_sdl2_audio_system_init = true;
    return 0;
  }

  int initSDL2AudioDevice(const AudioOutputParameters &params) {
    SDL_AudioSpec wanted_specs;
    wanted_specs.freq = params.sample_rate;
    wanted_specs.format = AUDIO_S16SYS;
    wanted_specs.channels = params.channels;
    wanted_specs.silence = 0;
    wanted_specs.samples = params.num_frames_of_buffer;
    wanted_specs.callback = SDL2AudioOutput::sdl2AudioCallback;
    wanted_specs.userdata = this;

    audio_device_id_ = SDL_OpenAudioDevice( // [1]
        NULL, 0, &wanted_specs, &audio_spec_, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (audio_device_id_ == 0) {
      printf("Failed to open audio: %s", SDL_GetError());
      return -1;
    }
    return 0;
  }

  void cleanSDL2() {
    if (audio_device_id_ != 0) {
      SDL_CloseAudioDevice(audio_device_id_);
      audio_device_id_ = 0;
    }
    if (is_sdl2_audio_system_init) {
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
  }

  SDL_AudioDeviceID audio_device_id_{0};
  SDL_AudioSpec audio_spec_{};
  std::atomic<bool> is_sdl2_audio_system_init{false};
  std::shared_ptr<IAudioSource> source_;
  std::atomic<AudioOutputState> state_{AudioOutputState::kIdle};
  std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler_{nullptr};
  std::shared_ptr<utils::ClockManager> clock_{nullptr};
  int64_t last_frame_pts_{-1};
  constexpr static int kFIFOSize = 44100;
  utils::SimpleFIFO<int16_t> sample_fifo_{kFIFOSize};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
