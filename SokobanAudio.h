#pragma once

#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#include "SGF/AudioTypes.h"
#include "SGF/ESP32DacAudioOutput.h"
#include "SGF/MusicPlayer.h"
#include "SGF/Synth.h"
#endif

class SokobanAudio {
public:
  static constexpr uint8_t AUDIO_DISABLED_PIN = 0xFF;

  explicit SokobanAudio(uint8_t outputPin);

  void setup();
  void startTitleMusic();
  void stopTitleMusic();

private:
  static constexpr uint32_t SAMPLE_RATE = 11025u;

  bool enabled() const;

  uint8_t outputPin = AUDIO_DISABLED_PIN;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  SGFAudio::MusicPlayer music;
  SGFAudio::ESP32DacAudioOutput audioOutput;
#endif
};
