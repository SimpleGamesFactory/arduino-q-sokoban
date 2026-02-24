#include "PlayingScene.h"

#include "SokobanGame.h"

PlayingScene::PlayingScene(SokobanGame& gameRef) : game(gameRef) {}

void PlayingScene::onEnter() {
  // `loadLevel()` marks dirty regions before entering the scene.
}

void PlayingScene::onPhysics(float delta) {
  if (game.levelSolved) {
    game.levelSolvedTimer += delta;
    if (game.fireAction.justPressed() ||
        game.levelSolvedTimer >= SokobanGame::LEVEL_SOLVED_DELAY_S) {
      game.advanceAfterLevelSolved();
    }
    return;
  }

  if (game.fireAction.justPressed()) {
    game.loadLevel(game.currentLevel);
    return;
  }

  bool moved = false;
  if (game.leftAction.justPressed()) {
    moved = game.tryMove(-1, 0);
  } else if (game.rightAction.justPressed()) {
    moved = game.tryMove(1, 0);
  } else if (game.upAction.justPressed()) {
    moved = game.tryMove(0, -1);
  } else if (game.downAction.justPressed()) {
    moved = game.tryMove(0, 1);
  }

  (void)moved;
}

void PlayingScene::onProcess(float delta) {
  (void)delta;
  game.flushDirty();
}
