#include "SokobanGame.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {

void fillRectOnDisplay(void* ctx, int x, int y, int w, int h, uint16_t color565) {
  static_cast<IScreen*>(ctx)->fillRect565(x, y, w, h, color565);
}

const char* const LEVEL1_ROWS[] = {
  "#####",
  "#@$.#",
  "#####"
};

const char* const LEVEL2_ROWS[] = {
  "  ####",
  "###  ####",
  "#     $ #",
  "# #  #$ #",
  "# . .#@ #",
  "#########"
};

const char* const LEVEL3_ROWS[] = {
  "########",
  "#      #",
  "# .**$@#",
  "#      #",
  "#####  #",
  "    ####"
};

const char* const LEVEL4_ROWS[] = {
  " #######",
  " #     #",
  " # .$. #",
  "## $@$ #",
  "#  .$. #",
  "#      #",
  "########"
};

const char* const LEVEL5_ROWS[] = {
  "###### #####",
  "#    ###   #",
  "# $$     #@#",
  "# $ #...   #",
  "#   ########",
  "#####"
};

const char* const LEVEL6_ROWS[] = {
  "####",
  "# .#",
  "#  ###",
  "#*@  #",
  "#  $ #",
  "#  ###",
  "####"
};

const char* const LEVEL7_ROWS[] = {
  "######",
  "#    #",
  "# #@ #",
  "# $* #",
  "# .* #",
  "#    #",
  "######"
};

const char* const LEVEL8_ROWS[] = {
  "#######",
  "#     #",
  "# .$. #",
  "# $.$ #",
  "# .$. #",
  "# $.$ #",
  "#  @  #",
  "#######"
};

const char* const LEVEL9_ROWS[] = {
  "#####",
  "#.  ##",
  "#@$$ #",
  "##   #",
  " ##  #",
  "  ##.#",
  "   ###"
};

const char* const LEVEL10_ROWS[] = {
  "      #####",
  "      #.  #",
  "      #.# #",
  "#######.# #",
  "# @ $ $ $ #",
  "# # # # ###",
  "#       #",
  "#########"
};

constexpr int HUD_TITLE_Y = 8;
constexpr int HUD_LEVEL_Y = 8;
constexpr int HUD_MOVES_Y = 24;
constexpr int HUD_TOTAL_Y = 24;
constexpr int HUD_STATUS_Y = 24;
constexpr int OVERLAY_TEXT1_Y_OFF = 10;
constexpr int OVERLAY_TEXT2_Y_OFF = 30;

constexpr int BOX_SPRITE_SLOT_COUNT = SpriteLayer::kMaxSprites - 1;
constexpr int PLAYER_SPRITE_SLOT = SpriteLayer::kMaxSprites - 1;

int fitCenteredScale(int screenWidth, const char* text, int maxScale, int margin) {
  for (int scale = maxScale; scale >= 1; --scale) {
    if (Font5x7::textWidth(text, scale) <= screenWidth - margin * 2) {
      return scale;
    }
  }
  return 1;
}

}  // namespace

const SokobanGame::LevelDef SokobanGame::LEVELS[LEVEL_COUNT] = {
  {5, 3, LEVEL1_ROWS},
  {9, 6, LEVEL2_ROWS},
  {8, 6, LEVEL3_ROWS},
  {8, 7, LEVEL4_ROWS},
  {12, 6, LEVEL5_ROWS},
  {6, 7, LEVEL6_ROWS},
  {6, 7, LEVEL7_ROWS},
  {7, 8, LEVEL8_ROWS},
  {6, 7, LEVEL9_ROWS},
  {11, 8, LEVEL10_ROWS},
};

SokobanGame::SokobanGame(
  IRenderTarget& renderTargetRef,
  IScreen& screenRef,
  const SGFHardware::HardwareProfile& hardwareProfileIn)
  : Game(FRAME_DEFAULT_STEP_US, FRAME_MAX_STEP_US),
    renderTarget(renderTargetRef),
    screen(screenRef),
    hardwareProfile(hardwareProfileIn),
    dirty(),
    flusher(dirty, MAX_TILE_W, MAX_TILE_H),
    sprites(),
    sceneSwitcher(),
    titleScene(*this),
    playingScene(*this),
    gameOverScene(*this) {
  pinLeft = hardwareProfile.input.left;
  pinRight = hardwareProfile.input.right;
  pinUp = hardwareProfile.input.up;
  pinDown = hardwareProfile.input.down;
  pinFire = hardwareProfile.input.fire;

  buildSpritePixels();
  initSpriteSlots();
}

void SokobanGame::setup() {
  start();
}

