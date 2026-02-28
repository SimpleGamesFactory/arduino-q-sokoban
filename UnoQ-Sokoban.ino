// sgf.name: UnoQ-Sokoban
// sgf.boards: unoq, esp32
// sgf.default_board: esp32
// sgf.port.unoq: /dev/ttyACM0
// sgf.port.esp32: /dev/ttyUSB0

// Define SGF_HW_PRESET here or pass it via -DSGF_HW_PRESET=...
// Examples:
// #define SGF_HW_PRESET SGF_HW_PRESET_UNOQ_ILI9341_320X240
// #define SGF_HW_PRESET SGF_HW_PRESET_ESP32_ST7789_240X240

#include "SGFHardwarePresets.h"
#include "SokobanGame.h"

auto hardware = SGFHardwareProfile::makeRuntime();
SokobanGame sokoban(hardware.renderTarget(), hardware.screen(), hardware.profile);

void setup() {
  hardware.display.begin(hardware.profile.display.spiHz);
  hardware.display.setRotation(hardware.profile.display.rotation);
  hardware.display.setBacklight(hardware.profile.display.backlightLevel);
  sokoban.setup();
}

void loop() {
  sokoban.loop();
}
