#pragma once

#include "SGF/Scene.h"

class SokobanGame;

class PlayingScene : public Scene {
public:
  explicit PlayingScene(SokobanGame& gameRef);

  void onEnter() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  SokobanGame& game;
};