void SokobanGame::onSetup() {
  leftPinInput.attach(pinLeft, true);
  rightPinInput.attach(pinRight, true);
  upPinInput.attach(pinUp, true);
  downPinInput.attach(pinDown, true);
  firePinInput.attach(pinFire, true);
  leftPinInput.begin(INPUT_PULLUP);
  rightPinInput.begin(INPUT_PULLUP);
  upPinInput.begin(INPUT_PULLUP);
  downPinInput.begin(INPUT_PULLUP);
  firePinInput.begin(INPUT_PULLUP);

  leftPinInput.resetFromPin();
  rightPinInput.resetFromPin();
  upPinInput.resetFromPin();
  downPinInput.resetFromPin();
  firePinInput.resetFromPin();

  leftAction.reset(leftPinInput.pressed());
  rightAction.reset(rightPinInput.pressed());
  upAction.reset(upPinInput.pressed());
  downAction.reset(downPinInput.pressed());
  fireAction.reset(firePinInput.pressed());
  fireConfirm.reset();

  dirty.clear();
  sceneSwitcher.setInitial(titleScene);
  resetClock();
}

void SokobanGame::onPhysics(float delta) {
  leftAction.update(leftPinInput.update());
  rightAction.update(rightPinInput.update());
  upAction.update(upPinInput.update());
  downAction.update(downPinInput.update());
  fireAction.update(firePinInput.update());
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
    size_t rowLen = strlen(row);
    for (int x = 0; x < boardW; x++) {
      char cell = (x < (int)rowLen) ? row[x] : ' ';
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

  updateBoardLayout();
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
  const int titleScale = fitCenteredScale(renderTarget.width(), "UNOQ SOKOBAN", 4, 12);

  dirty.clear();
  screen.fillScreen565(COLOR_BG);
  screen.fillRect565(14, 16, renderTarget.width() - 28, 4, COLOR_ACCENT);
  screen.fillRect565(14, 24, renderTarget.width() - 28, 2, COLOR_PANEL_LINE);
  screen.fillRect565(
    14, renderTarget.height() - 26, renderTarget.width() - 28, 2, COLOR_PANEL_LINE);
  screen.fillRect565(
    14, renderTarget.height() - 18, renderTarget.width() - 28, 4, COLOR_ACCENT);

  Font5x7::drawCenteredText(
    renderTarget.width(), 46, "UNOQ SOKOBAN", titleScale, COLOR_TEXT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 90, "10 PLANSZ", 2, COLOR_ACCENT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 118, "L/R/U/D - RUCH", 2, COLOR_TEXT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 144, "FIRE - START", 2, COLOR_TEXT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 170, "FIRE W GRZE - RESTART", 1, COLOR_TEXT_DIM,
    &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 194, "PRZENIES SKRZYNKI NA CELE", 1, COLOR_TEXT_DIM,
    &screen, fillRectOnDisplay);
}

void SokobanGame::renderGameOverScreen() {
  dirty.clear();
  char movesBuf[24];
  char levelsBuf[24];
  const int titleScale = fitCenteredScale(renderTarget.width(), "GAME OVER", 4, 12);
  snprintf(movesBuf, sizeof(movesBuf), "%lu", (unsigned long)finalMoves);
  snprintf(levelsBuf, sizeof(levelsBuf), "%u / %u", (unsigned)LEVEL_COUNT, (unsigned)LEVEL_COUNT);

  screen.fillScreen565(COLOR_GO_BG);
  screen.fillRect565(18, 18, renderTarget.width() - 36, 3, COLOR_GO_LINE);
  screen.fillRect565(18, renderTarget.height() - 21, renderTarget.width() - 36, 3, COLOR_GO_LINE);

  Font5x7::drawCenteredText(
    renderTarget.width(), 48, "GAME OVER", titleScale, COLOR_GO_TITLE, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 96, "UKONCZONE PLANSZE", 1, COLOR_TEXT_DIM,
    &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 112, levelsBuf, 3, COLOR_TEXT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 152, "RUCHY", 1, COLOR_TEXT_DIM, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 168, movesBuf, 3, COLOR_ACCENT, &screen, fillRectOnDisplay);
  Font5x7::drawCenteredText(
    renderTarget.width(), 206, "FIRE - MENU", 2, COLOR_TEXT, &screen, fillRectOnDisplay);
}

void SokobanGame::refreshHudTexts() {
  char buf[24];
  bool changed = false;

  snprintf(buf, sizeof(buf), "LVL %u/%u", (unsigned)(currentLevel + 1), (unsigned)LEVEL_COUNT);
  if (strcmp(hudLevelText, buf) != 0) {
    strncpy(hudLevelText, buf, sizeof(hudLevelText) - 1);
    hudLevelText[sizeof(hudLevelText) - 1] = '\0';
    changed = true;
  }

  snprintf(buf, sizeof(buf), "MOVES %lu", (unsigned long)levelMoves);
  if (strcmp(hudMovesText, buf) != 0) {
    strncpy(hudMovesText, buf, sizeof(hudMovesText) - 1);
    hudMovesText[sizeof(hudMovesText) - 1] = '\0';
    changed = true;
  }

  snprintf(buf, sizeof(buf), "TOTAL %lu", (unsigned long)totalMoves);
  if (strcmp(hudTotalText, buf) != 0) {
    strncpy(hudTotalText, buf, sizeof(hudTotalText) - 1);
    hudTotalText[sizeof(hudTotalText) - 1] = '\0';
    changed = true;
  }

  const char* status = levelSolved ? "OK" : "FIRE=RESET";
  if (strcmp(hudStatusText, status) != 0) {
    strncpy(hudStatusText, status, sizeof(hudStatusText) - 1);
    hudStatusText[sizeof(hudStatusText) - 1] = '\0';
    changed = true;
  }

  if (changed) {
    updateHudLayout();
    markHudDirty();
  }
}

void SokobanGame::refreshOverlayTexts() {
  overlayTitleText[0] = '\0';
  overlaySubText[0] = '\0';
  if (!levelSolved) {
    updateOverlayLayout();
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

  updateOverlayLayout();
}

void SokobanGame::updateBoardLayout() {
  int maxTileW = (renderTarget.width() - 8) / boardW;
  int maxTileH = (renderTarget.height() - HUD_H - 8) / boardH;
  tileSize = maxTileW;
  if (maxTileH < tileSize) {
    tileSize = maxTileH;
  }
  if (tileSize > MAX_TILE_SIZE) {
    tileSize = MAX_TILE_SIZE;
  }
  if (tileSize < SPRITE_SIZE) {
    tileSize = SPRITE_SIZE;
  }

  boardX0 = (renderTarget.width() - boardPixelWidth()) / 2;
  if (boardX0 < 4) {
    boardX0 = 4;
  }

  int contentTop = HUD_H + 4;
  int contentH = renderTarget.height() - contentTop - 4;
  boardY0 = contentTop + (contentH - boardPixelHeight()) / 2;
  if (boardY0 < contentTop) {
    boardY0 = contentTop;
  }
}

void SokobanGame::updateHudLayout() {
  const int screenW = renderTarget.width();
  hudTitleX = 8;
  hudLevelX = screenW - Font5x7::textWidth(hudLevelText, 2) - 8;
  if (hudLevelX < hudTitleX + Font5x7::textWidth("SOKOBAN", 2) + 8) {
    hudLevelX = hudTitleX + Font5x7::textWidth("SOKOBAN", 2) + 8;
  }

  hudMovesX = 8;
  hudTotalX = (screenW - Font5x7::textWidth(hudTotalText, 1)) / 2;
  if (hudTotalX < hudMovesX + Font5x7::textWidth(hudMovesText, 1) + 8) {
    hudTotalX = hudMovesX + Font5x7::textWidth(hudMovesText, 1) + 8;
  }

  hudStatusX = screenW - Font5x7::textWidth(hudStatusText, 1) - 8;
  if (hudStatusX < hudTotalX + Font5x7::textWidth(hudTotalText, 1) + 8) {
    hudStatusX = hudTotalX + Font5x7::textWidth(hudTotalText, 1) + 8;
  }
}

void SokobanGame::updateOverlayLayout() {
  int textW = Font5x7::textWidth(overlayTitleText, 2);
  int subW = Font5x7::textWidth(overlaySubText, 1);
  if (subW > textW) {
    textW = subW;
  }

  overlayW = textW + 24;
  if (overlayW < 160) {
    overlayW = 160;
  }
  if (overlayW > renderTarget.width() - 16) {
    overlayW = renderTarget.width() - 16;
  }

  overlayX0 = (renderTarget.width() - overlayW) / 2;
  overlayY0 = (renderTarget.height() - OVERLAY_H) / 2;
  overlayTitleX = (renderTarget.width() - Font5x7::textWidth(overlayTitleText, 2)) / 2;
  overlaySubX = (renderTarget.width() - Font5x7::textWidth(overlaySubText, 1)) / 2;
}

void SokobanGame::markHudDirty() {
  markRectDirty(0, 0, renderTarget.width(), HUD_H);
}

void SokobanGame::markOverlayDirty() {
  markRectDirty(overlayX0, overlayY0, overlayW, OVERLAY_H);
}

void SokobanGame::markCellDirty(int gx, int gy) {
  if (!inBounds(gx, gy)) {
    return;
  }
  markRectDirty(boardX0 + gx * tileSize, boardY0 + gy * tileSize, tileSize, tileSize);
}

void SokobanGame::markBoardFrameDirty() {
  int x = boardX0 - 2;
  int y = boardY0 - 2;
  int w = boardPixelWidth() + 4;
  int h = boardPixelHeight() + 4;
  markRectDirty(x, y, w, h);
}

void SokobanGame::markRectDirty(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }
  dirty.add(x, y, x + w - 1, y + h - 1);
}

void SokobanGame::invalidatePlayingScreen() {
  dirty.invalidate(renderTarget);
}

void SokobanGame::flushDirty() {
  flusher.flush(renderTarget, regionBuf, [this](int x0, int y0, int w, int h, uint16_t* buf) {
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
      s.setPosition(boardX0 + x * tileSize + spriteInset(), boardY0 + y * tileSize + spriteInset());
    }
  }

  auto& p = sprites.sprite(PLAYER_SPRITE_SLOT);
  p.active = true;
  p.setPosition(
    boardX0 + playerX * tileSize + spriteInset(),
    boardY0 + playerY * tileSize + spriteInset());
}

int SokobanGame::boardPixelWidth() const {
  return boardW * tileSize;
}

int SokobanGame::boardPixelHeight() const {
  return boardH * tileSize;
}

int SokobanGame::spriteInset() const {
  int inset = (tileSize - SPRITE_SIZE) / 2;
  if (inset < 0) {
    return 0;
  }
  return inset;
}

uint16_t SokobanGame::pixelAt(int x, int y) const {
  if (x < 0 || x >= renderTarget.width() || y < 0 || y >= renderTarget.height()) {
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
  lx = x - hudTitleX;
  if (Font5x7::textPixel("SOKOBAN", 2, lx, ly)) {
    return COLOR_ACCENT;
  }

  ly = y - HUD_LEVEL_Y;
  lx = x - hudLevelX;
  if (Font5x7::textPixel(hudLevelText, 2, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_MOVES_Y;
  lx = x - hudMovesX;
  if (Font5x7::textPixel(hudMovesText, 1, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_TOTAL_Y;
  lx = x - hudTotalX;
  if (Font5x7::textPixel(hudTotalText, 1, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - HUD_STATUS_Y;
  lx = x - hudStatusX;
  if (Font5x7::textPixel(hudStatusText, 1, lx, ly)) {
    return levelSolved ? COLOR_PLAYER_HI : COLOR_TEXT_DIM;
  }

  return color;
}

uint16_t SokobanGame::boardPixelAt(int x, int y) const {
  int frameX = boardX0 - 2;
  int frameY = boardY0 - 2;
  int frameW = boardPixelWidth() + 4;
  int frameH = boardPixelHeight() + 4;
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
  if (rx >= boardPixelWidth() || ry >= boardPixelHeight()) {
    return COLOR_BG;
  }

  int gx = rx / tileSize;
  int gy = ry / tileSize;
  int lx = rx - gx * tileSize;
  int ly = ry - gy * tileSize;
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
    if (ly >= tileSize - 2 || lx >= tileSize - 2) {
      return COLOR_WALL_SH;
    }
    return COLOR_WALL;
  }

  uint16_t floorColor = (((gx + gy) & 1) == 0) ? COLOR_FLOOR_A : COLOR_FLOOR_B;
  if (lx == 0 || ly == 0) {
    return COLOR_GRID;
  }

  if (hasTarget) {
    int cx = tileSize / 2;
    int cy = tileSize / 2;
    int dx = lx - cx;
    int dy = ly - cy;
    int dist2 = dx * dx + dy * dy;
    int outerR = tileSize / 2 - 3;
    int innerR = tileSize / 2 - 5;
    int holeR = tileSize / 2 - 7;
    if (outerR < 3) {
      outerR = 3;
    }
    if (innerR < 2) {
      innerR = 2;
    }
    if (holeR < 1) {
      holeR = 1;
    }
    if (dist2 <= outerR * outerR) {
      if (dist2 <= holeR * holeR) {
        return floorColor;
      }
      return dist2 <= innerR * innerR ? COLOR_TARGET_HI : COLOR_TARGET;
    }
  }

  return floorColor;
}

uint16_t SokobanGame::overlayPixelAt(int x, int y) const {
  if (!levelSolved) {
    return 0;
  }

  if (x < overlayX0 || x >= overlayX0 + overlayW || y < overlayY0 || y >= overlayY0 + OVERLAY_H) {
    return 0;
  }

  if (y < overlayY0 + 2 || y >= overlayY0 + OVERLAY_H - 2) {
    return COLOR_ACCENT;
  }

  int ly = y - (overlayY0 + OVERLAY_TEXT1_Y_OFF);
  int lx = x - overlayTitleX;
  if (Font5x7::textPixel(overlayTitleText, 2, lx, ly)) {
    return COLOR_TEXT;
  }

  ly = y - (overlayY0 + OVERLAY_TEXT2_Y_OFF);
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
