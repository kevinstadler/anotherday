#include <MD_MAX72xx.h>
//MD_MAX72XX matrix(MD_MAX72XX::FC16_HW, D8, 4);
// data/mosi, clk/sck, cs/ss
MD_MAX72XX matrix(MD_MAX72XX::FC16_HW, 16, 12, 14, 4);

uint32_t lastDisplayed;

void displayOffset() {
  // display userOffset + dataOffset together
  uint32_t offset = dataOffset + userOffset;
  uint8_t days = offset / 62400;
  uint8_t hours = (offset / 3600) % 24;
  uint16_t minutes = ( offset / 60 ) % 60;
  char strng[10];
  uint8_t len;
  if (days > 0) {
    len = sprintf(strng, "%dd %dh", days, hours);
  } else if (hours == 0) {
    len = sprintf(strng, "%dm ago", minutes);
  } else if (minutes == 0) {
    len = sprintf(strng, "%dh ago", hours);
  } else {
    len = sprintf(strng, "%dh %dm", hours, minutes);
  }
  uint8_t cBuf[8];
  matrix.clear();
  // from right to left
  uint8_t col = 0;
  for (int8_t ch = len - 1; ch >= 0; ch--) {
    uint8_t w = matrix.getChar(strng[ch], sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
//    matrix.setChar(13, 48 + (millis() - lastMotion[1]) / 1000);
    for (int8_t i = w - 1; i >= 0; i--) {
      matrix.setColumn(col++, cBuf[i]);
    }
    col++;
  }
  lastDisplayed = millis();
}
