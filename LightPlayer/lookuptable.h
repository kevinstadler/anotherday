#define MIXRESOLUTION 5
float kelvinPerMix[MIXRESOLUTION];

#define PWMRESOLUTION 12
uint16_t PWM[PWMRESOLUTION] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1023 };
float luxPerPWM[PWMRESOLUTION];

#include "TCS34725.h"
#include "TCS34725AutoGain.h"
TCS34725 tcs;
#include "calibrate.h"

// TODO use SPIFFS
#define LOOKUPTABLEFILE "/lookup.csv"
#include <FS.h>

void loadLookupTable() {
  SPIFFS.begin();

  // TODO trigger calibration through some jumper
  Wire.begin();
  if (!tcs.attach(Wire)) {
//  if (true && SPIFFS.exists(LOOKUPTABLEFILE)) {
    Serial.println("Reading table from flash");
    File file = SPIFFS.open(LOOKUPTABLEFILE, "r");
    for (byte i = 0; i < MIXRESOLUTION; i++) {
      kelvinPerMix[i] = file.parseFloat();
    }
    for (byte i = 0; i < PWMRESOLUTION; i++) {
     luxPerPWM[i] = file.parseFloat();
    }
    file.close();
  }

  if (luxPerPWM[PWMRESOLUTION - 1] == 0) {
    calibrate();
    Serial.println("Writing table to flash");
    File file = SPIFFS.open(LOOKUPTABLEFILE, "w");
    for (byte i = 0; i < MIXRESOLUTION; i++) {
      file.print(kelvinPerMix[i]);
      file.print(',');
    }
    for (byte i = 0; i < PWMRESOLUTION; i++) {
      file.print(luxPerPWM[i]);
      file.print(',');
    }
    file.close();
  }
  // debug output
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    Serial.println(kelvinPerMix[i]);
  }
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    Serial.println(luxPerPWM[i]);
  }
}

// threshold arrays must be ascending of length n, i.e. highest index is n-1
float getPosition(float value, float *thresholds, byte n) {
  if (value < thresholds[0]) {
    return 0;
  }
  for (uint8_t i = 1; i < n; i++) {
    if (value < thresholds[i]) {
      // between i-1 and i
      return i - 1 + (value - thresholds[i - 1]) / (thresholds[i] - thresholds[i - 1]);
    }
  }
  return n-1;
}

float interpolate(float pos, uint16_t *values) {
  float r = values[uint8_t(pos)];
  if (fmod(pos, 1) != 0) {
    r += fmod(pos, 1) * (values[uint8_t(pos) + 1] - values[uint8_t(pos)]);
  }
  return r;
}

// maps lux to [0, 1023] PWM cycle interval
uint16_t lookupPWM(float lux) {
  return round(interpolate(getPosition(lux, luxPerPWM, PWMRESOLUTION), PWM));
}

// maps CCT to float fraction of 'cold' (high temperature) LED
float lookupColdPortion(uint16_t kelvin) {
  return getPosition(kelvin, kelvinPerMix, MIXRESOLUTION) / (MIXRESOLUTION - 1);
}
