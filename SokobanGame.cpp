#include "SokobanGame.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {
const char* const LEVEL1_ROWS[] = {
  "########",
  "#  .   #",
  "#  $   #",
  "#  @   #",
  "#      #",
  "#      #",
  "########"
};

const char* const LEVEL2_ROWS[] = {
  "##########",
  "# .   .  #",
  "# $   $  #",
  "#   @    #",
  "#        #",
  "##########"
};

const char* const LEVEL3_ROWS[] = {
  "############",
  "# .   .   .#",
  "# $   $   $#",
  "#     @    #",
  "#          #",
  "#          #",
  "############"
};
}  // namespace

const SokobanGame::LevelDef SokobanGame::LEVELS[LEVEL_COUNT] = {
  {8, 7, LEVEL1_ROWS},
  {10, 6, LEVEL2_ROWS},
  {12, 7, LEVEL3_ROWS},
};

SokobanGame::SokobanGame(FastILI9341& gfxRef,
                         uint8_t leftPin,
                         uint8_t rightPin,
                         uint8_t upPinValue,
                         uint8_t downPinValue,
                         uint8_t firePinValue)
  : Game(FRAME_DEFAULT_STEP_US, FRAME_MAX_STEP_US),
    gfx(gfxRef),
    pinLeft(leftPin),
    pinRight(rightPin),
    pinUp(upPinValue),
    pinDown(downPinValue),
    pinFire(firePinValue),
    sceneSwitcher(),
    titleScene(*this),
    playingScene(*this),
    gameOverScene(*this) {}

void SokobanGame::setup() {
  start();
}

void SokobanGame::onSetup() {
  pinMode(pinLeft, INPUT_PULLUP);
  pinMode(pinRight, INPUT_PULLUP);
  pinMode(pinUp, INPUT_PULLUP);
  pinMode(pinDown, INPUT_PULLUP);
  pinMode(pinFire, INPUT_PULLUP);

  leftAction.reset(digitalRead(pinLeft) == LOW);
  rightAction.reset(digitalRead(pinRight) == LOW);
  upAction.reset(digitalRead(pinUp) == LOW);
  downAction.reset(digitalRead(pinDown) == LOW);
  fireAction.reset(digitalRead(pinFire) == LOW);
  fireConfirm.reset();

  bool ok = gfx.begin(DEFAULT_SPI_HZ);
  if (!ok) {
    while (1) {
      delay(1000);
    }
  }
  gfx.screenRotation(DEFAULT_ROTATION);

  sceneSwitcher.setInitial(titleScene);
  resetClock();
}

void SokobanGame::onPhysics(float delta) {
  leftAction.update(digitalRead(pinLeft) == LOW);
  rightAction.update(digitalRead(pinRight) == LOW);
  upAction.update(digitalRead(pinUp) == LOW);
  downAction.update(digitalRead(pinDown) == LOW);
  fireAction.update(digitalRead(pinFire) == LOW);
  sceneSwitcher.onPhysics(delta);
}

void SokobanGame::onProcess(float delta) {
  sceneSwitcher.onProcess(delta);
}

void SokobanGame::startNewGame() {
  currentLevel = 0;
  completedLevels = 0;
  totalMoves = 0;
  finalMoves = 0;
  loadLevel(currentLevel);
}

void SokobanGame::loadLevel(uint8_t levelIndex) {
  if (levelIndex >= LEVEL_COUNT) {
    return;
  }

  memset(board, ' ', sizeof(board));
  boardW = LEVELS[levelIndex].width;
  boardH = LEVELS[levelIndex].height;
  currentLevel = levelIndex;
  levelMoves = 0;
  remainingCrates = 0;
  levelSolved = false;
  levelSolvedTimer = 0.0f;

  bool playerFound = false;
  for (int y = 0; y < boardH; y++) {
    const char* row = LEVELS[levelIndex].rows[y];
    for (int x = 0; x < boardW; x++) {
      char cell = row[x];
      if (cell == '\0') {
        cell = ' ';
      }
      board[y][x] = cell;
      if (cell == '@' || cell == '+') {
        playerX = x;
        playerY = y;
        playerFound = true;
      }
      if (cell == '$') {
        remainingCrates++;
      }
    }
  }

  if (!playerFound) {
    playerX = 1;
    playerY = 1;
    if (inBounds(playerX, playerY) && board[playerY][playerX] == ' ') {
      board[playerY][playerX] = '@';
    }
  }

  int boardPxW = boardW * TILE_SIZE;
  int boardPxH = boardH * TILE_SIZE;
  boardX0 = (gfx.width() - boardPxW) / 2;
  if (boardX0 < 4) {
    boardX0 = 4;
  }
  int contentTop = HUD_H + 4;
  int contentH = gfx.height() - contentTop - 4;
  boardY0 = contentTop + (contentH - boardPxH) / 2;
  if (boardY0 < contentTop) {
    boardY0 = contentTop;
  }

  updateLevelSolvedState();
  setNeedsRedraw();
}

