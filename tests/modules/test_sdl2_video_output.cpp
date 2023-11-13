//
// Created by user on 11/13/23.
//

#include "j_video_player/modules/j_sdl2_video_output.h"
#include <gmock/gmock.h>
using namespace testing;

using namespace j_video_player;

class ASDL2VideoOutput : public Test {
public:
  SDL2VideoOutput output;
  VideoOutputParameters parameters;
};

TEST_F(ASDL2VideoOutput, prepareFailedIfWidthLessThanZero) {
  parameters.width = -1;
  parameters.height = 720;
  ASSERT_THAT(output.prepare(parameters), Eq(-1));
}

TEST_F(ASDL2VideoOutput, prepareFailedIfHeightLessThanZero) {
  parameters.width = 1280;
  parameters.height = -1;
  ASSERT_THAT(output.prepare(parameters), Eq(-1));
}

TEST_F(ASDL2VideoOutput, CanPrepareWithParameters) {
  parameters.width = 1280;
  parameters.height = 720;
  parameters.pixel_format = SDL_PIXELFORMAT_IYUV;
  ASSERT_THAT(output.prepare(parameters), Eq(0));
}