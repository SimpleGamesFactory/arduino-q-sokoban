#include "SokobanAudio.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
namespace {

constexpr float NOTE_C1 = 32.70f;
constexpr float NOTE_D1 = 36.71f;
constexpr float NOTE_F1 = 43.65f;
constexpr float NOTE_G1 = 49.00f;
constexpr float NOTE_AS1 = 58.27f;
constexpr float NOTE_KICK = 46.0f;
constexpr float NOTE_SNARE = 220.0f;
constexpr float NOTE_C2 = 65.41f;
constexpr float NOTE_D2 = 73.42f;
constexpr float NOTE_F2 = 87.31f;
constexpr float NOTE_G2 = 98.00f;
constexpr float NOTE_AS2 = 116.54f;
constexpr float NOTE_C3 = 130.81f;
constexpr float NOTE_D3 = 146.83f;

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

constexpr SGFAudio::PatternStep kBassPatternASteps[] = {
  {NOTE_C1, 2u, 205u},
  {0.0f, 2u, 0u},
  {NOTE_G1, 2u, 205u},
  {0.0f, 2u, 0u},
  {NOTE_AS1, 2u, 210u},
  {0.0f, 2u, 0u},
  {NOTE_G1, 2u, 205u},
  {0.0f, 2u, 0u},
};
constexpr SGFAudio::Pattern kBassPatternA{
  kBassPatternASteps,
  sizeof(kBassPatternASteps) / sizeof(kBassPatternASteps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kBassPatternBSteps[] = {
  {NOTE_D1, 2u, 205u},
  {0.0f, 2u, 0u},
  {NOTE_AS1, 2u, 208u},
  {0.0f, 2u, 0u},
  {NOTE_F1, 2u, 208u},
  {0.0f, 2u, 0u},
  {NOTE_G1, 2u, 210u},
  {0.0f, 2u, 0u},
};
constexpr SGFAudio::Pattern kBassPatternB{
  kBassPatternBSteps,
  sizeof(kBassPatternBSteps) / sizeof(kBassPatternBSteps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kKickSteps[] = {
  {NOTE_KICK, 1u, 255u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_KICK, 1u, 248u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_KICK, 1u, 244u},
  {0.0f, 1u, 0u},
  {NOTE_KICK, 1u, 230u},
  {0.0f, 1u, 0u},
};
constexpr SGFAudio::Pattern kKickPattern{
  kKickSteps,
  sizeof(kKickSteps) / sizeof(kKickSteps[0]),
  0u,
  true
};

constexpr SGFAudio::PatternStep kSnareSteps[] = {
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 205u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 220u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 205u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 225u},
  {0.0f, 1u, 0u},
};
constexpr SGFAudio::Pattern kSnarePattern{
  kSnareSteps,
  sizeof(kSnareSteps) / sizeof(kSnareSteps[0]),
  0u,
  true
};

constexpr SGFAudio::PatternStep kOrgan1PhraseASteps[] = {
  {NOTE_G2, 4u, 164u},
  {NOTE_AS2, 4u, 170u},
  {NOTE_C3, 4u, 176u},
  {NOTE_G2, 4u, 166u},
};
constexpr SGFAudio::Pattern kOrgan1PhraseA{
  kOrgan1PhraseASteps,
  sizeof(kOrgan1PhraseASteps) / sizeof(kOrgan1PhraseASteps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan1PhraseBSteps[] = {
  {NOTE_F2, 4u, 164u},
  {NOTE_G2, 4u, 168u},
  {NOTE_D3, 4u, 178u},
  {NOTE_AS2, 4u, 170u},
};
constexpr SGFAudio::Pattern kOrgan1PhraseB{
  kOrgan1PhraseBSteps,
  sizeof(kOrgan1PhraseBSteps) / sizeof(kOrgan1PhraseBSteps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan2PhraseASteps[] = {
  {NOTE_C2, 2u, 150u},
  {0.0f, 2u, 0u},
  {NOTE_D2, 2u, 152u},
  {0.0f, 2u, 0u},
  {NOTE_F2, 2u, 156u},
  {0.0f, 2u, 0u},
  {NOTE_D2, 2u, 152u},
  {0.0f, 2u, 0u},
};
constexpr SGFAudio::Pattern kOrgan2PhraseA{
  kOrgan2PhraseASteps,
  sizeof(kOrgan2PhraseASteps) / sizeof(kOrgan2PhraseASteps[0]),
  0u,
  false
};

constexpr SGFAudio::PatternStep kOrgan2PhraseBSteps[] = {
  {NOTE_AS1, 2u, 148u},
  {0.0f, 2u, 0u},
  {NOTE_C2, 2u, 150u},
  {0.0f, 2u, 0u},
  {NOTE_D2, 2u, 152u},
  {0.0f, 2u, 0u},
  {NOTE_C2, 2u, 150u},
  {0.0f, 2u, 0u},
};
constexpr SGFAudio::Pattern kOrgan2PhraseB{
  kOrgan2PhraseBSteps,
  sizeof(kOrgan2PhraseBSteps) / sizeof(kOrgan2PhraseBSteps[0]),
  0u,
  false
};

const SGFAudio::SongClip kBassClips[] = {
  {&kBassPatternA, 1u},
  {&kBassPatternB, 1u},
};
const SGFAudio::SongClip kKickClips[] = {
  {&kKickPattern, 2u},
};
const SGFAudio::SongClip kSnareClips[] = {
  {&kSnarePattern, 2u},
};
const SGFAudio::SongClip kOrgan1Clips[] = {
  {&kOrgan1PhraseA, 1u},
  {&kOrgan1PhraseB, 1u},
};
const SGFAudio::SongClip kOrgan2Clips[] = {
  {&kOrgan2PhraseA, 1u},
  {&kOrgan2PhraseB, 1u},
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

const SGFAudio::Song kTitleSong{
  kTitleSongLanes,
  sizeof(kTitleSongLanes) / sizeof(kTitleSongLanes[0]),
  84u,
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
  if (!enabled()) {
    return;
  }
  music.play(kTitleSong);
}

void SokobanAudio::stopTitleMusic() {
  music.stop();
}

bool SokobanAudio::enabled() const {
  return outputPin != AUDIO_DISABLED_PIN;
}
#else
SokobanAudio::SokobanAudio(uint8_t outputPin) : outputPin(outputPin) {}

void SokobanAudio::setup() {}

void SokobanAudio::startTitleMusic() {}

void SokobanAudio::stopTitleMusic() {}

bool SokobanAudio::enabled() const {
  return false;
}
#endif
