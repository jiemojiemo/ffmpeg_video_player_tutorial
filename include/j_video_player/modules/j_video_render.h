//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_VIDEO_RENDER_H
#define FFMPEG_VIDEO_PLAYER_J_VIDEO_RENDER_H
#include <libavutil/frame.h>
namespace j_video_player {
class IVideoRenderDeprecated {
public:
  virtual ~IVideoRenderDeprecated() = default;
  virtual void initVideoRender(int video_width, int video_height) = 0;
  virtual void uninit() = 0;
  virtual void renderVideoData(AVFrame *frame) = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_VIDEO_RENDER_H
