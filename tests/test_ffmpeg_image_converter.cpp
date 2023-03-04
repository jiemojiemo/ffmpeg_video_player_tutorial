//
// Created by user on 7/20/22.
//

#include "ffmpeg_utils/ffmpeg_image_converter.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace ffmpeg_utils;

class AFFMPEGImageConverter : public Test {
public:
  void TearDown() override {
    if (in_frame_buffer != nullptr) {
      av_free(in_frame_buffer);
    }
    if (in_frame != nullptr) {
      av_frame_free(&in_frame);
    }
  }

  void allocateInFrame() {
    int num_bytes = av_image_get_buffer_size(src_format, src_w, src_h,
                                             FFMPEGImageConverter::kAlign);

    in_frame_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    in_frame = av_frame_alloc();
    av_image_fill_arrays(in_frame->data, in_frame->linesize, in_frame_buffer,
                         src_format, src_w, src_h,
                         FFMPEGImageConverter::kAlign);
  }

  FFMPEGImageConverter c;
  int src_w = 10;
  int src_h = 10;
  int dst_w = 20;
  int dst_h = 20;
  AVPixelFormat src_format{AVPixelFormat::AV_PIX_FMT_YUV420P};
  AVPixelFormat dst_format{AVPixelFormat::AV_PIX_FMT_YUV410P};
  int flags = SWS_BILINEAR;

  AVFrame *in_frame{nullptr};
  uint8_t *in_frame_buffer{nullptr};
};

TEST_F(AFFMPEGImageConverter, PrepareWillAllocateInternalSwsContext) {
  c.prepare(src_w, src_h, src_format, dst_w, dst_h, dst_format, flags, nullptr,
            nullptr, nullptr);

  ASSERT_THAT(c.sws_ctx, NotNull());
}

TEST_F(AFFMPEGImageConverter, CanClearInterStates) {
  c.prepare(src_w, src_h, src_format, dst_w, dst_h, dst_format, flags, nullptr,
            nullptr, nullptr);

  c.clear();

  ASSERT_THAT(c.sws_ctx, IsNull());
  ASSERT_THAT(c.frame_buffer, IsNull());
  ASSERT_THAT(c.frame, IsNull());
}

TEST_F(AFFMPEGImageConverter, CanConvertFrame) {

  allocateInFrame();
  c.prepare(src_w, src_h, src_format, dst_w, dst_h, dst_format, flags, nullptr,
            nullptr, nullptr);

  auto [out_h, out] = c.convert(in_frame);

  ASSERT_THAT(out, NotNull());
}