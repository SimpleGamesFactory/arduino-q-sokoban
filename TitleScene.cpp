#include "TitleScene.h"

#include "SokobanGame.h"

TitleScene::TitleScene(SokobanGame& gameRef) : game(gameRef) {}

void TitleScene::onEnter() {
  game.resetActions();
  game.renderTitleScreen();
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireAction.isJustPressed()) {
    game.startNewGame();
    game.switchScene(game.playingScene);
  }
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}
