#include "GameOverScene.h"

#include "SokobanGame.h"

GameOverScene::GameOverScene(SokobanGame& gameRef) : game(gameRef) {}

void GameOverScene::onEnter() {
  game.resetActions();
  game.renderGameOverScreen();
}

void GameOverScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireAction.isJustPressed()) {
    game.switchScene(game.titleScene);
  }
}

void GameOverScene::onProcess(float delta) {
  (void)delta;
}
