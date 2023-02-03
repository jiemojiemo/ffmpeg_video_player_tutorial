//
// Created by user on 1/22/23.
//
#include "ffmpeg_utils/ffmpeg_decoder_context.h"
#include "utils/clock.h"
#include "utils/scope_guard.h"
#include <SDL2/SDL.h>

#include <cmath>
#include <queue>
#include <stdio.h>
#include <thread>
using namespace ffmpeg_utils;
using namespace utils;
using namespace std::literals;

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define FF_REFRESH_EVENT (SDL_USEREVENT)

class PlayContext {
public:
  DecoderContext *decode_ctx{nullptr};
  std::atomic<int> num_frames_played = 0;
  int max_frames_to_play = 0;
  int audio_hw_buffer_size{0};
  int audio_hw_buffer_frame_size{0};
  double audio_hw_delay{0};
  Clock audio_clock_t;
  Clock video_clock_t;

  void setClockAt(Clock &clock, double pts, double time) {
    clock.pts = pts;
    clock.last_updated = time;
  }

  void setClock(Clock &clock, double pts) {
    setClockAt(clock, pts, (double)av_gettime() / 1000000.0);
  }

  double getAudioClock() const { return getClock(audio_clock_t); }

  double getVideoClock() const { return getClock(video_clock_t); }

  double getClock(const Clock &c) const {
    double time = (double)av_gettime() / 1000000.0;
    return c.pts + time - c.last_updated;
  }

  int videoSync(AVFrame *video_frame) {
    auto video_timebase_d = av_q2d(decode_ctx->video_stream->time_base);

    auto pts = video_frame->pts * video_timebase_d;
    setClock(video_clock_t, pts);

    auto pts_delay = pts - video_clock_t.pre_pts;
    printf("PTS Delay:\t\t\t\t%lf\n", pts_delay);
    // if the obtained delay is incorrect
    if (pts_delay <= 0 || pts_delay >= 1.0) {
      // use the previously calculated delay
      pts_delay = video_clock_t.pre_frame_delay;
    }
    printf("Corrected PTS Delay:\t%f\n", pts_delay);

    // save delay information for the next time
    video_clock_t.pre_pts = pts;
    video_clock_t.pre_frame_delay = pts_delay;

    auto audio_ref_clock = getAudioClock();
    auto video_clock = getVideoClock();
    auto diff = video_clock - audio_ref_clock;
    printf("Audio Ref Clock:\t\t%lf\n", audio_ref_clock);
    printf("Audio Video Delay:\t\t%lf\n", diff);

    auto sync_threshold = std::max(pts_delay, AV_SYNC_THRESHOLD);
    printf("Sync Threshold:\t\t\t%lf\n", sync_threshold);

    if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
      if (diff <= -sync_threshold) {
        pts_delay = 0;
      } else if (diff >= sync_threshold) {
        pts_delay = 2 * pts_delay; // [2]
      }
    }

    printf("Corrected PTS delay:\t%lf\n", pts_delay);

