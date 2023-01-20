//
// Created by user on 1/15/23.
//
#include "ffmpeg_utils/ffmpeg_codec.h"
#include "ffmpeg_utils/ffmpeg_demuxer.h"
#include "ffmpeg_utils/ffmpeg_headers.h"
#include "ffmpeg_utils/ffmpeg_image_converter.h"
#include <SDL2/SDL.h>

#include <stdio.h>

using namespace ffmpeg_utils;

void printHelpMenu();

class SDLApp {
public:
  ~SDLApp() { onCleanup(); }

  int onInit(int video_width, int video_height) {
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (ret != 0) {
      printf("Could not initialize SDL - %s\n.", SDL_GetError());
      return -1;
    }

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

  void onEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_QUIT: {
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

    SDL_Quit();
  }

private:
  SDL_Window *screen{nullptr};
  SDL_Renderer *renderer{nullptr};
  SDL_Texture *texture{nullptr};
};

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    // wrong arguments, print help menu
    printHelpMenu();

    // exit with error
    return -1;
  }

  int maxFramesToDecode;
  sscanf(argv[2], "%d", &maxFramesToDecode);
  std::string infile = argv[1];
  // create ffmpeg demuxer
  FFMPEGDemuxer demuxer;
  int ret = demuxer.openFile(infile);
  if (ret < 0) {
    printf("Could not open file %s\n", infile.c_str());
    return -1;
  }

  demuxer.dumpFormat();

  // Find the first video stream
  int video_stream_index = demuxer.getVideoStreamIndex();
  if (video_stream_index == -1) {
    return -1;
  }

  AVStream *video_stream = demuxer.getStream(video_stream_index);
  FFMPEGCodec video_codec;
  auto codec_id = video_stream->codecpar->codec_id;
  auto par = video_stream->codecpar;
  video_codec.prepare(codec_id, par);

  auto dst_format = AVPixelFormat::AV_PIX_FMT_YUV420P;
  auto codec_context = video_codec.getCodecContext();
  FFMPEGImageConverter img_conv;
  img_conv.prepare(codec_context->width, codec_context->height,
                   codec_context->pix_fmt, codec_context->width,
                   codec_context->height, dst_format, SWS_BILINEAR, nullptr,
                   nullptr, nullptr);

  // Now we need a place to actually store the frame:
  AVFrame *pFrame = NULL;

  // Allocate video frame
  pFrame = av_frame_alloc(); // [9]
  if (pFrame == NULL) {
    // Could not allocate frame
    printf("Could not allocate frame.\n");

    // exit with error
    return -1;
  }

  auto video_width = video_codec.getCodecContext()->width;
  auto video_height = video_codec.getCodecContext()->height;

  SDLApp app;
  app.onInit(video_width, video_height);

  int i = 0;
  AVPacket *packet{nullptr};
  AVFrame *pict{nullptr};
  int convert_output_width{0};
  double fps = av_q2d(video_stream->r_frame_rate);
  double sleep_time = 1.0 / fps;
  SDL_Event event;

  for (;;) {
    if (i >= maxFramesToDecode) {
      break;
    }

    std::tie(ret, packet) = demuxer.readPacket();
    if (packet->stream_index == video_stream_index) {
      ret = video_codec.sendPacketToCodec(packet);
      if (ret < 0) {
        av_packet_unref(packet);
        printf("Error sending packet for decoding %s.\n", av_err2str(ret));
        return -1;
      }
    }
    av_packet_unref(packet);

    while (ret >= 0) {
      ret = video_codec.receiveFrame(pFrame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ||
          ret == AVERROR(EAGAIN)) {
        // EOF exit loop
        av_frame_unref(pFrame);
        break;
      } else if (ret < 0) {
        av_frame_unref(pFrame);
        printf("Error while decoding.\n");
        return -1;
      }

      std::tie(convert_output_width, pict) = img_conv.convert(pFrame);
      // Save the frame to disk
      if (++i <= maxFramesToDecode) {
        // print log information
        printf("Frame %c (%d) pts %lld dts %lld key_frame %d "
               "[coded_picture_number %d, display_picture_number %d,"
               " %dx%d]\n",
               av_get_picture_type_char(pFrame->pict_type),
               codec_context->frame_number, pict->pts, pict->pkt_dts,
               pict->key_frame, pict->coded_picture_number,
               pict->display_picture_number, codec_context->width,
               codec_context->height);

        app.onLoop(pict);
        app.onRender(sleep_time);
      } else {
        break;
      }
    }

    SDL_PollEvent(&event);
    app.onEvent(event);
  }

  // Free the YUV frame
  av_frame_unref(pFrame);
  av_frame_free(&pFrame);

  app.onCleanup();

  return 0;
}

/**
 * Print help menu containing usage information.
 */
void printHelpMenu() {
  printf("Invalid arguments.\n\n");
  printf("Usage: ./tutorial02 <filename> <max-frames-to-decode>\n\n");
  printf("e.g: ./tutorial02 /home/rambodrahmani/Videos/Labrinth-Jealous.mp4 "
         "200\n");
}