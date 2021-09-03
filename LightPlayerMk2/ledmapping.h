// 50000 is just below 2**16, so including 0 need 18 slots total
#define PWMRESOLUTION 18
uint32_t PWM[PWMRESOLUTION];
float luxPerPWM[PWMRESOLUTION];

#define MIXRESOLUTION 9
float kelvinPerMix[MIXRESOLUTION];

#include "ledcalibration.h"

#include <FS.h>
#define LOOKUPTABLEFILE "/lookup.csv"

// actually only ever uses integers
float targetCold;
float targetWarm;

void loadLookupTable() {
  // automatically fill calibration levels: [0, 1, 2, 4, ..., PWM_PERIOD]
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    PWM[i] = min(PWM_PERIOD, (1 << i) >> 1);
  }
  bool willCalibrate = false;
  DEBUG_PRINT("Checking TCS...");
  if (hasTCS()) {
    DEBUG_PRINT(tcs.lux());
    if (tcs.lux() < 2.0f) {
      DEBUG_PRINT(" -- set up in a dark room, queueing calibration!");
      willCalibrate = true;
    }
    DEBUG_PRINTLN();
  }
  if (!willCalibrate) {
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
    DEBUG_PRINTLN("Calibrating...");
    ESP.deepSleep(60e6); // TODO REMOVE
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
  DEBUG_PRINTLN("=== CCT (W:C) ===");
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    DEBUG_PRINT(MIXRESOLUTION - i - 1);
    DEBUG_PRINT(":");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" => ");
    DEBUG_PRINTLN(kelvinPerMix[i]);
  }
  DEBUG_PRINTLN("=== LUX/PWM (warm only) ===");
  // make full on correspond to 5k lux
  float scaleFactor = 5000.0 / luxPerPWM[PWMRESOLUTION - 1];
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    DEBUG_PRINT(PWM[i]);
    DEBUG_PRINT("/");
    DEBUG_PRINT(PWM_PERIOD);
    DEBUG_PRINT(" => ");
    DEBUG_PRINTLN(luxPerPWM[i]);
    luxPerPWM[i] *= scaleFactor;
  }
  DEBUG_PRINT("\nScaling the lux output by a factor of ");
  DEBUG_PRINTLN(scaleFactor);
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

float interpolate(float pos, uint32_t *values) {
  float r = values[uint8_t(pos)];
  if (fmod(pos, 1) != 0) {
    r += fmod(pos, 1) * (values[uint8_t(pos) + 1] - values[uint8_t(pos)]);
  }
  return r;
}

// maps lux to [0, PWM_PERIOD] interval
uint32_t lookupPWM(float lux) {
  // ceil so we get at least 1, and don't accept 0 PWM
  return ceil(interpolate(max(1.0f, getPosition(lux, luxPerPWM, PWMRESOLUTION)), PWM));
}

// maps CCT to float fraction of 'cold' (high temperature) LED
float lookupColdPortion(uint16_t kelvin) {
  return getPosition(kelvin, kelvinPerMix, MIXRESOLUTION) / (MIXRESOLUTION - 1);
}
