//
// Created by user on 1/22/23.
//
#include "ffmpeg_utils/ffmpeg_decode_engine.h"
#include "utils/av_synchronizer.h"
#include "utils/clock_manager.h"
#include <SDL2/SDL.h>
#include <cassert>

#include <cmath>
#include <queue>
#include <stdio.h>
using namespace ffmpeg_utils;
using namespace utils;
using namespace std::literals;

#define FF_REFRESH_EVENT (SDL_USEREVENT)

class SDLApp {
public:
  ~SDLApp() { onCleanup(); }

  int onInit() {
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (ret != 0) {
      printf("Could not initialize SDL - %s\n.", SDL_GetError());
      return -1;
    }

    return 0;
  }

  int onPrepareToPlayVideo(int video_width, int video_height) {
    screen = SDL_CreateWindow("SDL Video Player", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, video_width / 2,
                              video_height / 2,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!screen) {
      printf("SDL: could not set video mode - exiting.\n");
      return -1;
    }

    SDL_GL_SetSwapInterval(1);

    renderer = SDL_CreateRenderer(screen, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                SDL_TEXTUREACCESS_STREAMING, video_width,
                                video_height);

    return 0;
  }

  int onOpenAudioDevice(SDL_AudioSpec &wanted_specs, SDL_AudioSpec &specs) {
    audio_device_id = SDL_OpenAudioDevice( // [1]
        NULL, 0, &wanted_specs, &specs, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (audio_device_id == 0) {
      printf("Failed to open audio device: %s.\n", SDL_GetError());
      return -1;
    }

    audio_spec = specs;
    audio_sample_fifo =
        std::make_unique<AudioSampleFIFO>(specs.samples * specs.channels);

    return 0;
  }

  void pauseAudio(int pause_on) const {
    SDL_PauseAudioDevice(audio_device_id, pause_on);
  }

  void onEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_LEFT: {
        doSeekRelative(-5.0);
        break;
      }

      case SDLK_RIGHT: {
        doSeekRelative(5.0);
        break;
      }

      case SDLK_DOWN: {
        doSeekRelative(-60.0);
        break;
      }

      case SDLK_UP: {
        doSeekRelative(60.0);
        break;
      }
      case SDLK_SPACE: {
        pause();
        break;
      }
      }
      break;
    }
    case SDL_QUIT: {
      running = false;
      break;
    }
    case FF_REFRESH_EVENT: {
      videoRefreshTimer(event.user.data1);
    }
    default: {
      break;
    }
    }
  }

  void doSeekRelative(double incr) {
    assert(decode_engine != nullptr);
    auto audio_clock = clock.getAudioClock();
    auto target_pos = audio_clock + incr;
    decode_engine->seek(target_pos);
  }

  void pause() {
    paused = !paused;
    if (paused) {
      SDL_PauseAudioDevice(audio_device_id, 1);
    } else {
      SDL_PauseAudioDevice(audio_device_id, 0);
    }
  }

  void onLoop(AVFrame *pict) {
    SDL_UpdateYUVTexture(
        texture,           // the texture to update
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
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, // the rendering context
                   texture,  // the source texture
                   NULL,     // the source SDL_Rect structure or NULL for the
                             // entire texture
                   NULL      // the destination SDL_Rect structure or NULL for
                             // the entire rendering target; the texture will
                             // be stretched to fill the given rectangle
    );
    SDL_RenderPresent(renderer);
  }

  void onCleanup() {
    if (texture) {
      SDL_DestroyTexture(texture);
      texture = nullptr;
    }

    if (renderer) {
      SDL_DestroyRenderer(renderer);
      renderer = nullptr;
    }

    if (screen) {
      SDL_DestroyWindow(screen);
      screen = nullptr;
    }

    if (audio_device_id != 0) {
      SDL_CloseAudioDevice(audio_device_id);
      audio_device_id = 0;
    }

    SDL_Quit();
  }

  static Uint32 sdlRefreshTimerCallback(Uint32 interval, void *param) {
    (void)(interval);

    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = param;

    SDL_PushEvent(&event);

    return 0;
  }

  static void scheduleRefresh(FFMPEGDecodeEngine *engine, int delay) {
    SDL_AddTimer(delay, sdlRefreshTimerCallback, engine);
  }

  void videoRefreshTimer(void *userdata) {
    auto *engine = (FFMPEGDecodeEngine *)(userdata);

    // display video frame
    if (screen == nullptr) {
      onPrepareToPlayVideo(engine->video_codec_ctx->width,
                           engine->video_codec_ctx->height);
    }

    if (paused) {
      scheduleRefresh(engine, 1);
      return;
    }

    auto *video_frame = engine->pullVideoFrame();
    ON_SCOPE_EXIT([&video_frame] {
      if (video_frame != nullptr) {
        av_frame_unref(video_frame);
        av_frame_free(&video_frame);
      }
    });

    if (video_frame == nullptr) {
      scheduleRefresh(engine, 1);
      return;
    } else {
      updateVideoClockByFramePts(video_frame->pts,
                                 engine->video_stream->time_base);

      // compute target delay
      auto real_delay_ms = (int)(av_sync.computeTargetDelay(clock) * 1000);

      scheduleRefresh(engine, real_delay_ms);
      printf("Next Scheduled Refresh:\t%dms\n\n", real_delay_ms);

      // render picture
      onLoop(video_frame);
      onRender();
    }
  }

  void updateAudioClockByFramePts(int64_t frame_pts,
                                  const AVRational &time_base) {
    clock.setAudioClock(static_cast<double>(frame_pts) * av_q2d(time_base) +
                        audio_hw_delay);
  }

  void updateAudioClock(double frame_pts) {
    clock.setAudioClock(frame_pts + audio_hw_delay);
  }

  void updateVideoClockByFramePts(int64_t frame_pts,
                                  const AVRational &time_base) {
    double video_pts = static_cast<double>(frame_pts) * av_q2d(time_base);
    clock.setVideoClock(video_pts);
  }

  std::atomic<bool> running = true;
  FFMPEGDecodeEngine *decode_engine{nullptr};
  AVSynchronizer av_sync;
  ClockManager clock;
  SDL_AudioSpec audio_spec;
  double audio_hw_delay = {0};

  using AudioSampleFIFO = utils::SimpleFIFO<int16_t>;
  std::unique_ptr<AudioSampleFIFO> audio_sample_fifo;

