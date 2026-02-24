#pragma once

#include <stdint.h>

#include "SGF/Actions.h"
#include "SGF/Color565.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"
#include "SGF/Font5x7.h"
#include "SGF/Game.h"
#include "SGF/InputPin.h"
#include "SGF/Scene.h"
#include "SGF/Sprites.h"
#include "SGF/TileFlusher.h"
#include "GameOverScene.h"
#include "PlayingScene.h"
#include "TitleScene.h"

class SokobanGame : public Game {
public:
  SokobanGame(FastILI9341& gfxRef,
              uint8_t leftPin,
              uint8_t rightPin,
              uint8_t upPin,
              uint8_t downPin,
              uint8_t firePin);

  void setup();

private:
  struct LevelDef {
    uint8_t width = 0;
    uint8_t height = 0;
    const char* const* rows = nullptr;
  };

  static constexpr uint32_t FRAME_DEFAULT_STEP_US = 10000u;
  static constexpr uint32_t FRAME_MAX_STEP_US = 30000u;
  static constexpr uint32_t DEFAULT_SPI_HZ = 24000000u;
  static constexpr FastILI9341::ScreenRotation DEFAULT_ROTATION =
      FastILI9341::ScreenRotation::Landscape;

  static constexpr int SCREEN_W = 320;
  static constexpr int SCREEN_H = 240;
  static constexpr int TILE_SIZE = 20;
  static constexpr int SPRITE_SIZE = 16;
  static constexpr int BOARD_MAX_W = 14;
  static constexpr int BOARD_MAX_H = 10;
  static constexpr int HUD_H = 44;
  static constexpr int MAX_TILE_W = 64;
  static constexpr int MAX_TILE_H = 64;
  static constexpr uint8_t LEVEL_COUNT = 10;
  static constexpr float LEVEL_SOLVED_DELAY_S = 0.75f;

  static constexpr uint16_t COLOR_BG = Color565::rgb(8, 12, 18);
  static constexpr uint16_t COLOR_PANEL = Color565::rgb(14, 22, 32);
  static constexpr uint16_t COLOR_PANEL_LINE = Color565::rgb(42, 64, 82);
  static constexpr uint16_t COLOR_TEXT = Color565::rgb(228, 236, 244);
  static constexpr uint16_t COLOR_TEXT_DIM = Color565::rgb(140, 160, 176);
  static constexpr uint16_t COLOR_ACCENT = Color565::rgb(255, 196, 96);
  static constexpr uint16_t COLOR_WALL = Color565::rgb(54, 74, 98);
  static constexpr uint16_t COLOR_WALL_HI = Color565::rgb(86, 116, 150);
  static constexpr uint16_t COLOR_WALL_SH = Color565::rgb(30, 42, 58);
  static constexpr uint16_t COLOR_FLOOR_A = Color565::rgb(18, 26, 34);
  static constexpr uint16_t COLOR_FLOOR_B = Color565::rgb(14, 22, 30);
  static constexpr uint16_t COLOR_GRID = Color565::rgb(24, 36, 46);
  static constexpr uint16_t COLOR_TARGET = Color565::rgb(232, 96, 96);
  static constexpr uint16_t COLOR_TARGET_HI = Color565::rgb(255, 188, 160);
  static constexpr uint16_t COLOR_BOX = Color565::rgb(188, 136, 72);
  static constexpr uint16_t COLOR_BOX_HI = Color565::rgb(236, 188, 108);
  static constexpr uint16_t COLOR_BOX_SH = Color565::rgb(120, 84, 44);
  static constexpr uint16_t COLOR_PLAYER = Color565::rgb(92, 220, 148);
  static constexpr uint16_t COLOR_PLAYER_HI = Color565::rgb(156, 255, 196);
  static constexpr uint16_t COLOR_PLAYER_SH = Color565::rgb(44, 122, 78);
  static constexpr uint16_t COLOR_OVERLAY = Color565::rgb(20, 28, 40);
  static constexpr uint16_t COLOR_GO_BG = Color565::rgb(14, 8, 10);
  static constexpr uint16_t COLOR_GO_LINE = Color565::rgb(110, 34, 34);
  static constexpr uint16_t COLOR_GO_TITLE = Color565::rgb(255, 112, 112);

