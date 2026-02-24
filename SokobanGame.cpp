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

constexpr int HUD_TITLE_X = 8;
constexpr int HUD_TITLE_Y = 8;
constexpr int HUD_LEVEL_X = 120;
constexpr int HUD_LEVEL_Y = 8;
constexpr int HUD_MOVES_X = 8;
constexpr int HUD_MOVES_Y = 24;
constexpr int HUD_TOTAL_X = 120;
constexpr int HUD_TOTAL_Y = 24;
constexpr int HUD_STATUS_X = 220;
constexpr int HUD_STATUS_Y = 24;

constexpr int OVERLAY_W = 224;
constexpr int OVERLAY_H = 52;
constexpr int OVERLAY_TEXT1_Y_OFF = 10;
constexpr int OVERLAY_TEXT2_Y_OFF = 30;

constexpr int FONT_W = 5;
constexpr int FONT_H = 7;
constexpr int BOX_SPRITE_SLOT_COUNT = SpriteLayer::kMaxSprites - 1;
constexpr int PLAYER_SPRITE_SLOT = SpriteLayer::kMaxSprites - 1;

int textHeight(int scale) {
  return FONT_H * scale;
}

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
    dirty(),
    flusher(dirty, MAX_TILE_W, MAX_TILE_H),
    sprites(),
    pinLeft(leftPin),
    pinRight(rightPin),
    pinUp(upPinValue),
    pinDown(downPinValue),
    pinFire(firePinValue),
    sceneSwitcher(),
    titleScene(*this),
    playingScene(*this),
    gameOverScene(*this) {
  buildSpritePixels();
  initSpriteSlots();
}

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

  dirty.clear();
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

  syncSpritesFromBoard();
  refreshHudTexts();
  refreshOverlayTexts();
  updateLevelSolvedState();
  invalidatePlayingScreen();
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

  int oldPlayerX = playerX;
  int oldPlayerY = playerY;
  int nx = playerX + dx;
  int ny = playerY + dy;
  if (!inBounds(nx, ny)) {
    return false;
  }

  bool pushed = false;
  int boxFromX = 0;
  int boxFromY = 0;
  int boxToX = 0;
  int boxToY = 0;

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
    pushed = true;
    boxFromX = nx;
    boxFromY = ny;
    boxToX = bx;
    boxToY = by;
    removeBoxAt(boxFromX, boxFromY);
    placeBoxAt(boxToX, boxToY);
  } else if (!isFreeForPlayer(next)) {
    return false;
  }

  clearPlayerAt(playerX, playerY);
  placePlayerAt(nx, ny);
  playerX = nx;
  playerY = ny;

  markCellDirty(oldPlayerX, oldPlayerY);
  markCellDirty(playerX, playerY);
  if (pushed) {
    markCellDirty(boxFromX, boxFromY);
    markCellDirty(boxToX, boxToY);
  }

  syncSpritesFromBoard();

  levelMoves++;
  totalMoves++;
  refreshHudTexts();
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
    refreshOverlayTexts();
    markOverlayDirty();
    refreshHudTexts();
  }
}

