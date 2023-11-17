//
// Created by user on 11/13/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SDL2_VIDEO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_SDL2_VIDEO_OUTPUT_H
#include "j_video_player/ffmpeg_utils/ffmpeg_common_utils.h"
#include "j_video_player/modules/j_base_video_output.h"
#include <SDL2/SDL.h>
namespace j_video_player {
class SDL2VideoOutput : public BaseVideoOutput {
public:
  ~SDL2VideoOutput() override {
    BaseVideoOutput::cleanup();
    cleanSDL2();
  }
  int prepare(const VideoOutputParameters &parameters) override {
    if (parameters.width <= 0 || parameters.height <= 0) {
      LOGE("invalid width or height");
      return -1;
    }

    auto ret = initSDL2System();
    RETURN_IF_ERROR_LOG(ret, "initSDL2System failed");

    ret = initDisplayWindow(parameters);
    RETURN_IF_ERROR_LOG(ret, "initDisplayWindow failed");

    return 0;
  }

protected:
  int drawFrame(std::shared_ptr<Frame> frame) override {
    AVFrame *pict = frame->f;
    SDL_UpdateYUVTexture(texture_, nullptr,

                         pict->data[0], pict->linesize[0],

                         pict->data[1], pict->linesize[1],

                         pict->data[2], pict->linesize[2]);

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

    return 0;
  }

private:
  int initSDL2System() {
    if (!is_sdl2_system_init) {
      if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
      }
    }
    is_sdl2_system_init = true;
    return 0;
  }

  int initDisplayWindow(const VideoOutputParameters &parameters) {
    if (!is_sdl2_system_init) {
      LOGE("SDL2 system is not initialized");
      return -1;
    }

    if (window_ != nullptr) {
      LOGW("window_ is not null, please call cleanup first");
      return 0;
    }

    window_ = SDL_CreateWindow("FFmpeg Video Player", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, parameters.width,
                               parameters.height, SDL_WINDOW_OPENGL);
    if (!window_) {
      LOGE("SDL: could not create window - exiting:%s\n", SDL_GetError());
      return -1;
    }

    SDL_GL_SetSwapInterval(1);
    if(parameters.pixel_format != AVPixelFormat::AV_PIX_FMT_YUV420P){
      LOGE("only support AV_PIX_FMT_YUV420P");
      return -1;
    }

    // TODO: support more pixel format
    auto sdl2_format = SDL_PIXELFORMAT_IYUV;
    renderer_ = SDL_CreateRenderer(window_, -1, 0);
    texture_ = SDL_CreateTexture(renderer_, sdl2_format,
                                 SDL_TEXTUREACCESS_STREAMING, parameters.width,
                                 parameters.height);

    return 0;
  }

  void cleanSDL2() {
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

      SDL_QuitSubSystem(SDL_INIT_VIDEO);
      is_sdl2_system_init = false;
    }
  }

  std::atomic<bool> is_sdl2_system_init{false};
  SDL_Window *window_{nullptr};
  SDL_Renderer *renderer_{nullptr};
  SDL_Texture *texture_{nullptr};
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SDL2_VIDEO_OUTPUT_H
