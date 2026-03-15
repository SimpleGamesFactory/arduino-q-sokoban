#include "SokobanGame.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {

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

int fitCenteredScale(int screenWidth, const char* text, int maxScale, int margin) {
  for (int scale = maxScale; scale >= 1; --scale) {
    if (FontRenderer::textWidth(FONT_5X7, text, scale) <= screenWidth - margin * 2) {
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
    renderer(renderTargetRef, dirty, MAX_TILE_W, MAX_TILE_H),
    actionBindings{
      {leftPinInput, leftAction},
      {rightPinInput, rightAction},
      {upPinInput, upAction},
      {downPinInput, downAction},
      {firePinInput, fireAction},
    },
    titleScene(*this),
    playingScene(*this),
    gameOverScene(*this) {
  pinLeft = hardwareProfile.input.left;
  pinRight = hardwareProfile.input.right;
  pinUp = hardwareProfile.input.up;
  pinDown = hardwareProfile.input.down;
  pinFire = hardwareProfile.input.fire;

  renderer.setBackgroundRenderer([this](int x0,
                                        int y0,
                                        int w,
                                        int h,
                                        int32_t worldX0,
                                        int32_t worldY0,
                                        uint16_t* buf) {
    (void)worldX0;
    (void)worldY0;
    renderBackgroundToBuffer(x0, y0, w, h, buf);
  });
  renderer.setRegionBuffer(regionBuf);

  buildSpritePixels();
  initSpriteSlots();
}

void SokobanGame::setup() {
  start();
}

void SokobanGame::onSetup() {
  leftPinInput.attach(pinLeft);
  rightPinInput.attach(pinRight);
  upPinInput.attach(pinUp);
  downPinInput.attach(pinDown);
  firePinInput.attach(pinFire);

  configureActions(actionBindings, sizeof(actionBindings) / sizeof(actionBindings[0]));

  dirty.clear();
  switchScene(titleScene);
}

void SokobanGame::onPhysics(float delta) {
  (void)delta;
}

void SokobanGame::onProcess(float delta) {
  (void)delta;
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
  switchScene(gameOverScene);
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
  const Vector2i screenSize = renderTarget.size();
  const int titleScale = fitCenteredScale(screenSize.x, "UNOQ SOKOBAN", 4, 12);

  dirty.clear();
  screen.fillScreen565(COLOR_BG);
  screen.fillRect565(14, 16, screenSize.x - 28, 4, COLOR_ACCENT);
  screen.fillRect565(14, 24, screenSize.x - 28, 2, COLOR_PANEL_LINE);
  screen.fillRect565(14, screenSize.y - 26, screenSize.x - 28, 2, COLOR_PANEL_LINE);
  screen.fillRect565(14, screenSize.y - 18, screenSize.x - 28, 4, COLOR_ACCENT);

  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 46, "UNOQ SOKOBAN", titleScale, COLOR_TEXT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 90, "10 PLANSZ", 2, COLOR_ACCENT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 118, "L/R/U/D - RUCH", 2, COLOR_TEXT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 144, "FIRE - START", 2, COLOR_TEXT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 170, "FIRE W GRZE - RESTART", 1, COLOR_TEXT_DIM);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 194, "PRZENIES SKRZYNKI NA CELE", 1, COLOR_TEXT_DIM);
}

void SokobanGame::renderGameOverScreen() {
  const Vector2i screenSize = renderTarget.size();
  dirty.clear();
  char movesBuf[24];
  char levelsBuf[24];
  const int titleScale = fitCenteredScale(screenSize.x, "GAME OVER", 4, 12);
  snprintf(movesBuf, sizeof(movesBuf), "%lu", (unsigned long)finalMoves);
  snprintf(levelsBuf, sizeof(levelsBuf), "%u / %u", (unsigned)LEVEL_COUNT, (unsigned)LEVEL_COUNT);

  screen.fillScreen565(COLOR_GO_BG);
  screen.fillRect565(18, 18, screenSize.x - 36, 3, COLOR_GO_LINE);
  screen.fillRect565(18, screenSize.y - 21, screenSize.x - 36, 3, COLOR_GO_LINE);

  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 48, "GAME OVER", titleScale, COLOR_GO_TITLE);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 96, "UKONCZONE PLANSZE", 1, COLOR_TEXT_DIM);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 112, levelsBuf, 3, COLOR_TEXT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 152, "RUCHY", 1, COLOR_TEXT_DIM);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 168, movesBuf, 3, COLOR_ACCENT);
  FontRenderer::drawTextCentered(FONT_5X7, screen, screenSize.x / 2, 206, "FIRE - MENU", 2, COLOR_TEXT);
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
  const Vector2i screenSize = renderTarget.size();
  int maxTileW = (screenSize.x - 8) / boardW;
  int maxTileH = (screenSize.y - HUD_H - 8) / boardH;
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

  boardX0 = (screenSize.x - boardPixelWidth()) / 2;
  if (boardX0 < 4) {
    boardX0 = 4;
  }

  int contentTop = HUD_H + 4;
  int contentH = screenSize.y - contentTop - 4;
  boardY0 = contentTop + (contentH - boardPixelHeight()) / 2;
  if (boardY0 < contentTop) {
    boardY0 = contentTop;
  }
}

