#include "TitleScene.h"

#include "SokobanGame.h"

TitleScene::TitleScene(SokobanGame& gameRef) : game(gameRef) {}

void TitleScene::onEnter() {
  game.resetActions();
  game.audio.startTitleMusic();
  game.renderTitleScreen();
}

void TitleScene::onExit() {
}

void TitleScene::onInput(const InputEvent& event) {
  if (!event.isActionJustPressed(game.fireAction)) {
    return;
  }

  game.startNewGame();
  game.switchScene(game.playingScene);
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}
