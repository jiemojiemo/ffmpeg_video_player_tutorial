//
// Created by user on 1/18/23.
//
#include "j_video_player/ffmpeg_utils/ffmpeg_decoder_context.h"
#include <SDL2/SDL.h>

#include <queue>
#include <stdio.h>
#include <thread>
using namespace ffmpeg_utils;

class SDLApp {
public:
  ~SDLApp() { onCleanup(); }

  int onInit() {
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
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

    return 0;
  }

  void pauseAudio(int pause_on) const {
    SDL_PauseAudioDevice(audio_device_id, pause_on);
  }

  void onEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_QUIT: {
      running = false;
      break;
    }
    default: {
      break;
    }
    }
  }

  void onLoop(AVFrame *pict) {
    SDL_UpdateYUVTexture(
        texture, // the texture to update
        nullptr, // a pointer to the rectangle of pixels to update, or NULL to
                 // update the entire texture
        pict->data[0],     // the raw pixel data for the Y plane
        pict->linesize[0], // the number of bytes between rows of pixel data for
                           // the Y plane
        pict->data[1],     // the raw pixel data for the U plane
        pict->linesize[1], // the number of bytes between rows of pixel data for
                           // the U plane
        pict->data[2],     // the raw pixel data for the V plane
        pict->linesize[2]  // the number of bytes between rows of pixel data for
                           // the V plane
    );
  }

  void onRender(double sleep_time_s) {
    SDL_Delay((1000 * sleep_time_s) - 10);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(
        renderer, // the rendering context
        texture,  // the source texture
        NULL, // the source SDL_Rect structure or NULL for the entire texture
        NULL  // the destination SDL_Rect structure or NULL for the entire
              // rendering target; the texture will be stretched to fill the
              // given rectangle
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

  bool running = true;

private:
  SDL_Window *screen{nullptr};
  SDL_Renderer *renderer{nullptr};
  SDL_Texture *texture{nullptr};
  SDL_AudioDeviceID audio_device_id{0};
};

void audioCallback(void *userdata, Uint8 *stream, int len) {
  auto *ctx = (DecoderContext *)(userdata);
  auto &audio_packet_queue = ctx->audio_packet_queue;
  auto *sample_fifo = ctx->audio_sample_fifo.get();
  auto &audio_codec = ctx->audio_codec;
  auto &resampler = ctx->audio_resampler;
  int num_channels = audio_codec.getCodecContext()->channels;

  const int num_samples_of_stream = len / sizeof(int16_t);
  int num_samples_need = num_samples_of_stream;
  int sample_index = 0;
  auto *int16_stream = reinterpret_cast<int16_t *>(stream);
  int16_t s = 0;
  AVFrame *frame = av_frame_alloc();
  if (frame == nullptr) {
    printf("Could not allocate frame.\n");
    return;
  }

  auto getSampleFromFIFO = [&]() {
    for (; num_samples_need > 0;) {
      if (sample_fifo->pop(s)) {
        int16_stream[sample_index++] = s;
        --num_samples_need;
      } else {
        break;
      }
    }
  };

  auto resampleAudioAndPushToFIFO = [&]() {
    int num_samples_out_per_channel =
        resampler.convert((const uint8_t **)frame->data, frame->nb_samples);
    int num_samples_total = num_samples_out_per_channel * num_channels;
    auto *int16_resample_data =
        reinterpret_cast<int16_t *>(resampler.resample_data[0]);
    for (int i = 0; i < num_samples_total; ++i) {
      sample_fifo->push(std::move(int16_resample_data[i]));
    }
  };

  for (;;) {
    // try to get samples from fifo
    getSampleFromFIFO();

    if (num_samples_need <= 0) {
      goto end;
    } else {
      // if samples in fifo not enough, decode packet and push samples to fifo
      auto *packet = audio_packet_queue.pop();

      // there is no packet in queue, just fill remain samples to zero
      if (packet == nullptr) {
        std::fill_n(int16_stream, num_samples_need, 0);
        goto end;
      } else {
        // try to decode audio packet
        int ret = audio_codec.sendPacketToCodec(packet);
        av_packet_unref(packet);
        av_packet_free(&packet);
        if (ret < 0) {
          printf("Error sending packet for decoding %s.\n", av_err2str(ret));
          goto end;
        }

        while (true) {
          ret = audio_codec.receiveFrame(frame);
          if (ret == AVERROR(EINVAL) || ret == AVERROR_EOF ||
              ret == AVERROR(EAGAIN)) {
            // EOF exit loop
            av_frame_unref(frame);
            break;
          } else if (ret < 0) {
            av_frame_unref(frame);
            printf("Error while decoding.\n");
            return;
          }

          resampleAudioAndPushToFIFO();
        }
      }
    }
  }

end:
  av_frame_unref(frame);
  av_frame_free(&frame);
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
  int max_frames_to_decode = std::stoi(argv[2]);

  // prepare ffmpeg for decoding
  DecoderContext decoder_ctx;
  int ret = decoder_ctx.prepare(infile);
  RETURN_IF_ERROR(ret);

  // prepare sdl
  auto video_width = decoder_ctx.video_codec_ctx->width;
  auto video_height = decoder_ctx.video_codec_ctx->height;

  SDLApp sdl_app;
  ret = sdl_app.onInit();
  RETURN_IF_ERROR_LOG(ret, "sdl init failed\n");

  SDL_AudioSpec wanted_specs;
  SDL_AudioSpec specs;
  wanted_specs.freq = decoder_ctx.audio_codec_ctx->sample_rate;
  wanted_specs.format = AUDIO_S16SYS;
  wanted_specs.channels = decoder_ctx.audio_codec_ctx->channels;
  wanted_specs.silence = 0;
  wanted_specs.samples = DecoderContext::SDL_AUDIO_BUFFER_SIZE;
  wanted_specs.callback = audioCallback;
  wanted_specs.userdata = &decoder_ctx;

  ret = sdl_app.onOpenAudioDevice(wanted_specs, specs);
  RETURN_IF_ERROR_LOG(ret, "sdl open audio device failed\n");

  ret = sdl_app.onPrepareToPlayVideo(video_width, video_height);
  RETURN_IF_ERROR_LOG(ret, "sdl prepare window to play failed\n");

  // start to play audio
  sdl_app.pauseAudio(0);

  // start to play video
  AVFrame *frame = av_frame_alloc();
  if (frame == nullptr) {
    printf("Could not allocate frame.\n");
    return -1;
  }

  int i = 0;
  AVPacket *packet{nullptr};
  AVFrame *pict{nullptr};
  int convert_output_width{0};
  double fps = av_q2d(decoder_ctx.video_stream->r_frame_rate);
  double sleep_time = 1.0 / fps;
  SDL_Event event;
  for (; sdl_app.running;) {
    if (i >= max_frames_to_decode) {
      sdl_app.running = false;
      break;
    }

    std::tie(ret, packet) = decoder_ctx.demuxer.readPacket();
    if (packet->stream_index == decoder_ctx.video_stream_index) {
      ret = decoder_ctx.video_codec.sendPacketToCodec(packet);
      if (ret < 0) {
        av_packet_unref(packet);
        printf("Error sending packet for video decoding %s.\n",
               av_err2str(ret));
        return -1;
      }

      while (ret >= 0) {
        ret = decoder_ctx.video_codec.receiveFrame(frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ||
            ret == AVERROR(EAGAIN)) {
          // EOF exit loop
          av_frame_unref(frame);
          break;
        } else if (ret < 0) {
          av_frame_unref(frame);
          printf("Error while decoding.\n");
          return -1;
        }

        std::tie(convert_output_width, pict) =
            decoder_ctx.img_conv.convert(frame);

        sdl_app.onLoop(pict);
        sdl_app.onRender(sleep_time);
      }

      SDL_PollEvent(&event);
      sdl_app.onEvent(event);
    } else if (packet->stream_index == decoder_ctx.audio_stream_index) {
      decoder_ctx.audio_packet_queue.cloneAndPush(packet);
    }

    av_packet_unref(packet);
  }

  av_frame_unref(frame);
  av_frame_free(&frame);
  sdl_app.onCleanup();
  return 0;
}