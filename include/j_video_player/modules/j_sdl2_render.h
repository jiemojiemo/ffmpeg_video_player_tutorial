//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
#define FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
#include "j_video_player/modules/j_audio_render.h"
#include "j_video_player/modules/j_video_render.h"
#include "j_video_player/utils/simple_fifo.h"
#include "ringbuffer.hpp"
#include <SDL2/SDL.h>
#include <functional>
#include <thread>
#include <vector>

namespace j_video_player {
class SDL2Render : public IAudioRender, public IVideoRender {
public:
  ~SDL2Render() override { cleanup(); }
  void initAudioRender() override {
    initSDL2();
    initAudioDevice();
  }
  void clearAudioCache() override { cleanupFIFO(); }
  int getAudioCacheRemainSize() override {
    return audio_sample_buffer_.readAvailable();
  }
  void renderAudioData(int16_t *data, int nb_samples) override {
    for (; audio_sample_buffer_.writeAvailable() < (size_t)(nb_samples);) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (int i = 0; i < nb_samples; i++) {
      audio_sample_buffer_.insert(data[i]);
    }
  }

  void setAudioCallback(std::function<void(uint8_t *, int)> func) override {
    audio_callback_ = std::move(func);
  }

  void initVideoRender(int video_width, int video_height) override {
    initSDL2();
    initDisplayWindow(video_width, video_height);
  }
  void renderVideoData(AVFrame *frame) override {
    if (is_sdl2_system_init && window_ && renderer_ && texture_) {
      onLoop(frame);
      onRender();
    }
  }
  void uninit() override { cleanup(); }

  void cleanup() {
    cleanupFIFO();
    cleanupSDL();
  }

  void cleanupFIFO() { audio_sample_buffer_.producerClear(); }

  void cleanupSDL() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_sdl2_system_init) {
      if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
      }

      if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
      }

      if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
      }

      if (audio_device_id != 0) {
        SDL_CloseAudioDevice(audio_device_id);
        audio_device_id = 0;
      }

      SDL_Quit();
      is_sdl2_system_init = false;
    }
  }

  size_t getFIFOSize() const { return audio_sample_buffer_.readAvailable(); }

private:
  void initSDL2() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_sdl2_system_init) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return;
      }
    }

    is_sdl2_system_init = true;
  }

  void initDisplayWindow(int video_width, int video_height) {
    if (!is_sdl2_system_init) {
      return;
    }

    // already init
    if (window_ != nullptr) {
      return;
    }

    window_ = SDL_CreateWindow("FFmpeg Video Player", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, video_width,
                               video_height, SDL_WINDOW_OPENGL);
    if (!window_) {
      printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
      exit(-1);
    }

    SDL_GL_SetSwapInterval(1);

    renderer_ = SDL_CreateRenderer(window_, -1, 0);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV,
                                 SDL_TEXTUREACCESS_STREAMING, video_width,
                                 video_height);
  }

  void initAudioDevice() {
    SDL_AudioSpec wanted_specs;
    wanted_specs.freq = 44100;
    wanted_specs.format = AUDIO_S16SYS;
    wanted_specs.channels = 2;
    wanted_specs.silence = 0;
    wanted_specs.samples = 1024;
    wanted_specs.callback = audioCallback;
    wanted_specs.userdata = this;

    audio_device_id = SDL_OpenAudioDevice( // [1]
        NULL, 0, &wanted_specs, &audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (audio_device_id == 0) {
      printf("Failed to open audio: %s", SDL_GetError());
      return;
    }

    SDL_PauseAudioDevice(audio_device_id, 0);
  }

  static void audioCallback(void *userdata, Uint8 *stream, int len) {

    auto *self = (SDL2Render *)(userdata);
    return self->audio_callback_(stream, len);

    //    auto *out_buffer = (int16_t *)(stream);
    //    auto total_need_samples = len / sizeof(int16_t);
    //    auto num_samples_need = total_need_samples;
    //    int sample_index = 0;
    //
    //    std::fill_n(out_buffer, total_need_samples, 0);
    //
    //    auto getSampleFromFIFO = [&]() {
    //      for (; num_samples_need > 0;) {
    //        if (auto s = self->audio_sample_buffer_.peek()) {
    //          out_buffer[sample_index++] = *s;
    //          --num_samples_need;
    //          self->audio_sample_buffer_.remove();
    //        } else {
    //          break;
    //        }
    //      }
    //    };
    //
    //    getSampleFromFIFO();
  }

  void onLoop(AVFrame *pict) {
    SDL_UpdateYUVTexture(
        texture_,          // the texture to update
        nullptr,           // a pointer to the rectangle of pixels to update, or
                           // NULL to update the entire texture
        pict->data[0],     // the raw pixel data for the Y plane
        pict->linesize[0], // the number of bytes between rows of pixel
                           // data for the Y plane
        pict->data[1],     // the raw pixel data for the U plane
        pict->linesize[1], // the number of bytes between rows of pixel
                           // data for the U plane
        pict->data[2],     // the raw pixel data for the V plane
        pict->linesize[2]  // the number of bytes between rows of pixel
                           // data for the V plane
    );
  }

  void onRender() {
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, // the rendering context
                   texture_,  // the source texture
                   NULL,      // the source SDL_Rect structure or NULL for the
                              // entire texture
                   NULL       // the destination SDL_Rect structure or NULL for
                              // the entire rendering target; the texture will
                              // be stretched to fill the given rectangle
    );
    SDL_RenderPresent(renderer_);
  }

  SDL_Window *window_{nullptr};
  SDL_Renderer *renderer_{nullptr};
  SDL_Texture *texture_{nullptr};
  SDL_AudioDeviceID audio_device_id{0};
  SDL_AudioSpec audio_spec;
  std::function<void(uint8_t *, int)> audio_callback_{nullptr};
  constexpr static int kMaxAudioSampleSize = 1024 * 2;

  jnk0le::Ringbuffer<int16_t, kMaxAudioSampleSize> audio_sample_buffer_;

  std::mutex mutex_;
  std::atomic<bool> is_sdl2_system_init{false};
};
} // namespace j_video_player
#endif // FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