    return (int)std::round(pts_delay * 1000);
  }
};

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
    case FF_REFRESH_EVENT: {
      videoRefreshTimer(event.user.data1);
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

  void onRender() {
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

  static Uint32 sdlRefreshTimerCallback(Uint32 interval, void *param) {
    (void)(interval);

    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = param;

    SDL_PushEvent(&event);

    return 0;
  }

  static void scheduleRefresh(PlayContext *context, int delay) {
    SDL_AddTimer(delay, sdlRefreshTimerCallback, context);
  }

  void videoRefreshTimer(void *userdata) {
    auto *ctx = (PlayContext *)(userdata);
    auto *decoder_ctx = ctx->decode_ctx;

    // display video frame
    if (screen == nullptr) {
      onPrepareToPlayVideo(decoder_ctx->video_codec_ctx->width,
                           decoder_ctx->video_codec_ctx->height);
    }

    auto *video_frame = decoder_ctx->video_frame_sync_que.tryPop();
    ON_SCOPE_EXIT([&video_frame] {
      if (video_frame != nullptr) {
        av_frame_unref(video_frame);
        av_frame_free(&video_frame);
      }
    });

    if (video_frame == nullptr) {
      scheduleRefresh(ctx, 1);
    } else {
      auto real_delay = ctx->videoSync(video_frame);

      scheduleRefresh(ctx, real_delay);

      printf("Next Scheduled Refresh:\t%dms\n\n", real_delay);

      // video format convert
      auto [convert_output_width, pict] =
          decoder_ctx->img_conv.convert(video_frame);

      // render picture
      onLoop(pict);
      onRender();

      if (ctx->num_frames_played++ > ctx->max_frames_to_play) {
        running = false;
      }
    }
  }

  int videoSync(PlayContext *ctx, AVFrame *video_frame) {
    auto video_timebase_d = av_q2d(ctx->decode_ctx->video_stream->time_base);

    auto pts = video_frame->pts * video_timebase_d;
    ctx->setClock(ctx->video_clock_t, pts);

    auto pts_delay = pts - ctx->video_clock_t.pre_pts;
    printf("PTS Delay:\t\t\t\t%lf\n", pts_delay);
    // if the obtained delay is incorrect
    if (pts_delay <= 0 || pts_delay >= 1.0) {
      // use the previously calculated delay
      pts_delay = ctx->video_clock_t.pre_frame_delay;
    }
    printf("Corrected PTS Delay:\t%f\n", pts_delay);

    // save delay information for the next time
    ctx->video_clock_t.pre_pts = pts;
    ctx->video_clock_t.pre_frame_delay = pts_delay;

    auto audio_ref_clock = ctx->getAudioClock();
    auto video_clock = ctx->getVideoClock();
    auto diff = video_clock - audio_ref_clock;
    printf("Audio Ref Clock:\t\t%lf\n", audio_ref_clock);
    printf("Audio Video Delay:\t\t%lf\n", diff);

    auto sync_threshold = std::max(pts_delay, AV_SYNC_THRESHOLD);
    printf("Sync Threshold:\t\t\t%lf\n", sync_threshold);

    if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
      if (diff <= -sync_threshold) {
        pts_delay = std::max(0.0, pts_delay + diff);
      } else if (diff >= sync_threshold) {
        pts_delay = 2 * pts_delay; // [2]
      }
    }

    printf("Corrected PTS delay:\t%lf\n", pts_delay);

    return (int)std::round(pts_delay * 1000);
  }

  std::atomic<bool> running = true;

private:
  SDL_Window *screen{nullptr};
  SDL_Renderer *renderer{nullptr};
  SDL_Texture *texture{nullptr};
  SDL_AudioDeviceID audio_device_id{0};
};

