#include "SGF.h"
#include "SokobanGame.h"

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED D6

#define PIN_LEFT 2
#define PIN_RIGHT 3
#define PIN_FIRE D4
#define PIN_UP D5
#define PIN_DOWN D7

FastILI9341 gfx(TFT_CS, TFT_DC, TFT_RST, TFT_LED);
SokobanGame sokoban(gfx, PIN_LEFT, PIN_RIGHT, PIN_UP, PIN_DOWN, PIN_FIRE);

void setup() {
  sokoban.setup();
}

void loop() {
  sokoban.loop();
}
