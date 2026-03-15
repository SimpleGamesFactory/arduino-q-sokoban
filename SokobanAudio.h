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
  void startGameplayMusic();
  void playPlayerMove();
  void playBoxMove();
  void playBoxPlaced();
  void playLevelReset();
  void playLevelFinished();

private:
  static constexpr uint32_t SAMPLE_RATE = 11025u;

  bool enabled() const;

  uint8_t outputPin = AUDIO_DISABLED_PIN;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  void playSongAtVolume(uint8_t volume);
  void playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity = 255u);

  SGFAudio::MusicPlayer music;
  static constexpr int SFX_VOICE_START = 5;
  static constexpr int SFX_VOICE_COUNT = 3;
  uint8_t nextSfxVoice = SFX_VOICE_START;
  SGFAudio::ESP32DacAudioOutput audioOutput;
#endif
};