void audioCallback(void *userdata, Uint8 *stream, int len) {
  auto *play_ctx = (PlayContext *)(userdata);
  auto *decode_ctx = play_ctx->decode_ctx;
  auto &audio_frame_queue = decode_ctx->audio_frame_sync_que;
  auto *sample_fifo = decode_ctx->audio_sample_fifo.get();
  auto &audio_codec = decode_ctx->audio_codec;
  auto *audio_stream = decode_ctx->audio_stream;
  auto &resampler = decode_ctx->audio_resampler;
  int num_channels = audio_codec.getCodecContext()->channels;

  const int num_samples_of_stream = len / sizeof(int16_t);
  int num_samples_need = num_samples_of_stream;
  int sample_index = 0;
  auto *int16_stream = reinterpret_cast<int16_t *>(stream);
  int16_t s = 0;

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

  auto resampleAudioAndPushToFIFO = [&](AVFrame *frame) {
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
      break;
    } else {
      // if samples in fifo not enough, get frame and push samples to fifo
      auto *frame = audio_frame_queue.tryPop();
      ON_SCOPE_EXIT([&frame] {
        if (frame != nullptr) {
          av_frame_unref(frame);
          av_frame_free(&frame);
        }
      });

      // there is no frame in queue, just fill remain samples to zero
      if (frame == nullptr) {
        printf("no audio frame, set zeros\n");
        std::fill_n(int16_stream, num_samples_need, 0);
        return;
      } else {
        // update audio block
        play_ctx->setClock(play_ctx->audio_clock_t,
                           frame->pts * av_q2d(audio_stream->time_base));
        resampleAudioAndPushToFIFO(frame);
      }
    }
  }
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
  PlayContext ctx;
  ctx.decode_ctx = &decoder_ctx;
  ctx.max_frames_to_play = max_frames_to_decode;

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
  wanted_specs.userdata = &ctx;

  ret = sdl_app.onOpenAudioDevice(wanted_specs, specs);
  RETURN_IF_ERROR_LOG(ret, "sdl open audio device failed\n");

  ctx.audio_hw_buffer_size = specs.size;
  ctx.audio_hw_buffer_frame_size =
      specs.size / sizeof(int16_t) / decoder_ctx.audio_codec_ctx->channels;
  ctx.audio_hw_delay = ctx.audio_hw_buffer_frame_size *
                       av_q2d(decoder_ctx.audio_stream->time_base);

  // start to play audio
  sdl_app.pauseAudio(0);

  SDLApp::scheduleRefresh(&ctx, 39);

  std::thread demux_thread([&]() {
    AVPacket *packet{nullptr};

    for (; sdl_app.running;) {

      // sleep if packet size in queue is very large
      if (decoder_ctx.video_packet_sync_que.totalPacketSize() >=
              DecoderContext::MAX_VIDEOQ_SIZE ||
          decoder_ctx.audio_packet_sync_que.totalPacketSize() >=
              DecoderContext::MAX_AUDIOQ_SIZE) {
        std::this_thread::sleep_for(10ms);
        continue;
      }

      std::tie(ret, packet) = decoder_ctx.demuxer.readPacket();
      ON_SCOPE_EXIT([&packet] { av_packet_unref(packet); });

      // read end of file, just exit this thread
      if (ret == AVERROR_EOF || packet == nullptr) {
        sdl_app.running = false;
        break;
      }

      if (packet->stream_index == decoder_ctx.video_stream_index) {
        decoder_ctx.video_packet_sync_que.tryPush(packet);
      } else if (packet->stream_index == decoder_ctx.audio_stream_index) {
        decoder_ctx.audio_packet_sync_que.tryPush(packet);
      }
    }
  });

  auto decodePacketAndPushToFrameQueue =
      [](WaitablePacketQueue &packet_queue, FFMPEGCodec &codec,
         AVFrame *out_frame, WaitableFrameQueue &out_frame_queue) {
        auto *pkt = packet_queue.waitAndPop();
        ON_SCOPE_EXIT([&pkt] {
          if (pkt != nullptr) {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
          }
        });

        int ret = codec.sendPacketToCodec(pkt);
        if (ret < 0) {
          printf("Error sending packet for decoding %s.\n", av_err2str(ret));
          return -1;
        }

        while (ret >= 0) {
          ret = codec.receiveFrame(out_frame);
          ON_SCOPE_EXIT([&out_frame] { av_frame_unref(out_frame); });

          // need more packet
          if (ret == AVERROR(EAGAIN)) {
            break;
          } else if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
            // EOF exit loop
            break;
          } else if (ret < 0) {
            printf("Error while decoding.\n");
            return -1;
          }

          out_frame_queue.waitAndPush(out_frame);
        }

        return 0;
      };

  auto decodePacket = [](WaitablePacketQueue &packet_queue, FFMPEGCodec &codec,
                         AVFrame *out_frame) {
    auto *pkt = packet_queue.waitAndPop();
    ON_SCOPE_EXIT([&pkt] {
      if (pkt != nullptr) {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
      }
    });

    int ret = codec.sendPacketToCodec(pkt);
    if (ret < 0) {
      printf("Error sending packet for decoding %s.\n", av_err2str(ret));
      return -1;
    }

    while (ret >= 0) {
      ret = codec.receiveFrame(out_frame);

      // need more packet
      if (ret == AVERROR(EAGAIN)) {
        break;
      } else if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
        // EOF exit loop
        break;
      } else if (ret < 0) {
        printf("Error while decoding.\n");
        return -1;
      }

      // got frame
      return 0;
    }

    return 0;
  };

  std::thread video_decode_thread([&]() {
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
      printf("Could not allocate frame.\n");
      return -1;
    }
    ON_SCOPE_EXIT([&frame] {
      av_frame_unref(frame);
      av_frame_free(&frame);
    });

    for (; sdl_app.running;) {
      if (decoder_ctx.video_packet_sync_que.size() != 0) {
        ret = decodePacket(decoder_ctx.video_packet_sync_que,
                           decoder_ctx.video_codec, frame);
        RETURN_IF_ERROR_LOG(ret, "decode video packet failed\n");
        if (frame) {
          frame->pts = frame->best_effort_timestamp + frame->repeat_pict;
          decoder_ctx.video_frame_sync_que.waitAndPush(frame);
        }
        av_frame_unref(frame);
      }
    }
    return 0;
  });

  std::thread audio_decode_thread([&]() {
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
      printf("Could not allocate frame.\n");
      return -1;
    }
    ON_SCOPE_EXIT([&frame] {
      av_frame_unref(frame);
      av_frame_free(&frame);
    });

    for (; sdl_app.running;) {
      if (decoder_ctx.audio_packet_sync_que.size() != 0) {
        ret = decodePacketAndPushToFrameQueue(decoder_ctx.audio_packet_sync_que,
                                              decoder_ctx.audio_codec, frame,
                                              decoder_ctx.audio_frame_sync_que);
        RETURN_IF_ERROR_LOG(ret, "decode audio packet failed\n");
      }
    }
    return 0;
  });

  SDL_Event event;
  for (; sdl_app.running;) {
    SDL_WaitEvent(&event);
    sdl_app.onEvent(event);
  }

  demux_thread.join();

  decoder_ctx.video_frame_sync_que.clear();
  video_decode_thread.join();

  decoder_ctx.audio_frame_sync_que.clear();
  audio_decode_thread.join();

  sdl_app.onCleanup();
  return 0;
}