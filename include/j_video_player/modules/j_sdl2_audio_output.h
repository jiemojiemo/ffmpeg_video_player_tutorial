//
// Created by user on 11/14/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
#include "j_video_player/ffmpeg_utils/ffmpeg_common_utils.h"
#include "j_video_player/modules/j_base_audio_output.h"
#include <SDL.h>

namespace j_video_player {
class SDL2AudioOutput : public BaseAudioOutput {
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

private:
  static void sdl2AudioCallback(void *userdata, Uint8 *stream, int len) {
    auto *audio_output = static_cast<SDL2AudioOutput *>(userdata);
    if (audio_output->getState() == AudioOutputState::kStopped) {
      return;
    }
    auto *short_stream = reinterpret_cast<short int *>(stream);
    const int num_samples_of_stream = len / sizeof(int16_t);

    std::fill_n(short_stream, num_samples_of_stream, 0);

    audio_output->pullAudioSamples(short_stream, num_samples_of_stream);
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
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }

  SDL_AudioDeviceID audio_device_id_{0};
  SDL_AudioSpec audio_spec_{};
  std::atomic<bool> is_sdl2_audio_system_init{false};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SDL2_AUDIO_OUTPUT_H
