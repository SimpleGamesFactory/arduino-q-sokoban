#include "GameOverScene.h"

#include "SokobanGame.h"

GameOverScene::GameOverScene(SokobanGame& gameRef) : game(gameRef) {}

void GameOverScene::onEnter() {
  game.fireConfirm.reset();
  game.renderGameOverScreen();
}

void GameOverScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireConfirm.update(game.fireAction)) {
    game.sceneSwitcher.switchTo(game.titleScene);
    game.resetClock();
  }
}

void GameOverScene::onProcess(float delta) {
  (void)delta;
}
