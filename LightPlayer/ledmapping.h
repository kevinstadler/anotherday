//#define PWM_BITS 14 // write [0, 9191]
#define PWM_BITS 10

#define PWMRESOLUTION PWM_BITS+1
uint16_t PWM[PWMRESOLUTION];
float luxPerPWM[PWMRESOLUTION];

#define MIXRESOLUTION 5
float kelvinPerMix[MIXRESOLUTION];

#include "ledcalibration.h"

#include <FS.h>
#define LOOKUPTABLEFILE "/lookup.csv"

// actually only ever uses integers
float targetCold;
float targetWarm;

void loadLookupTable() {
  // automatically fill calibration levels: [0, 1, 3, 7, ...]
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    PWM[i] = 0xFFFF >> (16 - i);
    DEBUG_PRINTLN(PWM[i]);
  }
  if (!hasTCS()) {
    DEBUG_PRINTLN("Reading table from flash");
    File file = SPIFFS.open(LOOKUPTABLEFILE, "r");
    for (byte i = 0; i < MIXRESOLUTION; i++) {
      kelvinPerMix[i] = file.parseFloat();
    }
    for (byte i = 0; i < PWMRESOLUTION; i++) {
      luxPerPWM[i] = file.parseFloat();
    }
    file.close();

    if (luxPerPWM[0] == 0) {
      DEBUG_PRINTLN("No TCS connected AND no valid calibration data found in SPIFFS, shutting down.");
      ESP.deepSleep(0);
    }
  }

  // if nothing was read
  if (luxPerPWM[0] == 0) {
    calibrate();
    
    DEBUG_PRINTLN("Writing table to flash");
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
  // TODO calculate which steps exceed the JNDs and print warning message
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    DEBUG_PRINTLN(kelvinPerMix[i]);
  }
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    DEBUG_PRINTLN(luxPerPWM[i]);
  }
}

// TODO switch to https://github.com/RobTillaart/MultiMap ?

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

void setLight(float lux, uint16_t cct) {
  uint16_t intensity = lookupPWM(lux);
  float cold = lookupColdPortion(cct);
  DEBUG_PRINT(" -> ");
  DEBUG_PRINT(lux);
  DEBUG_PRINT("lux @ ");
  DEBUG_PRINT(cct);
  DEBUG_PRINT("K (");
  DEBUG_PRINT(cold * intensity);
  DEBUG_PRINT("/");
  DEBUG_PRINT((1 - cold) * intensity);
  DEBUG_PRINT(")");
  targetCold = round(cold * intensity);
  targetWarm = round((1 - cold) * intensity);
}