void SokobanGame::advanceAfterLevelSolved() {
  completedLevels = currentLevel + 1;
  if (completedLevels < LEVEL_COUNT) {
    loadLevel(completedLevels);
    return;
  }

  finalMoves = totalMoves;
  sceneSwitcher.switchTo(gameOverScene);
  resetClock();
}

bool SokobanGame::tryMove(int dx, int dy) {
  if (dx == 0 && dy == 0) {
    return false;
  }
  if (levelSolved) {
    return false;
  }

  int nx = playerX + dx;
  int ny = playerY + dy;
  if (!inBounds(nx, ny)) {
    return false;
  }

  char next = board[ny][nx];
  if (next == '#') {
    return false;
  }

  if (isBox(next)) {
    int bx = nx + dx;
    int by = ny + dy;
    if (!inBounds(bx, by)) {
      return false;
    }
    char beyond = board[by][bx];
    if (!isFreeForBox(beyond)) {
      return false;
    }
    removeBoxAt(nx, ny);
    placeBoxAt(bx, by);
  } else if (!isFreeForPlayer(next)) {
    return false;
  }

  clearPlayerAt(playerX, playerY);
  placePlayerAt(nx, ny);
  playerX = nx;
  playerY = ny;

  levelMoves++;
  totalMoves++;
  updateLevelSolvedState();
  return true;
}

bool SokobanGame::inBounds(int x, int y) const {
  return x >= 0 && x < boardW && y >= 0 && y < boardH;
}

bool SokobanGame::isBox(char cell) {
  return cell == '$' || cell == '*';
}

bool SokobanGame::isFreeForPlayer(char cell) {
  return cell == ' ' || cell == '.';
}

bool SokobanGame::isFreeForBox(char cell) {
  return cell == ' ' || cell == '.';
}

void SokobanGame::clearPlayerAt(int x, int y) {
  char& cell = board[y][x];
  if (cell == '@') {
    cell = ' ';
  } else if (cell == '+') {
    cell = '.';
  }
}

void SokobanGame::placePlayerAt(int x, int y) {
  char& cell = board[y][x];
  if (cell == ' ') {
    cell = '@';
  } else if (cell == '.') {
    cell = '+';
  }
}

void SokobanGame::removeBoxAt(int x, int y) {
  char& cell = board[y][x];
  if (cell == '$') {
    cell = ' ';
  } else if (cell == '*') {
    cell = '.';
    remainingCrates++;
  }
}

void SokobanGame::placeBoxAt(int x, int y) {
  char& cell = board[y][x];
  if (cell == ' ') {
    cell = '$';
  } else if (cell == '.') {
    cell = '*';
    remainingCrates--;
  }
}

void SokobanGame::updateLevelSolvedState() {
  if (!levelSolved && remainingCrates == 0) {
    levelSolved = true;
    levelSolvedTimer = 0.0f;
    setNeedsRedraw();
  }
}

void SokobanGame::renderTitleScreen() {
  gfx.fillScreen565(COLOR_BG);
  gfx.fillRect565(14, 16, gfx.width() - 28, 4, COLOR_ACCENT);
  gfx.fillRect565(14, 24, gfx.width() - 28, 2, COLOR_PANEL_LINE);
  gfx.fillRect565(14, gfx.height() - 26, gfx.width() - 28, 2, COLOR_PANEL_LINE);
  gfx.fillRect565(14, gfx.height() - 18, gfx.width() - 28, 4, COLOR_ACCENT);

  gfx.drawCenteredText(46, "UNOQ SOKOBAN", 4, COLOR_TEXT);
  gfx.drawCenteredText(90, "3 PLANSZE", 2, COLOR_ACCENT);
  gfx.drawCenteredText(118, "L/R/U/D - RUCH", 2, COLOR_TEXT);
  gfx.drawCenteredText(144, "FIRE - START", 2, COLOR_TEXT);
  gfx.drawCenteredText(170, "FIRE W GRZE - RESTART", 1, COLOR_TEXT_DIM);
  gfx.drawCenteredText(194, "PRZENIES SKRZYNKI NA CELE", 1, COLOR_TEXT_DIM);
}

