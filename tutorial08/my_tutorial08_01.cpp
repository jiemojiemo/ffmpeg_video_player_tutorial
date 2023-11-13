//
// Created by user on 11/13/23.
//
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_sdl2_video_output.h"
#include "j_video_player/modules/j_simple_source.h"
void printHelpMenu() {
  printf("Invalid arguments.\n\n");
  printf("Usage: ./tutorial03 <filename> <max-frames-to-decode>\n\n");
  printf("e.g: ./tutorial03 /home/rambodrahmani/Videos/Labrinth-Jealous.mp4 "
         "200\n");
}

using namespace j_video_player;
using namespace std::chrono_literals;
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printHelpMenu();
    return -1;
  }
  std::string in_file = argv[1];

  auto video_decoder = std::make_shared<FFmpegVideoDecoder>();
  auto video_source = std::make_shared<SimpleVideoSource>(video_decoder);
  auto video_output = std::make_shared<SDL2VideoOutput>();
  auto image_converter = std::make_shared<ffmpeg_utils::FFMPEGImageConverter>();
  auto av_clock = std::make_shared<utils::ClockManager>();

  // prepare for all
  auto ret = video_source->open(in_file);
  if (ret != 0) {
    LOGE("open source failed, exit");
    return -1;
  }

  auto media_file_info = video_source->getMediaFileInfo();

  int expected_width = 640;
  int expected_height = 480;
  int expected_pixel_format = AV_PIX_FMT_YUV420P;
  image_converter->prepare(media_file_info.width, media_file_info.height,
                           (AVPixelFormat)media_file_info.pixel_format,
                           expected_width, expected_height,
                           (AVPixelFormat)expected_pixel_format, 0, nullptr,
                           nullptr, nullptr);

  video_output->attachSource(video_source);
  video_output->attachImageConverter(image_converter);
  //  video_output->attachAVSyncClock(av_clock);

  VideoOutputParameters output_param;
  output_param.width = expected_width;
  output_param.height = expected_height;
  output_param.pixel_format = SDL_PIXELFORMAT_IYUV;
  ret = video_output->prepare(output_param);
  if (ret != 0) {
    LOGE("prepare video output failed, exit");
    return -1;
  }

  video_source->play();
  video_output->play();

  SDL_Event event;
  for (;;) {
    SDL_PollEvent(&event);
    switch (event.type) {
    case SDL_QUIT:
      video_source->stop();
      video_output->stop();
      return 0;
    }
  }
}