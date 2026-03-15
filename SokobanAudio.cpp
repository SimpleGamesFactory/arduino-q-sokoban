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
  {&kSnarePattern1, 4u},
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

const SGFAudio::Song kTitleSong{
  kTitleSongLanes,
  sizeof(kTitleSongLanes) / sizeof(kTitleSongLanes[0]),
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
