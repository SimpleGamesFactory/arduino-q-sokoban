#include "PlayingScene.h"

#include "SokobanGame.h"

PlayingScene::PlayingScene(SokobanGame& gameRef) : game(gameRef) {}

void PlayingScene::onEnter() {
  game.audio.startGameplayMusic();
  // `loadLevel()` marks dirty regions before entering the scene.
}

void PlayingScene::onInput(const InputEvent& event) {
  if (game.levelSolved) {
    if (event.isActionJustPressed(game.fireAction)) {
      game.advanceAfterLevelSolved();
    }
    return;
  }

  if (event.isActionJustPressed(game.fireAction)) {
    game.audio.playLevelReset();
    game.loadLevel(game.currentLevel);
    return;
  }

  if (event.isActionJustPressed(game.leftAction)) {
    game.tryMove(-1, 0);
  } else if (event.isActionJustPressed(game.rightAction)) {
    game.tryMove(1, 0);
  } else if (event.isActionJustPressed(game.upAction)) {
    game.tryMove(0, -1);
  } else if (event.isActionJustPressed(game.downAction)) {
    game.tryMove(0, 1);
  }
}

void PlayingScene::onPhysics(float delta) {
  if (game.levelSolved) {
    game.levelSolvedTimer += delta;
    if (game.levelSolvedTimer >= SokobanGame::LEVEL_SOLVED_DELAY_S) {
      game.advanceAfterLevelSolved();
    }
  }
}

void PlayingScene::onProcess(float delta) {
  (void)delta;
  game.flushDirty();
}