void SokobanGame::renderPlayingScreen() {
  gfx.fillScreen565(COLOR_BG);
  renderHud();

  for (int y = 0; y < boardH; y++) {
    for (int x = 0; x < boardW; x++) {
      drawCell(x, y, board[y][x]);
    }
  }

  int frameX = boardX0 - 2;
  int frameY = boardY0 - 2;
  int frameW = boardW * TILE_SIZE + 4;
  int frameH = boardH * TILE_SIZE + 4;
  gfx.fillRect565(frameX, frameY, frameW, 1, COLOR_PANEL_LINE);
  gfx.fillRect565(frameX, frameY + frameH - 1, frameW, 1, COLOR_PANEL_LINE);
  gfx.fillRect565(frameX, frameY, 1, frameH, COLOR_PANEL_LINE);
  gfx.fillRect565(frameX + frameW - 1, frameY, 1, frameH, COLOR_PANEL_LINE);

  if (levelSolved) {
    int w = 224;
    int h = 52;
    int x = (gfx.width() - w) / 2;
    int y = (gfx.height() - h) / 2;
    char lineBuf[32];
    snprintf(lineBuf, sizeof(lineBuf), "PLANSZA %u OK", (unsigned)(currentLevel + 1));
    gfx.fillRect565(x, y, w, h, COLOR_OVERLAY);
    gfx.fillRect565(x, y, w, 2, COLOR_ACCENT);
    gfx.fillRect565(x, y + h - 2, w, 2, COLOR_ACCENT);
    gfx.drawCenteredText(y + 10, lineBuf, 2, COLOR_TEXT);
    if (currentLevel + 1 < LEVEL_COUNT) {
      gfx.drawCenteredText(y + 30, "KOLEJNA ZA CHWILE", 1, COLOR_TEXT_DIM);
    } else {
      gfx.drawCenteredText(y + 30, "KONIEC GRY", 1, COLOR_TEXT_DIM);
    }
  }
}

void SokobanGame::renderGameOverScreen() {
  char movesBuf[24];
  char levelsBuf[24];
  snprintf(movesBuf, sizeof(movesBuf), "%lu", (unsigned long)finalMoves);
  snprintf(levelsBuf, sizeof(levelsBuf), "%u / %u", (unsigned)LEVEL_COUNT, (unsigned)LEVEL_COUNT);

  gfx.fillScreen565(COLOR_GO_BG);
  gfx.fillRect565(18, 18, gfx.width() - 36, 3, COLOR_GO_LINE);
  gfx.fillRect565(18, gfx.height() - 21, gfx.width() - 36, 3, COLOR_GO_LINE);

  gfx.drawCenteredText(48, "GAME OVER", 4, COLOR_GO_TITLE);
  gfx.drawCenteredText(96, "UKONCZONE PLANSZE", 1, COLOR_TEXT_DIM);
  gfx.drawCenteredText(112, levelsBuf, 3, COLOR_TEXT);
  gfx.drawCenteredText(152, "RUCHY", 1, COLOR_TEXT_DIM);
  gfx.drawCenteredText(168, movesBuf, 3, COLOR_ACCENT);
  gfx.drawCenteredText(206, "FIRE - MENU", 2, COLOR_TEXT);
}

