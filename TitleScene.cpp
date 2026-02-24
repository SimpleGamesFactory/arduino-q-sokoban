#include "TitleScene.h"

#include "SokobanGame.h"

TitleScene::TitleScene(SokobanGame& gameRef) : game(gameRef) {}

void TitleScene::onEnter() {
  game.fireConfirm.reset();
  game.renderTitleScreen();
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireConfirm.update(game.fireAction)) {
    game.startNewGame();
    game.sceneSwitcher.switchTo(game.playingScene);
    game.resetClock();
  }
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}
