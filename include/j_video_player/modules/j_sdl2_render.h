//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
#define FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
#include "j_video_player/modules/j_audio_render.h"
#include "j_video_player/modules/j_video_render.h"
#include "j_video_player/utils/simple_fifo.h"
#include <SDL2/SDL.h>
#include <vector>
#include <mutex>

namespace j_video_player {
class SDL2Render : public IAudioRender, public IVideoRender {
public:
  ~SDL2Render() override { cleanup(); }
  void initAudioRender() override {
    initSDL2();
    initAudioDevice();
    initAudioFIFO();
  }
  void clearAudioCache() override {}
  void renderAudioData(int16_t *data, int nb_samples) override {
    if (audio_sample_fifo_) {
      for (int i = 0; i < nb_samples; i++) {
        audio_sample_fifo_->push(std::move(data[i]));
      }
    }
  }

  void initVideoRender(int video_width, int video_height) override {
    initSDL2();
    initDisplayWindow(video_width, video_height);
  }
  void renderVideoData(AVFrame *frame) override {
    if (is_init_) {
      onLoop(frame);
      onRender();
    }
  }
  void uninit() override { cleanup(); }

  void cleanup() {
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
  }

  size_t getFIFOSize() const { return audio_sample_fifo_->size(); }

private:
  void initSDL2() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_init_) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return;
      }
    }

    is_init_ = true;
  }

  void initDisplayWindow(int video_width, int video_height) {
    if (!is_init_) {
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
  }

  void initAudioFIFO() {
    audio_sample_fifo_ = std::make_unique<AudioSampleFIFO>(audio_spec.samples *
                                                           audio_spec.channels);
  }

  static void audioCallback(void *userdata, Uint8 *stream, int len) {
    auto* self = (SDL2Render*)(userdata);
    auto* out_buffer = (int16_t*)(stream);
    auto total_need_samples = len / sizeof(int16_t);
    auto num_samples_need = total_need_samples;
    int sample_index = 0;
    int16_t s = 0;

    auto getSampleFromFIFO = [&]() {
      for (; num_samples_need > 0;) {
        if (self->audio_sample_fifo_->pop(s)) {
          out_buffer[sample_index++] = s;
          --num_samples_need;
        } else {
          break;
        }
      }
    };

    getSampleFromFIFO();
    if(num_samples_need > 0){
      auto remain = total_need_samples - num_samples_need;
      std::fill_n(out_buffer + sample_index, remain, 0);
    }
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
  using AudioSampleFIFO = utils::SimpleFIFO<int16_t>;
  std::unique_ptr<AudioSampleFIFO> audio_sample_fifo_;

  std::mutex mutex_;
  std::atomic<bool> is_init_{false};
};
} // namespace j_video_player
#endif // FFMPEG_VIDEO_PLAYER_J_SDL2_RENDER_H
