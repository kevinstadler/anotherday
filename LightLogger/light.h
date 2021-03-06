#include "TCS34725.h"
#include "TCS34725AutoGain.h"

TCS34725::RawData raw;

#define INTERVAL_INCREASE 1
#define INTERVAL_DECREASE 2
#define LIGHT_LOG 4

// return true if there is new data that is significantly different from the last logged one to be worth logging
bool getLight() {
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);

  Wire.begin(D1, D2); // switch pins around
  TCS34725 tcs;
  if (tcs.attach(Wire)) {
    tcs.enableColorTempAndLuxCalculation(true);
    readWithAutoGain(&tcs, 1000);
  } else {
    DEBUG_PRINTLN("Failed to attach Wire!");
  }
  raw = tcs.raw();
  // cut power again
  digitalWrite(D3, LOW);

  float lux = tcs.lux();
  int32_t cct = tcs.colorTemperature();

  // determine if there is change to the previously logged one
  // JND of Lux is 7% of current value
  float deltaLux = abs(nv->rtcData.lastLux - lux);
  float relDeltaLux = abs(1.0 - nv->rtcData.lastLux / lux);
  // JND of Kelvin is ~100 until 4000, ~500 above 5000
  int32_t dCct = abs(cct - (int32_t) nv->rtcData.lastCct);
  // TODO check positive lux instead of raw.c value?
  DEBUG_PRINTLN(raw.c);
  DEBUG_PRINTLN(lux);
  DEBUG_PRINTLN(relDeltaLux);
  DEBUG_PRINTLN(nv->rtcData.lastCct);
  DEBUG_PRINTLN(cct);
  if ((deltaLux > .1 && relDeltaLux >= .05) || (lux >= .3 && dCct > 100)) {
    // worth logging
    nv->rtcData.lastLux = lux;
    nv->rtcData.lastCct = cct;

    // TODO calculate deep sleep time based on battery voltage
    // https://electronics.stackexchange.com/questions/32321/lipoly-battery-when-to-stop-draining
    DEBUG_PRINTLN("Data changed, decreasing interval");
    nv->rtcData.readInterval = min(max(MIN_INTERVAL, data->readInterval / 2), RESET_INTERVAL);
    return true;
  } else {
    // increase by doubling, max is 10s step
    nv->rtcData.readInterval = min(data->readInterval + min(data->readInterval, MAX_INCREASE), MAX_INTERVAL);
    DEBUG_PRINT("Data stationary, increasing interval to ");
    DEBUG_PRINTLN(data->readInterval / 1e3);
    // don't log
    return false;
  }
}
