//
// Created by user on 11/24/23.
//

#ifndef VIDEOPLAYERTUTORIALS_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_AUDIO_OUTPUT_H
#define VIDEOPLAYERTUTORIALS_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_AUDIO_OUTPUT_H
#include "SuperpoweredAndroidAudioIO.h"
#include "j_video_player/modules/j_base_audio_output.h"

namespace j_video_player {
class AndroidAudioOutput : public BaseAudioOutput {
public:
  int prepare(const AudioOutputParameters &params) override {
    if(!params.isValid()) {
      LOGE("Invalid AudioOutputParameters");
      return -1;
    }

    if(params.channels != 2) {
      LOGE("Only support stereo audio");
      return -1;
    }

    audio_io_ = std::make_unique<SuperpoweredAndroidAudioIO>(
        params.sample_rate,
        params.num_frames_of_buffer,
        false,
        true,
        &audioProcessingCallback,
        this
    );
    return 0;
  }
  int play() override {
    audio_io_->start();
    state_ = AudioOutputState::kPlaying;
    return 0;
  }
  int stop() override {
    audio_io_->stop();
    state_ = AudioOutputState::kStopped;
    return 0;
  }

private:
  static bool audioProcessingCallback (void *clientdata, short int *audioIO, int numberOfFrames, int samplerate)
  {
    auto* self = static_cast<AndroidAudioOutput*>(clientdata);
    self->pullAudioSamples(audioIO, numberOfFrames * 2);
    return true;
  }

  std::unique_ptr<SuperpoweredAndroidAudioIO> audio_io_{nullptr};
};
}

#endif //VIDEOPLAYERTUTORIALS_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_AUDIO_OUTPUT_H
