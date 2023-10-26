//
// Created by user on 5/23/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_VIDEO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_VIDEO_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_image_converter.h"
#include "j_video_player/modules/j_ffmpeg_base_decoder.h"
#include "j_video_player/modules/j_video_render.h"
#include "j_video_player/utils/av_synchronizer.h"
#include "j_video_player/utils/clock_manager.h"
namespace j_video_player {
class VideoDecoder : public FFMPEGBaseDecoder {
public:
  VideoDecoder(const std::string &file_path) {
    init(file_path, AVMEDIA_TYPE_VIDEO);
  }

  void setRender(std::shared_ptr<IVideoRender> render) {
    video_render_ = std::move(render);
  }

  void setAVSyncClock(std::shared_ptr<utils::ClockManager> clock) {
    clock_ = std::move(clock);
  }

  void onPrepareDecoder() override {
    if (isInitSucc()) {
      converter_ = std::make_unique<ffmpeg_utils::FFMPEGImageConverter>();
      auto codec_context = getCodecContext();
      auto in_width = codec_context->width;
      auto in_height = codec_context->height;
      auto in_pix_fmt = codec_context->pix_fmt;
      auto out_width = in_width;
      auto out_height = in_height;
      auto out_pix_fmt = AV_PIX_FMT_YUV420P;
      converter_->prepare(in_width, in_height, in_pix_fmt, out_width,
                          out_height, out_pix_fmt, SWS_BILINEAR, nullptr,
                          nullptr, nullptr);

      if (video_render_) {
        video_render_->initVideoRender(out_width, out_height);
      }
    }
  }
  void OnDecoderDone() override { converter_ = nullptr; }
  void OnFrameAvailable(AVFrame *frame) override {
    if (frame && converter_) {
      auto result = converter_->convert(frame);
      if (result.second != nullptr) {
        if (video_render_) {
          doAVSync(frame->pts);
          video_render_->renderVideoData(result.second);
        }
      }
    }
  }

  void *getVideoConverter() { return converter_.get(); }

private:
  void doAVSync(int64_t frame_pts) {
    if (clock_) {
      auto pts = frame_pts / (float)(AV_TIME_BASE);
      clock_->setVideoClock(pts);
      auto real_delay_ms = (int)(av_sync_.computeTargetDelay(*clock_) * 1000);
      std::this_thread::sleep_for(std::chrono::milliseconds(real_delay_ms));
    }
  }
  std::unique_ptr<ffmpeg_utils::FFMPEGImageConverter> converter_{nullptr};
  std::shared_ptr<IVideoRender> video_render_{nullptr};
  std::shared_ptr<utils::ClockManager> clock_{nullptr};
  utils::AVSynchronizer av_sync_;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_VIDEO_DECODER_H