void SokobanGame::updateHudLayout() {
  const int screenW = renderTarget.size().x;
  hudTitleX = 8;
  hudLevelX = screenW - textWidth(hudLevelText, 2) - 8;
  if (hudLevelX < hudTitleX + textWidth("SOKOBAN", 2) + 8) {
    hudLevelX = hudTitleX + textWidth("SOKOBAN", 2) + 8;
  }

  hudMovesX = 8;
  hudTotalX = (screenW - textWidth(hudTotalText, 1)) / 2;
  if (hudTotalX < hudMovesX + textWidth(hudMovesText, 1) + 8) {
    hudTotalX = hudMovesX + textWidth(hudMovesText, 1) + 8;
  }

  hudStatusX = screenW - textWidth(hudStatusText, 1) - 8;
  if (hudStatusX < hudTotalX + textWidth(hudTotalText, 1) + 8) {
    hudStatusX = hudTotalX + textWidth(hudTotalText, 1) + 8;
  }
}

void SokobanGame::updateOverlayLayout() {
  const Vector2i screenSize = renderTarget.size();
  int textW = textWidth(overlayTitleText, 2);
  int subW = textWidth(overlaySubText, 1);
  if (subW > textW) {
    textW = subW;
  }

  overlayW = textW + 24;
  if (overlayW < 160) {
    overlayW = 160;
  }
  if (overlayW > screenSize.x - 16) {
    overlayW = screenSize.x - 16;
  }

  overlayX0 = (screenSize.x - overlayW) / 2;
  overlayY0 = (screenSize.y - OVERLAY_H) / 2;
  overlayTitleX = (screenSize.x - textWidth(overlayTitleText, 2)) / 2;
  overlaySubX = (screenSize.x - textWidth(overlaySubText, 1)) / 2;
}

void SokobanGame::markHudDirty() {
  markRectDirty(0, 0, renderTarget.size().x, HUD_H);
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
  renderer.invalidate();
}

void SokobanGame::flushDirty() {
  renderer.render();
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
  for (int i = 0; i < BOX_SPRITE_SLOT_COUNT; i++) {
    Renderer2D::SpriteHandle sprite = renderer.sprite(i);
    sprite.setActive(false);
    sprite.setBitmap(boxSpritePixels, SPRITE_SIZE, SPRITE_SIZE, 0);
    sprite.setScale(SpriteScale::Normal);
    sprite.setAnchor(Vector2f{0.0f, 0.0f});
  }

  Renderer2D::SpriteHandle playerSprite = renderer.sprite(PLAYER_SPRITE_SLOT);
  playerSprite.setActive(false);
  playerSprite.setBitmap(playerSpritePixels, SPRITE_SIZE, SPRITE_SIZE, 0);
  playerSprite.setScale(SpriteScale::Normal);
  playerSprite.setAnchor(Vector2f{0.0f, 0.0f});
}

void SokobanGame::syncSpritesFromBoard() {
  for (int i = 0; i < BOX_SPRITE_SLOT_COUNT; i++) {
    renderer.sprite(i).setActive(false);
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
      Renderer2D::SpriteHandle sprite = renderer.sprite(slot++);
      sprite.setPosition(Vector2i{
        boardX0 + x * tileSize + spriteInset(),
        boardY0 + y * tileSize + spriteInset(),
      });
      sprite.setActive(true);
    }
  }

  Renderer2D::SpriteHandle playerSprite = renderer.sprite(PLAYER_SPRITE_SLOT);
  playerSprite.setPosition(Vector2i{
    boardX0 + playerX * tileSize + spriteInset(),
    boardY0 + playerY * tileSize + spriteInset(),
  });
  playerSprite.setActive(true);
}

int SokobanGame::textWidth(const char* text, int scale) const {
  return FontRenderer::textWidth(FONT_5X7, text, scale);
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
  const Vector2i screenSize = renderTarget.size();
  if (x < 0 || x >= screenSize.x || y < 0 || y >= screenSize.y) {
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

  return COLOR_PANEL;
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

  return COLOR_OVERLAY;
}

void SokobanGame::renderHudTextToBuffer(BufferFillRect& fillRect) const {
  FontRenderer::drawText(FONT_5X7, fillRect, hudTitleX, HUD_TITLE_Y, "SOKOBAN", 2, COLOR_ACCENT);
  FontRenderer::drawText(FONT_5X7, fillRect, hudLevelX, HUD_LEVEL_Y, hudLevelText, 2, COLOR_TEXT);
  FontRenderer::drawText(FONT_5X7, fillRect, hudMovesX, HUD_MOVES_Y, hudMovesText, 1, COLOR_TEXT);
  FontRenderer::drawText(FONT_5X7, fillRect, hudTotalX, HUD_TOTAL_Y, hudTotalText, 1, COLOR_TEXT);
  FontRenderer::drawText(
    FONT_5X7,
    fillRect,
    hudStatusX,
    HUD_STATUS_Y,
    hudStatusText,
    1,
    levelSolved ? COLOR_PLAYER_HI : COLOR_TEXT_DIM);
}

void SokobanGame::renderOverlayTextToBuffer(BufferFillRect& fillRect) const {
  if (!levelSolved) {
    return;
  }

  FontRenderer::drawText(FONT_5X7, fillRect, overlayTitleX, overlayY0 + OVERLAY_TEXT1_Y_OFF,
                         overlayTitleText, 2, COLOR_TEXT);
  FontRenderer::drawText(FONT_5X7, fillRect, overlaySubX, overlayY0 + OVERLAY_TEXT2_Y_OFF,
                         overlaySubText, 1, COLOR_TEXT_DIM);
}

void SokobanGame::renderBackgroundToBuffer(int x0, int y0, int w, int h, uint16_t* buf) {
  for (int yy = 0; yy < h; yy++) {
    int y = y0 + yy;
    for (int xx = 0; xx < w; xx++) {
      int x = x0 + xx;
      buf[yy * w + xx] = pixelAt(x, y);
    }
  }

  BufferFillRect fillRect(x0, y0, w, h, buf);
  renderHudTextToBuffer(fillRect);

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
    renderOverlayTextToBuffer(fillRect);
  }
}
