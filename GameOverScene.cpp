#include "GameOverScene.h"

#include "SokobanGame.h"

GameOverScene::GameOverScene(SokobanGame& gameRef) : game(gameRef) {}

void GameOverScene::onEnter() {
  game.resetActions();
  game.renderGameOverScreen();
}

void GameOverScene::onInput(const InputEvent& event) {
  if (!event.isActionJustPressed(game.fireAction)) {
    return;
  }

  game.switchScene(game.titleScene);
}

void GameOverScene::onPhysics(float delta) {
  (void)delta;
}

void GameOverScene::onProcess(float delta) {
  (void)delta;
}
