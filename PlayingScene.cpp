#include "PlayingScene.h"

#include "SokobanGame.h"

PlayingScene::PlayingScene(SokobanGame& gameRef) : game(gameRef) {}

void PlayingScene::onEnter() {
  // `loadLevel()` marks dirty regions before entering the scene.
}

void PlayingScene::onPhysics(float delta) {
  if (game.levelSolved) {
    game.levelSolvedTimer += delta;
    if (game.fireAction.isJustPressed() ||
        game.levelSolvedTimer >= SokobanGame::LEVEL_SOLVED_DELAY_S) {
      game.advanceAfterLevelSolved();
    }
    return;
  }

  if (game.fireAction.isJustPressed()) {
    game.loadLevel(game.currentLevel);
    return;
  }

  bool moved = false;
  if (game.leftAction.isJustPressed()) {
    moved = game.tryMove(-1, 0);
  } else if (game.rightAction.isJustPressed()) {
    moved = game.tryMove(1, 0);
  } else if (game.upAction.isJustPressed()) {
    moved = game.tryMove(0, -1);
  } else if (game.downAction.isJustPressed()) {
    moved = game.tryMove(0, 1);
  }

  (void)moved;
}

void PlayingScene::onProcess(float delta) {
  (void)delta;
  game.flushDirty();
}
