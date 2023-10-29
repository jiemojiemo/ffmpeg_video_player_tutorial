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

  auto doSeekRelative = [&](float sec) {
    auto current_pos = audio_decoder->getCurrentPosition();
    auto target_pos = current_pos + sec;
    printf("seek to %f\n", target_pos);
    audio_decoder->seek(target_pos);
    video_decoder->seek(target_pos);
  };

  auto doPauseOrPlaying = [&]() {
    auto is_playing = audio_decoder->isPlaying();
    if (is_playing) {
      audio_decoder->pause();
      video_decoder->pause();
    } else {
      audio_decoder->start();
      video_decoder->start();
    }
  };

  SDL_Event event;
  for (;;) {
    SDL_WaitEvent(&event);
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
        doPauseOrPlaying();
        break;
      }
      default:
        break;
      }
      break;
    }
    case SDL_QUIT:
      video_decoder->stop();
      audio_decoder->stop();

      return 0;
    }
  }
}