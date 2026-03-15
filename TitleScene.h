#pragma once

#include "SGF/Scene.h"

class SokobanGame;

class TitleScene : public Scene {
public:
  explicit TitleScene(SokobanGame& gameRef);

  void onEnter() override;
  void onExit() override;
  void onInput(const InputEvent& event) override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  SokobanGame& game;
};
