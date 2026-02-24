#pragma once

#include "SGF/Scene.h"

class SokobanGame;

class GameOverScene : public Scene {
public:
  explicit GameOverScene(SokobanGame& gameRef);

  void onEnter() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  SokobanGame& game;
};
