#include "SokobanAudio.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
namespace {

constexpr float NOTE_B0 = 30.87f;
constexpr float NOTE_C2 = 65.41f;
constexpr float NOTE_CS2 = 69.30f;
constexpr float NOTE_D2 = 73.42f;
constexpr float NOTE_DS2 = 77.78f;
constexpr float NOTE_CS1 = 34.65f;
constexpr float NOTE_DS1 = 38.89f;
constexpr float NOTE_E1 = 41.20f;
constexpr float NOTE_E2 = 82.41f;
constexpr float NOTE_FS1 = 46.25f;
constexpr float NOTE_GS2 = 103.83f;
constexpr float NOTE_GS1 = 51.91f;
constexpr float NOTE_A1 = 55.00f;
constexpr float NOTE_A2 = 110.00f;
constexpr float NOTE_AS2 = 116.54f;
constexpr float NOTE_B1 = 61.74f;
constexpr float NOTE_B2 = 123.47f;
constexpr float NOTE_CS3 = 138.59f;
constexpr float NOTE_DS3 = 155.56f;
constexpr float NOTE_E3 = 164.81f;
constexpr float NOTE_KICK = 46.0f;
constexpr float NOTE_SNARE = 220.0f;

constexpr int TITLE_BASS_VOICE = 0;
constexpr int TITLE_KICK_VOICE = 1;
constexpr int TITLE_SNARE_VOICE = 2;
constexpr int TITLE_ORGAN1_VOICE = 3;
constexpr int TITLE_ORGAN2_VOICE = 4;

constexpr SGFAudio::PitchPoint kKickPitchEnv[] = {
  {0u, 1200},
  {24u, -200},
  {72u, -1400},
};
constexpr SGFAudio::Adsr kBassEnv{4u, 36u, 170u, 32u};
constexpr SGFAudio::Instrument kBassInstrument{
  SGFAudio::Waveform::Saw,
  kBassEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  240.0f,
  0.0f,
  155u
};
constexpr SGFAudio::Adsr kKickEnv{0u, 54u, 0u, 0u};
constexpr SGFAudio::Instrument kKickInstrument{
  SGFAudio::Waveform::Sine,
  kKickEnv,
  {},
  kKickPitchEnv,
  sizeof(kKickPitchEnv) / sizeof(kKickPitchEnv[0]),
  AUDIO_FILTER_LP,
  420.0f,
  0.0f,
  255u
};
constexpr SGFAudio::Adsr kSnareEnv{0u, 28u, 0u, 12u};
constexpr SGFAudio::Instrument kSnareInstrument{
  SGFAudio::Waveform::Noise,
  kSnareEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_HP,
  0.0f,
  1450.0f,
  210u
};
constexpr SGFAudio::Lfo kOrgan1Lfo{
  true,
  SGFAudio::Waveform::Sine,
  5.2f,
  9.0f
};
constexpr SGFAudio::Adsr kOrgan1Env{10u, 60u, 210u, 140u};
constexpr SGFAudio::Instrument kOrgan1Instrument{
  SGFAudio::Waveform::Square,
  kOrgan1Env,
  kOrgan1Lfo,
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  980.0f,
  0.0f,
  58u
};
constexpr SGFAudio::Lfo kOrgan2Lfo{
  true,
  SGFAudio::Waveform::Sine,
  4.8f,
  7.0f
};
constexpr SGFAudio::Adsr kOrgan2Env{10u, 70u, 205u, 150u};
constexpr SGFAudio::Instrument kOrgan2Instrument{
  SGFAudio::Waveform::Saw,
  kOrgan2Env,
  kOrgan2Lfo,
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  900.0f,
  0.0f,
  54u
};

constexpr SGFAudio::Adsr kPlayerMoveEnv{0u, 18u, 0u, 10u};
constexpr SGFAudio::Instrument kPlayerMoveInstrument{
  SGFAudio::Waveform::Noise,
  kPlayerMoveEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  1600.0f,
  90.0f,
  168u
};
constexpr SGFAudio::SfxStep kPlayerMoveSteps[] = {
  {14u, 0, 0, 138u, true, true},
  {20u, -1, 0, 118u, true, false},
  {12u, -3, 0, 92u, true, false},
};
constexpr SGFAudio::Sfx kPlayerMoveSfx{
  &kPlayerMoveInstrument,
  kPlayerMoveSteps,
  sizeof(kPlayerMoveSteps) / sizeof(kPlayerMoveSteps[0])
};

constexpr SGFAudio::Adsr kBoxMoveEnv{0u, 26u, 132u, 28u};
constexpr SGFAudio::Instrument kBoxMoveInstrument{
  SGFAudio::Waveform::Saw,
  kBoxMoveEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  860.0f,
  0.0f,
  132u
};
constexpr SGFAudio::SfxStep kBoxMoveSteps[] = {
  {22u, 0, 0, 220u, true, true},
  {26u, -3, 0, 200u, true, false},
  {34u, -7, 0, 168u, true, false},
};
constexpr SGFAudio::Sfx kBoxMoveSfx{
  &kBoxMoveInstrument,
  kBoxMoveSteps,
  sizeof(kBoxMoveSteps) / sizeof(kBoxMoveSteps[0])
};

constexpr SGFAudio::Adsr kBoxPlacedEnv{0u, 16u, 170u, 38u};
constexpr SGFAudio::Instrument kBoxPlacedInstrument{
  SGFAudio::Waveform::Square,
  kBoxPlacedEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  2400.0f,
  0.0f,
  156u
};
constexpr SGFAudio::SfxStep kBoxPlacedSteps[] = {
  {24u, 0, 0, 220u, true, true},
  {30u, 4, 0, 255u, true, false},
  {34u, 7, 0, 190u, true, false},
};
constexpr SGFAudio::Sfx kBoxPlacedSfx{
  &kBoxPlacedInstrument,
  kBoxPlacedSteps,
  sizeof(kBoxPlacedSteps) / sizeof(kBoxPlacedSteps[0])
};

constexpr SGFAudio::Adsr kLevelResetEnv{0u, 18u, 150u, 52u};
constexpr SGFAudio::Instrument kLevelResetInstrument{
  SGFAudio::Waveform::Saw,
  kLevelResetEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  1800.0f,
  0.0f,
  160u
};
constexpr SGFAudio::SfxStep kLevelResetSteps[] = {
  {28u, 5, 0, 220u, true, true},
  {36u, 0, 0, 205u, true, false},
  {48u, -5, 0, 180u, true, false},
  {60u, -10, 0, 150u, true, false},
};
constexpr SGFAudio::Sfx kLevelResetSfx{
  &kLevelResetInstrument,
  kLevelResetSteps,
  sizeof(kLevelResetSteps) / sizeof(kLevelResetSteps[0])
};

constexpr SGFAudio::Adsr kLevelFinishedEnv{0u, 24u, 196u, 70u};
constexpr SGFAudio::Instrument kLevelFinishedInstrument{
  SGFAudio::Waveform::Triangle,
  kLevelFinishedEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  2600.0f,
  0.0f,
  176u
};
constexpr SGFAudio::SfxStep kLevelFinishedSteps[] = {
  {42u, 0, 0, 220u, true, true},
  {42u, 4, 0, 240u, true, false},
  {56u, 7, 0, 255u, true, false},
  {72u, 12, 0, 220u, true, false},
};
constexpr SGFAudio::Sfx kLevelFinishedSfx{
  &kLevelFinishedInstrument,
  kLevelFinishedSteps,
  sizeof(kLevelFinishedSteps) / sizeof(kLevelFinishedSteps[0])
};

constexpr SGFAudio::PatternStep kBassPattern1Steps[] = {
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_A1, 1u, 210u},
  {0.0f, 3u, 0u},
  {NOTE_E1, 1u, 208u},
  {0.0f, 3u, 0u},
  {NOTE_A1, 1u, 210u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_E1, 1u, 208u},
  {NOTE_FS1, 1u, 208u},
  {NOTE_GS1, 1u, 214u},
};
constexpr SGFAudio::Pattern kBassPattern1{
  kBassPattern1Steps,
  sizeof(kBassPattern1Steps) / sizeof(kBassPattern1Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kBassPattern2Steps[] = {
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_E1, 1u, 210u},
  {0.0f, 3u, 0u},
  {NOTE_B0, 1u, 205u},
  {0.0f, 3u, 0u},
  {NOTE_E1, 1u, 210u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_B0, 1u, 205u},
  {NOTE_CS1, 1u, 208u},
  {NOTE_DS1, 1u, 214u},
};
constexpr SGFAudio::Pattern kBassPattern2{
  kBassPattern2Steps,
  sizeof(kBassPattern2Steps) / sizeof(kBassPattern2Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kKickPattern1Steps[] = {
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
};
constexpr SGFAudio::Pattern kKickPattern1{
  kKickPattern1Steps,
  sizeof(kKickPattern1Steps) / sizeof(kKickPattern1Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kKickPattern2Steps[] = {
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 250u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 3u, 0u},
  {NOTE_KICK, 1u, 250u},
  {0.0f, 3u, 0u},
};
constexpr SGFAudio::Pattern kKickPattern2{
  kKickPattern2Steps,
  sizeof(kKickPattern2Steps) / sizeof(kKickPattern2Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kSnarePattern1Steps[] = {
  {0.0f, 16u, 0u},
};
constexpr SGFAudio::Pattern kSnarePattern1{
  kSnarePattern1Steps,
  sizeof(kSnarePattern1Steps) / sizeof(kSnarePattern1Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kSnarePattern2Steps[] = {
  {0.0f, 1u, 0u},
  {0.0f, 3u, 0u},
  {NOTE_SNARE, 1u, 225u},
  {0.0f, 3u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 3u, 0u},
  {NOTE_SNARE, 1u, 225u},
  {0.0f, 3u, 0u},
};
constexpr SGFAudio::Pattern kSnarePattern2{
  kSnarePattern2Steps,
  sizeof(kSnarePattern2Steps) / sizeof(kSnarePattern2Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan1Pattern1Steps[] = {
  {NOTE_CS3, 4u, 168u},
  {0.0f, 4u, 0u},
  {NOTE_DS3, 4u, 172u},
  {NOTE_E3, 4u, 176u},
  {NOTE_CS3, 4u, 168u},
  {0.0f, 12u, 0u},
};
constexpr SGFAudio::Pattern kOrgan1Pattern1{
  kOrgan1Pattern1Steps,
  sizeof(kOrgan1Pattern1Steps) / sizeof(kOrgan1Pattern1Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan2Pattern1Steps[] = {
  {NOTE_A2, 4u, 156u},
  {0.0f, 8u, 0u},
  {NOTE_A2, 4u, 156u},
  {NOTE_A2, 4u, 160u},
  {0.0f, 8u, 0u},
  {NOTE_E2, 1u, 154u},
  {NOTE_DS2, 1u, 154u},
  {NOTE_D2, 1u, 154u},
  {NOTE_C2, 1u, 154u},
};
constexpr SGFAudio::Pattern kOrgan2Pattern1{
  kOrgan2Pattern1Steps,
  sizeof(kOrgan2Pattern1Steps) / sizeof(kOrgan2Pattern1Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan1Pattern2Steps[] = {
  {NOTE_GS2, 4u, 168u},
  {0.0f, 4u, 0u},
  {NOTE_AS2, 4u, 172u},
  {NOTE_B2, 4u, 176u},
  {NOTE_GS2, 4u, 168u},
  {0.0f, 12u, 0u},
};
constexpr SGFAudio::Pattern kOrgan1Pattern2{
  kOrgan1Pattern2Steps,
  sizeof(kOrgan1Pattern2Steps) / sizeof(kOrgan1Pattern2Steps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan2Pattern2Steps[] = {
  {NOTE_E2, 4u, 156u},
  {0.0f, 8u, 0u},
  {NOTE_E2, 4u, 156u},
  {NOTE_E2, 4u, 160u},
  {0.0f, 8u, 0u},
  {NOTE_B1, 1u, 154u},
  {NOTE_CS2, 1u, 154u},
  {NOTE_D2, 1u, 154u},
  {NOTE_DS2, 1u, 154u},
};
constexpr SGFAudio::Pattern kOrgan2Pattern2{
  kOrgan2Pattern2Steps,
  sizeof(kOrgan2Pattern2Steps) / sizeof(kOrgan2Pattern2Steps[0]),
  0u,
  false
};

const SGFAudio::SongClip kBassClips[] = {
  {&kBassPattern1, 4u},
  {&kBassPattern2, 4u},
};
const SGFAudio::SongClip kKickClips[] = {
  {&kKickPattern1, 4u},
  {&kKickPattern2, 4u},
};
const SGFAudio::SongClip kSnareClips[] = {
  {&kSnarePattern2, 4u},
  {&kSnarePattern2, 4u},
};
const SGFAudio::SongClip kOrgan1Clips[] = {
  {&kOrgan1Pattern1, 2u},
  {&kOrgan1Pattern2, 2u},
};
const SGFAudio::SongClip kOrgan2Clips[] = {
  {&kOrgan2Pattern1, 2u},
  {&kOrgan2Pattern2, 2u},
};

const SGFAudio::SongLane kTitleSongLanes[] = {
  {TITLE_BASS_VOICE, SGFAudio::makeProgramRef(kBassInstrument), kBassClips,
   sizeof(kBassClips) / sizeof(kBassClips[0])},
  {TITLE_KICK_VOICE, SGFAudio::makeProgramRef(kKickInstrument), kKickClips,
   sizeof(kKickClips) / sizeof(kKickClips[0])},
  {TITLE_SNARE_VOICE, SGFAudio::makeProgramRef(kSnareInstrument), kSnareClips,
   sizeof(kSnareClips) / sizeof(kSnareClips[0])},
  {TITLE_ORGAN1_VOICE, SGFAudio::makeProgramRef(kOrgan1Instrument), kOrgan1Clips,
   sizeof(kOrgan1Clips) / sizeof(kOrgan1Clips[0])},
  {TITLE_ORGAN2_VOICE, SGFAudio::makeProgramRef(kOrgan2Instrument), kOrgan2Clips,
   sizeof(kOrgan2Clips) / sizeof(kOrgan2Clips[0])},
};

const SGFAudio::SongClip kGameplayKickClips[] = {
  {&kKickPattern1, 8u},
};
const SGFAudio::SongClip kGameplaySnareClips[] = {
  {&kSnarePattern1, 8u},
};

const SGFAudio::SongLane kGameplaySongLanes[] = {
  {TITLE_BASS_VOICE, SGFAudio::makeProgramRef(kBassInstrument), kBassClips,
   sizeof(kBassClips) / sizeof(kBassClips[0])},
  {TITLE_KICK_VOICE, SGFAudio::makeProgramRef(kKickInstrument), kGameplayKickClips,
   sizeof(kGameplayKickClips) / sizeof(kGameplayKickClips[0])},
  {TITLE_SNARE_VOICE, SGFAudio::makeProgramRef(kSnareInstrument), kGameplaySnareClips,
   sizeof(kGameplaySnareClips) / sizeof(kGameplaySnareClips[0])},
  {TITLE_ORGAN1_VOICE, SGFAudio::makeProgramRef(kOrgan1Instrument), kOrgan1Clips,
   sizeof(kOrgan1Clips) / sizeof(kOrgan1Clips[0])},
  {TITLE_ORGAN2_VOICE, SGFAudio::makeProgramRef(kOrgan2Instrument), kOrgan2Clips,
   sizeof(kOrgan2Clips) / sizeof(kOrgan2Clips[0])},
};

const SGFAudio::Song kTitleSong{
  kTitleSongLanes,
  sizeof(kTitleSongLanes) / sizeof(kTitleSongLanes[0]),
  90u,
  4u
};

const SGFAudio::Song kGameplaySong{
  kGameplaySongLanes,
  sizeof(kGameplaySongLanes) / sizeof(kGameplaySongLanes[0]),
  90u,
  4u
};

}  // namespace

SokobanAudio::SokobanAudio(uint8_t outputPin)
  : outputPin(outputPin),
    music(SAMPLE_RATE),
    audioOutput(music, outputPin) {}

void SokobanAudio::setup() {
  music.setVolume(190u);
  if (enabled()) {
    audioOutput.begin();
  }
}

void SokobanAudio::startTitleMusic() {
  playSongAtVolume(190u);
}

void SokobanAudio::startGameplayMusic() {
  if (!enabled()) {
    return;
  }
  music.setVolume(95u);
  music.play(kGameplaySong);
}

void SokobanAudio::playPlayerMove() {
  playSfx(kPlayerMoveSfx, 220.0f, 180u);
}

void SokobanAudio::playBoxMove() {
  playSfx(kBoxMoveSfx, 123.47f, 230u);
}

void SokobanAudio::playBoxPlaced() {
  playSfx(kBoxPlacedSfx, 392.0f, 255u);
}

void SokobanAudio::playLevelReset() {
  playSfx(kLevelResetSfx, 220.0f, 240u);
}

void SokobanAudio::playLevelFinished() {
  playSfx(kLevelFinishedSfx, 261.63f, 255u);
}

void SokobanAudio::playSongAtVolume(uint8_t volume) {
  if (!enabled()) {
    return;
  }
  music.setVolume(volume);
  music.play(kTitleSong);
}

void SokobanAudio::playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity) {
  if (!enabled()) {
    return;
  }

  int voice = nextSfxVoice;
  for (int i = 0; i < SFX_VOICE_COUNT; ++i) {
    int candidate = SFX_VOICE_START + ((nextSfxVoice - SFX_VOICE_START + i) % SFX_VOICE_COUNT);
    if (!music.synth().voiceActive(candidate)) {
      voice = candidate;
      break;
    }
  }

  music.synth().triggerSfx(voice, sfx, baseHz, velocity);
  nextSfxVoice = SFX_VOICE_START + ((voice - SFX_VOICE_START + 1) % SFX_VOICE_COUNT);
}

bool SokobanAudio::enabled() const {
  return outputPin != AUDIO_DISABLED_PIN;
}
#else
SokobanAudio::SokobanAudio(uint8_t outputPin) : outputPin(outputPin) {}

void SokobanAudio::setup() {}

void SokobanAudio::startTitleMusic() {}

void SokobanAudio::startGameplayMusic() {}

void SokobanAudio::playPlayerMove() {}

void SokobanAudio::playBoxMove() {}

void SokobanAudio::playBoxPlaced() {}

void SokobanAudio::playLevelReset() {}

void SokobanAudio::playLevelFinished() {}

bool SokobanAudio::enabled() const {
  return false;
}
#endif
