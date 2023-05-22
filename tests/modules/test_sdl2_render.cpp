//
// Created by user on 5/22/23.
//
#include "j_video_player/modules/j_sdl2_render.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;

class ASDL2Render : public Test {
public:
  void TearDown() override { r.uninit(); }
  SDL2Render r;
};

TEST_F(ASDL2Render, CanInitSDL2Render) {
  r.initAudioRender();
  r.initVideoRender(1280, 720);
}

TEST_F(ASDL2Render, UninitReleaseResources) {
  r.initAudioRender();
  r.initVideoRender(1280, 720);

  r.uninit();
}

TEST_F(ASDL2Render, RenderAudioDataPushDataToFIFO) {
  r.initAudioRender();
  ASSERT_THAT(r.getFIFOSize(), Eq(0));

  int n = 1024;
  std::vector<int16_t> data(n, 0);
  r.renderAudioData(data.data(), n);

  ASSERT_THAT(r.getFIFOSize(), Eq(n));
}