  FastILI9341& gfx;
  DirtyRects dirty;
  TileFlusher flusher;
  SpriteLayer sprites;
  uint16_t regionBuf[MAX_TILE_W * MAX_TILE_H]{};
  uint16_t boxSpritePixels[SPRITE_SIZE * SPRITE_SIZE]{};
  uint16_t playerSpritePixels[SPRITE_SIZE * SPRITE_SIZE]{};
  uint8_t pinLeft = 0;
  uint8_t pinRight = 0;
  uint8_t pinUp = 0;
  uint8_t pinDown = 0;
  uint8_t pinFire = 0;
  DebouncedInputPin leftPinInput;
  DebouncedInputPin rightPinInput;
  DebouncedInputPin upPinInput;
  DebouncedInputPin downPinInput;
  DebouncedInputPin firePinInput;

  DigitalAction leftAction;
  DigitalAction rightAction;
  DigitalAction upAction;
  DigitalAction downAction;
  DigitalAction fireAction;
  PressReleaseAction fireConfirm;

  SceneSwitcher sceneSwitcher;
  TitleScene titleScene;
  PlayingScene playingScene;
  GameOverScene gameOverScene;

  char board[BOARD_MAX_H][BOARD_MAX_W]{};
  int boardW = 0;
  int boardH = 0;
  int boardX0 = 0;
  int boardY0 = 0;
  int playerX = 0;
  int playerY = 0;
  int remainingCrates = 0;

  uint8_t currentLevel = 0;
  uint8_t completedLevels = 0;
  uint32_t totalMoves = 0;
  uint32_t levelMoves = 0;
  uint32_t finalMoves = 0;

  bool levelSolved = false;
  float levelSolvedTimer = 0.0f;
  char hudLevelText[12]{};
  char hudMovesText[16]{};
  char hudTotalText[16]{};
  char hudStatusText[16]{};
  char overlayTitleText[24]{};
  char overlaySubText[24]{};
  int overlayTitleX = 0;
  int overlaySubX = 0;

  friend class TitleScene;
  friend class PlayingScene;
  friend class GameOverScene;

  static const LevelDef LEVELS[LEVEL_COUNT];

  void onSetup() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

  void startNewGame();
  void loadLevel(uint8_t levelIndex);
  void advanceAfterLevelSolved();

  bool tryMove(int dx, int dy);
  bool inBounds(int x, int y) const;
  static bool isBox(char cell);
  static bool isFreeForPlayer(char cell);
  static bool isFreeForBox(char cell);
  void clearPlayerAt(int x, int y);
  void placePlayerAt(int x, int y);
  void removeBoxAt(int x, int y);
  void placeBoxAt(int x, int y);
  void updateLevelSolvedState();

  void renderTitleScreen();
  void renderGameOverScreen();
  void refreshHudTexts();
  void refreshOverlayTexts();
  void markHudLevelDirty();
  void markHudMovesDirty();
  void markHudTotalDirty();
  void markHudStatusDirty();
  void markOverlayDirty();
  void markCellDirty(int gx, int gy);
  void markBoardFrameDirty();
  void markRectDirty(int x, int y, int w, int h);
  void invalidatePlayingScreen();
  void flushDirty();
  void buildSpritePixels();
  void initSpriteSlots();
  void syncSpritesFromBoard();
  uint16_t pixelAt(int x, int y) const;
  uint16_t hudPixelAt(int x, int y) const;
  uint16_t boardPixelAt(int x, int y) const;
  uint16_t cellPixelAt(char cell, int gx, int gy, int lx, int ly) const;
  uint16_t overlayPixelAt(int x, int y) const;
  void renderRegionToBuffer(int x0, int y0, int w, int h, uint16_t* buf);
};