private:
  SDL_Window *screen{nullptr};
  SDL_Renderer *renderer{nullptr};
  SDL_Texture *texture{nullptr};
  SDL_AudioDeviceID audio_device_id{0};
  bool paused = false;
};

void audioCallback(void *userdata, Uint8 *stream, int len) {
  auto *sdl_app = (SDLApp *)(userdata);
  assert(sdl_app->decode_engine != nullptr);

  const int num_samples_of_stream = len / sizeof(int16_t);
  const int num_samples_per_channel =
      num_samples_of_stream / sdl_app->audio_spec.channels;

  auto [num_sample_out, last_pts] = sdl_app->decode_engine->pullAudioSamples(
      num_samples_per_channel, sdl_app->audio_spec.channels,
      (int16_t *)(stream));
  sdl_app->updateAudioClock(last_pts);
}

void printHelpMenu() {
  printf("Invalid arguments.\n\n");
  printf("Usage: ./tutorial03 <filename> <max-frames-to-decode>\n\n");
  printf("e.g: ./tutorial03 /home/rambodrahmani/Videos/Labrinth-Jealous.mp4 "
         "200\n");
}

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    // wrong arguments, print help menu
    printHelpMenu();

    // exit with error
    return -1;
  }

  std::string infile = argv[1];

  FFMPEGDecodeEngine engine;
  int ret = engine.openFile(infile);
  RETURN_IF_ERROR_LOG(ret, "engine open file failed\n");

  SDLApp sdl_app;
  sdl_app.decode_engine = &engine;
  ret = sdl_app.onInit();
  RETURN_IF_ERROR_LOG(ret, "sdl init failed\n");

  SDL_AudioSpec wanted_specs;
  SDL_AudioSpec specs;
  wanted_specs.freq = engine.audio_codec_ctx->sample_rate;
  wanted_specs.format = AUDIO_S16SYS;
  wanted_specs.channels = engine.audio_codec_ctx->channels;
  wanted_specs.silence = 0;
  wanted_specs.samples = 1024;
  wanted_specs.callback = audioCallback;
  wanted_specs.userdata = &sdl_app;

  ret = sdl_app.onOpenAudioDevice(wanted_specs, specs);
  RETURN_IF_ERROR_LOG(ret, "sdl open audio device failed\n");

  AudioOutputConfig audio_config;
  audio_config.sample_rate = specs.freq,
  audio_config.num_channels = specs.channels,
  audio_config.channel_layout =
      (specs.channels == 1) ? (AV_CH_LAYOUT_MONO) : (AV_CH_LAYOUT_STEREO);
  VideoOutputConfig video_config = engine.getInputFileVideoConfig();
  video_config.pixel_format = AV_PIX_FMT_YUV420P;
  engine.start(&video_config, &audio_config);

  auto audio_hw_buffer_frame_size =
      specs.size / sizeof(int16_t) / specs.channels;
  sdl_app.audio_hw_delay = audio_hw_buffer_frame_size / (double)(specs.freq);

  // start to play audio
  sdl_app.pauseAudio(0);

  SDLApp::scheduleRefresh(&engine, 39);

  SDL_Event event;
  for (; sdl_app.running;) {
    SDL_WaitEvent(&event);
    sdl_app.onEvent(event);
  }

  engine.stop();
  sdl_app.onCleanup();
  return 0;
}