void SokobanGame::renderHud() {
  char levelBuf[24];
  char movesBuf[24];
  char totalBuf[24];
  snprintf(levelBuf,
           sizeof(levelBuf),
           "LVL %u/%u",
           (unsigned)(currentLevel + 1),
           (unsigned)LEVEL_COUNT);
  snprintf(movesBuf, sizeof(movesBuf), "MOVES %lu", (unsigned long)levelMoves);
  snprintf(totalBuf, sizeof(totalBuf), "TOTAL %lu", (unsigned long)totalMoves);

  gfx.fillRect565(0, 0, gfx.width(), HUD_H, COLOR_PANEL);
  gfx.fillRect565(0, HUD_H - 2, gfx.width(), 2, COLOR_PANEL_LINE);
  gfx.drawText(8, 8, "SOKOBAN", 2, COLOR_ACCENT);
  gfx.drawText(120, 8, levelBuf, 2, COLOR_TEXT);
  gfx.drawText(8, 24, movesBuf, 1, COLOR_TEXT);
  gfx.drawText(120, 24, totalBuf, 1, COLOR_TEXT);
  if (levelSolved) {
    gfx.drawText(230, 24, "OK", 1, COLOR_PLAYER_HI);
  } else {
    gfx.drawText(220, 24, "FIRE=RESET", 1, COLOR_TEXT_DIM);
  }
}

void SokobanGame::drawCell(int gx, int gy, char cell) {
  int x = boardX0 + gx * TILE_SIZE;
  int y = boardY0 + gy * TILE_SIZE;

  if (cell == '#') {
    gfx.fillRect565(x, y, TILE_SIZE, TILE_SIZE, COLOR_WALL);
    gfx.fillRect565(x, y, TILE_SIZE, 2, COLOR_WALL_HI);
    gfx.fillRect565(x, y, 2, TILE_SIZE, COLOR_WALL_HI);
    gfx.fillRect565(x, y + TILE_SIZE - 2, TILE_SIZE, 2, COLOR_WALL_SH);
    gfx.fillRect565(x + TILE_SIZE - 2, y, 2, TILE_SIZE, COLOR_WALL_SH);
    return;
  }

  uint16_t floorColor = (((gx + gy) & 1) == 0) ? COLOR_FLOOR_A : COLOR_FLOOR_B;
  gfx.fillRect565(x, y, TILE_SIZE, TILE_SIZE, floorColor);
  gfx.fillRect565(x, y, TILE_SIZE, 1, COLOR_GRID);
  gfx.fillRect565(x, y, 1, TILE_SIZE, COLOR_GRID);

  bool hasTarget = (cell == '.' || cell == '*' || cell == '+');
  if (hasTarget) {
    int tx = x + 5;
    int ty = y + 5;
    gfx.fillRect565(tx, ty, TILE_SIZE - 10, TILE_SIZE - 10, COLOR_TARGET);
    gfx.fillRect565(tx + 2, ty + 2, TILE_SIZE - 14, TILE_SIZE - 14, COLOR_TARGET_HI);
    gfx.fillRect565(tx + 4, ty + 4, TILE_SIZE - 18, TILE_SIZE - 18, floorColor);
  }

  if (cell == '$' || cell == '*') {
    int bx = x + 3;
    int by = y + 3;
    int bw = TILE_SIZE - 6;
    int bh = TILE_SIZE - 6;
    gfx.fillRect565(bx, by, bw, bh, COLOR_BOX);
    gfx.fillRect565(bx, by, bw, 2, COLOR_BOX_HI);
    gfx.fillRect565(bx, by, 2, bh, COLOR_BOX_HI);
    gfx.fillRect565(bx, by + bh - 2, bw, 2, COLOR_BOX_SH);
    gfx.fillRect565(bx + bw - 2, by, 2, bh, COLOR_BOX_SH);
    gfx.fillRect565(bx + bw / 2 - 1, by + 3, 2, bh - 6, COLOR_BOX_SH);
    gfx.fillRect565(bx + 3, by + bh / 2 - 1, bw - 6, 2, COLOR_BOX_SH);
  }

  if (cell == '@' || cell == '+') {
    int px = x + 4;
    int py = y + 2;
    gfx.fillRect565(px + 4, py, 8, 5, COLOR_PLAYER_HI);
    gfx.fillRect565(px + 3, py + 5, 10, 3, COLOR_PLAYER);
    gfx.fillRect565(px + 2, py + 8, 12, 6, COLOR_PLAYER);
    gfx.fillRect565(px + 1, py + 14, 5, 3, COLOR_PLAYER_SH);
    gfx.fillRect565(px + 10, py + 14, 5, 3, COLOR_PLAYER_SH);
  }
}

void SokobanGame::setNeedsRedraw() {
  needsRedraw = true;
}