void SokobanGame::renderTitleScreen() {
  dirty.clear();
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

void SokobanGame::renderGameOverScreen() {
  dirty.clear();
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

void SokobanGame::refreshHudTexts() {
  char buf[24];

  snprintf(buf, sizeof(buf), "LVL %u/%u", (unsigned)(currentLevel + 1), (unsigned)LEVEL_COUNT);
  if (strcmp(hudLevelText, buf) != 0) {
    strncpy(hudLevelText, buf, sizeof(hudLevelText) - 1);
    hudLevelText[sizeof(hudLevelText) - 1] = '\0';
    markHudLevelDirty();
  }

  snprintf(buf, sizeof(buf), "MOVES %lu", (unsigned long)levelMoves);
  if (strcmp(hudMovesText, buf) != 0) {
    strncpy(hudMovesText, buf, sizeof(hudMovesText) - 1);
    hudMovesText[sizeof(hudMovesText) - 1] = '\0';
    markHudMovesDirty();
  }

  snprintf(buf, sizeof(buf), "TOTAL %lu", (unsigned long)totalMoves);
  if (strcmp(hudTotalText, buf) != 0) {
    strncpy(hudTotalText, buf, sizeof(hudTotalText) - 1);
    hudTotalText[sizeof(hudTotalText) - 1] = '\0';
    markHudTotalDirty();
  }

  const char* status = levelSolved ? "OK" : "FIRE=RESET";
  if (strcmp(hudStatusText, status) != 0) {
    strncpy(hudStatusText, status, sizeof(hudStatusText) - 1);
    hudStatusText[sizeof(hudStatusText) - 1] = '\0';
    markHudStatusDirty();
  }
}

void SokobanGame::refreshOverlayTexts() {
  overlayTitleText[0] = '\0';
  overlaySubText[0] = '\0';
  overlayTitleX = 0;
  overlaySubX = 0;
  if (!levelSolved) {
    return;
  }

  snprintf(overlayTitleText,
           sizeof(overlayTitleText),
           "PLANSZA %u OK",
           (unsigned)(currentLevel + 1));
  if (currentLevel + 1 < LEVEL_COUNT) {
    strncpy(overlaySubText, "KOLEJNA ZA CHWILE", sizeof(overlaySubText) - 1);
  } else {
    strncpy(overlaySubText, "KONIEC GRY", sizeof(overlaySubText) - 1);
  }
  overlaySubText[sizeof(overlaySubText) - 1] = '\0';

  overlayTitleX = (gfx.width() - Font5x7::textWidth(overlayTitleText, 2)) / 2;
  overlaySubX = (gfx.width() - Font5x7::textWidth(overlaySubText, 1)) / 2;
}

void SokobanGame::markHudLevelDirty() {
  markRectDirty(HUD_LEVEL_X - 1, HUD_LEVEL_Y - 1, 100, textHeight(2) + 2);
}

void SokobanGame::markHudMovesDirty() {
  markRectDirty(HUD_MOVES_X - 1, HUD_MOVES_Y - 1, 100, textHeight(1) + 2);
}

void SokobanGame::markHudTotalDirty() {
  markRectDirty(HUD_TOTAL_X - 1, HUD_TOTAL_Y - 1, 100, textHeight(1) + 2);
}

void SokobanGame::markHudStatusDirty() {
  markRectDirty(HUD_STATUS_X - 1, HUD_STATUS_Y - 1, 92, textHeight(1) + 2);
}

void SokobanGame::markOverlayDirty() {
  int x = (gfx.width() - OVERLAY_W) / 2;
  int y = (gfx.height() - OVERLAY_H) / 2;
  markRectDirty(x, y, OVERLAY_W, OVERLAY_H);
}

void SokobanGame::markCellDirty(int gx, int gy) {
  if (!inBounds(gx, gy)) {
    return;
  }
  markRectDirty(boardX0 + gx * TILE_SIZE, boardY0 + gy * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void SokobanGame::markBoardFrameDirty() {
  int x = boardX0 - 2;
  int y = boardY0 - 2;
  int w = boardW * TILE_SIZE + 4;
  int h = boardH * TILE_SIZE + 4;
  markRectDirty(x, y, w, h);
}

void SokobanGame::markRectDirty(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }
  dirty.add(x, y, x + w - 1, y + h - 1);
}

void SokobanGame::invalidatePlayingScreen() {
  dirty.invalidate(gfx);
}

void SokobanGame::flushDirty() {
  flusher.flush(gfx, regionBuf, [this](int x0, int y0, int w, int h, uint16_t* buf) {
    renderRegionToBuffer(x0, y0, w, h, buf);
  });
}

void SokobanGame::buildSpritePixels() {
  memset(boxSpritePixels, 0, sizeof(boxSpritePixels));
  memset(playerSpritePixels, 0, sizeof(playerSpritePixels));

  auto put = [](uint16_t* dst, int size, int x, int y, uint16_t c) {
    if (x < 0 || y < 0 || x >= size || y >= size) {
      return;
    }
    dst[y * size + x] = c;
  };

  for (int y = 2; y <= 13; y++) {
    for (int x = 2; x <= 13; x++) {
      uint16_t c = COLOR_BOX;
      if (y <= 3 || x <= 3) {
        c = COLOR_BOX_HI;
      } else if (y >= 12 || x >= 12) {
        c = COLOR_BOX_SH;
      }
      put(boxSpritePixels, SPRITE_SIZE, x, y, c);
    }
  }
  for (int y = 5; y <= 10; y++) {
    put(boxSpritePixels, SPRITE_SIZE, 7, y, COLOR_BOX_SH);
    put(boxSpritePixels, SPRITE_SIZE, 8, y, COLOR_BOX_SH);
  }
  for (int x = 5; x <= 10; x++) {
    put(boxSpritePixels, SPRITE_SIZE, x, 7, COLOR_BOX_SH);
    put(boxSpritePixels, SPRITE_SIZE, x, 8, COLOR_BOX_SH);
  }

  for (int y = 1; y <= 15; y++) {
    for (int x = 1; x <= 14; x++) {
      bool paint = false;
      uint16_t c = 0;
      if (y >= 1 && y <= 5 && x >= 5 && x <= 10) {
        paint = true;
        c = (y <= 2 || x <= 5) ? COLOR_PLAYER_HI : COLOR_PLAYER;
      } else if (y >= 6 && y <= 11 && x >= 4 && x <= 11) {
        paint = true;
        c = (x <= 5 || y <= 7) ? COLOR_PLAYER_HI : COLOR_PLAYER;
      } else if (y >= 12 && y <= 15 && ((x >= 3 && x <= 6) || (x >= 9 && x <= 12))) {
        paint = true;
        c = COLOR_PLAYER_SH;
      }
      if (paint) {
        put(playerSpritePixels, SPRITE_SIZE, x, y - 1, c);
      }
    }
  }
}

void SokobanGame::initSpriteSlots() {
  sprites.clearAll();
  for (int i = 0; i < BOX_SPRITE_SLOT_COUNT; i++) {
    auto& s = sprites.sprite(i);
    s.active = false;
    s.w = SPRITE_SIZE;
    s.h = SPRITE_SIZE;
    s.pixels565 = boxSpritePixels;
    s.transparent = 0;
    s.scale = SpriteLayer::Scale::Normal;
    s.setAnchor(0.0f, 0.0f);
  }

  auto& p = sprites.sprite(PLAYER_SPRITE_SLOT);
  p.active = false;
  p.w = SPRITE_SIZE;
  p.h = SPRITE_SIZE;
  p.pixels565 = playerSpritePixels;
  p.transparent = 0;
  p.scale = SpriteLayer::Scale::Normal;
  p.setAnchor(0.0f, 0.0f);
}

void SokobanGame::syncSpritesFromBoard() {
  for (int i = 0; i < BOX_SPRITE_SLOT_COUNT; i++) {
    sprites.sprite(i).active = false;
  }

  int slot = 0;
  for (int y = 0; y < boardH; y++) {
    for (int x = 0; x < boardW; x++) {
      if (!isBox(board[y][x])) {
        continue;
      }
      if (slot >= BOX_SPRITE_SLOT_COUNT) {
        continue;
      }
      auto& s = sprites.sprite(slot++);
      s.active = true;
      s.setPosition(boardX0 + x * TILE_SIZE + 2, boardY0 + y * TILE_SIZE + 2);
    }
  }

  auto& p = sprites.sprite(PLAYER_SPRITE_SLOT);
  p.active = true;
  p.setPosition(boardX0 + playerX * TILE_SIZE + 2, boardY0 + playerY * TILE_SIZE + 2);
}

uint16_t SokobanGame::pixelAt(int x, int y) const {
  if (x < 0 || x >= gfx.width() || y < 0 || y >= gfx.height()) {
    return COLOR_BG;
  }
  if (y < HUD_H) {
    return hudPixelAt(x, y);
  }
  return boardPixelAt(x, y);
}

uint16_t SokobanGame::hudPixelAt(int x, int y) const {
  if (y < 0 || y >= HUD_H) {
    return 0;
  }

  if (y >= HUD_H - 2) {
    return COLOR_PANEL_LINE;
  }

  uint16_t color = COLOR_PANEL;
  int ly = 0;
  int lx = 0;

  ly = y - HUD_TITLE_Y;
  lx = x - HUD_TITLE_X;
  if (Font5x7::textPixel("SOKOBAN", 2, lx, ly)) {
    return COLOR_ACCENT;
  }

  ly = y - HUD_LEVEL_Y;
  lx = x - HUD_LEVEL_X;
  if (Font5x7::textPixel(hudLevelText, 2, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_MOVES_Y;
  lx = x - HUD_MOVES_X;
  if (Font5x7::textPixel(hudMovesText, 1, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_TOTAL_Y;
  lx = x - HUD_TOTAL_X;
  if (Font5x7::textPixel(hudTotalText, 1, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_STATUS_Y;
  lx = x - HUD_STATUS_X;
  if (Font5x7::textPixel(hudStatusText, 1, lx, ly)) {
    return levelSolved ? COLOR_PLAYER_HI : COLOR_TEXT_DIM;
  }

  return color;
}

uint16_t SokobanGame::boardPixelAt(int x, int y) const {
  int frameX = boardX0 - 2;
  int frameY = boardY0 - 2;
  int frameW = boardW * TILE_SIZE + 4;
  int frameH = boardH * TILE_SIZE + 4;
  if (x >= frameX && x < frameX + frameW && y >= frameY && y < frameY + frameH) {
    bool onFrame = (x == frameX) || (x == frameX + frameW - 1) ||
                   (y == frameY) || (y == frameY + frameH - 1);
    if (onFrame) {
      return COLOR_PANEL_LINE;
    }
  }

  if (x < boardX0 || y < boardY0) {
    return COLOR_BG;
  }
  int rx = x - boardX0;
  int ry = y - boardY0;
  if (rx >= boardW * TILE_SIZE || ry >= boardH * TILE_SIZE) {
    return COLOR_BG;
  }

  int gx = rx / TILE_SIZE;
  int gy = ry / TILE_SIZE;
  int lx = rx - gx * TILE_SIZE;
  int ly = ry - gy * TILE_SIZE;
  return cellPixelAt(board[gy][gx], gx, gy, lx, ly);
}

uint16_t SokobanGame::cellPixelAt(char cell, int gx, int gy, int lx, int ly) const {
  (void)gx;
  (void)gy;
  bool hasTarget = (cell == '.' || cell == '*' || cell == '+');
  bool wall = (cell == '#');

  if (wall) {
    if (ly <= 1 || lx <= 1) {
      return COLOR_WALL_HI;
    }
    if (ly >= TILE_SIZE - 2 || lx >= TILE_SIZE - 2) {
      return COLOR_WALL_SH;
    }
    return COLOR_WALL;
  }

  uint16_t floorColor = (((gx + gy) & 1) == 0) ? COLOR_FLOOR_A : COLOR_FLOOR_B;
  if (lx == 0 || ly == 0) {
    return COLOR_GRID;
  }

  if (hasTarget) {
    bool outer = (lx >= 5 && lx <= TILE_SIZE - 6 && ly >= 5 && ly <= TILE_SIZE - 6);
    bool inner = (lx >= 7 && lx <= TILE_SIZE - 8 && ly >= 7 && ly <= TILE_SIZE - 8);
    bool hole = (lx >= 9 && lx <= TILE_SIZE - 10 && ly >= 9 && ly <= TILE_SIZE - 10);
    if (outer) {
      if (hole) {
        return floorColor;
      }
      return inner ? COLOR_TARGET_HI : COLOR_TARGET;
    }
  }

  return floorColor;
}

uint16_t SokobanGame::overlayPixelAt(int x, int y) const {
  if (!levelSolved) {
    return 0;
  }

  int ox = (gfx.width() - OVERLAY_W) / 2;
  int oy = (gfx.height() - OVERLAY_H) / 2;
  if (x < ox || x >= ox + OVERLAY_W || y < oy || y >= oy + OVERLAY_H) {
    return 0;
  }

  if (y < oy + 2 || y >= oy + OVERLAY_H - 2) {
    return COLOR_ACCENT;
  }

  int ly = y - (oy + OVERLAY_TEXT1_Y_OFF);
  int lx = x - overlayTitleX;
  if (Font5x7::textPixel(overlayTitleText, 2, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - (oy + OVERLAY_TEXT2_Y_OFF);
  lx = x - overlaySubX;
  if (Font5x7::textPixel(overlaySubText, 1, lx, ly)) {
    return COLOR_TEXT_DIM;
  }

  return COLOR_OVERLAY;
}

void SokobanGame::renderRegionToBuffer(int x0, int y0, int w, int h, uint16_t* buf) {
  for (int yy = 0; yy < h; yy++) {
    int y = y0 + yy;
    for (int xx = 0; xx < w; xx++) {
      int x = x0 + xx;
      buf[yy * w + xx] = pixelAt(x, y);
    }
  }

  sprites.renderRegion(x0, y0, w, h, buf);

  if (levelSolved) {
    for (int yy = 0; yy < h; yy++) {
      int y = y0 + yy;
      for (int xx = 0; xx < w; xx++) {
        int x = x0 + xx;
        uint16_t c = overlayPixelAt(x, y);
        if (c != 0) {
          buf[yy * w + xx] = c;
        }
      }
    }
  }
}
