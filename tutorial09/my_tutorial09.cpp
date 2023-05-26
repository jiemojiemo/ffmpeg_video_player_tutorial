//
// Created by user on 5/22/23.
//
#include "j_video_player/modules/j_audio_decoder.h"
#include "j_video_player/modules/j_sdl2_render.h"
#include "j_video_player/modules/j_video_decoder.h"
#include <iostream>
void printHelpMenu() {
  printf("Invalid arguments.\n\n");
  printf("Usage: ./tutorial03 <filename> <max-frames-to-decode>\n\n");
  printf("e.g: ./tutorial03 /home/rambodrahmani/Videos/Labrinth-Jealous.mp4 "
         "200\n");
}

using namespace j_video_player;
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printHelpMenu();
    return -1;
  }

  std::string in_file = argv[1];

  //  auto audio_decoder = std::make_unique<AudioDecoder>(in_file);
  auto video_decoder = std::make_unique<VideoDecoder>(in_file);
  auto video_render = std::make_shared<SDL2Render>();
  auto audio_decoder = std::make_unique<AudioDecoder>(in_file);
  auto audio_render = std::make_shared<SDL2Render>();


  // sdl2 window must init in main thread
  auto width = video_decoder->getCodecContext()->width;
  auto height = video_decoder->getCodecContext()->height;
  video_render->initVideoRender(width, height);

  auto clock = std::make_shared<utils::ClockManager>();
  video_decoder->setAVSyncClock(clock);
  video_decoder->setRender(video_render);
  audio_decoder->setAVSyncClock(clock);
  audio_decoder->setRender(audio_render);

  video_decoder->start();
  audio_decoder->start();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  SDL_Event event;
  for (;;) {
    SDL_WaitEvent(&event);
    switch (event.type) {
    case SDL_QUIT:
      video_decoder->stop();
      return 0;
    }
  }
}