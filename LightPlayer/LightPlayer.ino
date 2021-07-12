#define DEBUG 1
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// 'cold' white i.e. high Kelvin
#define CW D7
// 'warm' white i.e. low Kelvin
#define WW D8
#include "ledmapping.h"
float currentCold;
float currentWarm;

#include "time.h"
#include "wifi.h"
#include "data.h"
uint32_t nextDownload;

#include "ledplayback.h" // calculateSteps(), i, nSteps, nextStepTime, ...

#include "display.h"

void setup() {
  pinMode(CW, OUTPUT);
  digitalWrite(CW, 0);
  pinMode(WW, OUTPUT);
  digitalWrite(WW, 0);

  startWifi();

  matrix.begin();
//  timer1_attachInterrupt(clearDisplayCallback);
//  timer1_enable();
//  timer1_write();
//  #include <ESP8266WiFi.h>
  for (uint8_t col = 0; col < matrix.getColumnCount(); col++) {
    matrix.setColumn(col, 0xff);
    delay(20);
    matrix.setColumn(col, 0x00);
  }

  analogWrite(CW, 0);
  analogWrite(WW, 0);
//  analogWriteResolution(PWM_BITS);
//  analogWriteRange(0xFFFF >> (16 - PWM_BITS));
//  analogWriteFreq(0xFFFF >> (14 - PWM_BITS));

  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println();
  #endif

  if (!SPIFFS.begin()) {
    DEBUG_PRINTLN("SPIFFS error, shutting down.");
    ESP.deepSleep(0);
  }

  loadLookupTable();
  loadData();
  calculateSteps();
}

#define LOOPINTERVAL 50 // ms
#define MAXBRIGHTNESSINCREASE 1.1f // no more than 10% brightness increase per loopinterval

void loop() {
  uint32_t m = millis();

  // poti polling and WiFi updating happens here...
  // EWMA with old component > 2/3 guarantees point-changes are ignored
  byte newPoti = (3*poti + readPoti()) / 4;

  if (newPoti != poti) {
    // A0 can read 10 bit resolution (up to 1023), drop down to 5 bit to be more resistant to noise
    poti = newPoti;
    DEBUG_PRINTLN();
    DEBUG_PRINT("Poti setting has changed, is now ");
    DEBUG_PRINT(poti);
    DEBUG_PRINT(", updating offset to ");
    // change/discontinuity in replayoffset
    // 32 poti levels * 15 minutes each = 480m = 8h (+1h base offset)
    // 30 poti levels * 10 minutes each = 300m = 5h (+1h base offset)
    setUserOffset(newPoti * 10);
    DEBUG_PRINTLN(userOffset/60); // print in minutes
    uint32_t targetTime = getTargetTime();
    if (targetTime < data[0].ts || targetTime > data[ndata-1].ts) {
      DEBUG_PRINTLN("OUT OF RANGE, increasing data offset");
      // += because the current dataOffset is included in the targetTime, plus 1 minute headroom
      dataOffset += 60 + targetTime - data[ndata-1].ts;
    }
    calculateSteps();
    displayOffset();
  } else if (m > lastDisplayed + 7e3) {
    // turn off display after 7 seconds
    matrix.clear();
  }

  // check current millis because nextStepTime might've been set in calculateSteps()
  if (millis() >= nextStepTime) {
    uint32_t targetTime = getTargetTime();
    if (targetTime >= data[i+1].ts) {
      DEBUG_PRINTLN();
      i = (i+1) % (ndata - 1);
      calculateSteps(false);
    }

    nextStepTime += stepDuration;
    setLight(((nSteps - currentStep) * data[i].illuminance + currentStep * data[i+1].illuminance) / nSteps,
      ((nSteps - currentStep) * data[i].cct + currentStep * data[i+1].cct) / nSteps);
    currentStep++;
  }

  int32_t waitLeft = LOOPINTERVAL - (millis() - m);
  if (currentCold != targetCold || currentWarm != targetWarm) {
    // slow fadey approach so as not to kill the eyes
    float coldDiff = targetCold - currentCold;
    float warmDiff = targetWarm - currentWarm;
    // turn down instantly, but never increase by more than 10% of current brightness per step
    currentCold = coldDiff <= 0 ? targetCold : min(targetCold, max(1.f, MAXBRIGHTNESSINCREASE * currentCold));
    currentWarm = warmDiff <= 0 ? targetWarm : min(targetWarm, max(1.f, MAXBRIGHTNESSINCREASE * currentWarm));
    analogWrite(CW, currentCold);
    analogWrite(WW, currentWarm);
    DEBUG_PRINT('.');

  } else if (stepDuration > 20e3 && now() > nextDownload) {
    DEBUG_PRINTLN("\nData is stale, fetching new");
    if (downloadData()) {
      DEBUG_PRINT("downloaded, loading...");
      loadData();
    }
    nextDownload = max(data[ndata - 1].ts + HOUR, now() + 5 * MINUTE);
    // even if download didn't give new data, networktime might have changed!
    calculateSteps();
    DEBUG_PRINT("Working with data that is ");
    DEBUG_PRINT((now() - data[ndata - 1].ts)/60);
    DEBUG_PRINTLN(" minutes old.");
    waitLeft = LOOPINTERVAL - (millis() - m);
  }

  if (waitLeft > 5) {
    delay(waitLeft);
  }
}
