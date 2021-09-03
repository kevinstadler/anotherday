#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)

// can't trust D* definitions between D1 R1 and D1 mini, so specify GPIO directly

// 'warm' white i.e. low Kelvin = yellow cable
#define WW 0
#define WW_PIN 12 // R1: D6/D12
// 'cold' white i.e. high Kelvin = black cable
#define CW 1
#define CW_PIN 15 // R2: D10

uint32_t pwm_pins[2][3] = {
  // MUX_REGISTER, MUX_VALUE and GPIO
  // https://github.com/willemwouters/ESP8266/blob/master/sdk/esp_iot_rtos_sdk-master/include/espressif/esp8266/pin_mux_register.h#L38
  {PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, WW_PIN}, // D7 = warm
  {PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, CW_PIN}, // D8 = cold
};

// frequency = 5.000.000 / period (which is given in # of 200ns ticks)
// tick period for 100Hz PWM = 50000
#define PWM_PERIOD 50000
#include "pwm.h"

#include "ledmapping.h"
float currentCold;
float currentWarm;

enum Status { DISCONNECTED, CONNECTED, DOWNLOADING };
volatile Status status = DISCONNECTED;

#include "time.h"
uint32_t nextDownload;
bool mustSetLight = false;
#include "data.h"
#include "wifi.h"

#include "ledplayback.h" // i, userOffset, dataOffset,...

#include "display.h"

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(CW_PIN, OUTPUT);
  digitalWrite(CW_PIN, 0);
  pinMode(WW_PIN, OUTPUT);
  digitalWrite(WW_PIN, 0);

  DEBUG_PRINTLN("Starting up...");
  DEBUG_PRINTLN(" * PWM");
  pwm_init(PWM_PERIOD, NULL, 2, pwm_pins);
  DEBUG_PRINTLN(" * WiFi");
  setupWifi();

  DEBUG_PRINTLN(" * display");
  display();

  DEBUG_PRINTLN(" * FS");
  if (!SPIFFS.begin()) {
    DEBUG_PRINTLN("SPIFFS error, shutting down.");
    ESP.deepSleep(0);
  }

  DEBUG_PRINTLN(" * light data");
  loadLookupTable();
//  loadData();
  setUserOffset(readPoti());
  DEBUG_PRINT(" * waiting for playback data..");

  for (uint16_t i = 0; ndata == 0; i++) {
    // give up after 20 seconds tops
    if (i == 40) {
      ESP.reset();
    }
    for (uint8_t part = 0; part <= status; part++) {
      for (uint8_t col = 0; col < 8; col++) {
        // from right to left
        matrix.setColumn(8*(3-part) + col, (part == status && i % 2) ? 0x00 : 0xff);
      }
    }
    delay(500);
  }
  setLight();
  displayOffset();
}

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define LOOPINTERVAL 30 // ms
uint32_t lastLoop;

void loop() {
  lastLoop = millis();

  // poti polling and WiFi updating happens here...
  // EWMA with old component > 2/3 guarantees jittery point-changes are ignored
  byte newPoti = (3*poti + readPoti()) / 4;

  // ignore last bit
  if (newPoti != poti) {
    // A0 can read 10 bit resolution (up to 1023), drop down to 5 bit to be more resistant to noise
    DEBUG_PRINTLN();
    DEBUG_PRINT("Poti ");
    DEBUG_PRINT(poti);
    DEBUG_PRINT(" -> ");
    DEBUG_PRINT(newPoti);
    DEBUG_PRINT(", offset ");
    poti = newPoti;
    // change/discontinuity in replayoffset
    // 32 poti levels * 15 minutes each = 480m = 8h (+1h base offset)
    // 30 poti levels * 10 minutes each = 300m = 5h (+1h base offset)
    DEBUG_PRINT(userOffset/60); // print in minutes
    DEBUG_PRINT(" -> ");
    setUserOffset(poti);
    DEBUG_PRINTLN(userOffset/60); // print in minutes
    uint32_t targetTime = getTargetTime();
    if (targetTime > data[ndata-1].ts) { // TODO do nothing?targetTime < data[0].ts ||
      DEBUG_PRINTLN(targetTime - data[ndata-1].ts);
      DEBUG_PRINT("OUT OF RANGE, increasing data offset: ");
      // += because the current dataOffset is included in the targetTime, plus 1 minute headroom
//        dataOffset += 60 + targetTime - data[ndata-1].ts;
      dataOffset += OFFSET_RESOLUTION;
      DEBUG_PRINTLN(dataOffset);
    }
    setLight();
    displayOffset();
  } else if (lastLoop > lastDisplayed + DISPLAY_DURATION) {
    // turn off display after 7 seconds
    display(false);
  }

  if (mustSetLight) {
    mustSetLight = false;
    setLight();
  } else {
    uint32_t targetTime = getTargetTime();
    if (i == ndata - 1) {
      dataOffset += targetTime - data[ndata-2].ts;
  //    DEBUG_PRINT("\nReached the end of data, increasing dataOffset to ");
  //    DEBUG_PRINTLN(dataOffset);
    } else if (targetTime >= data[i+1].ts) {
      i++;
      setLight(false);
    }
  }

  int32_t waitLeft = LOOPINTERVAL - (millis() - lastLoop);
  if (currentCold != targetCold || currentWarm != targetWarm) {
    // slow fadey approach so as not to kill the eyes
    float coldDiff = targetCold - currentCold;
    float warmDiff = targetWarm - currentWarm;
    // turn down instantly, but never increase by more than 10% of current brightness per step
    // FIXME is this PWM or what is it???
    coldDiff = sgn(coldDiff) * min(abs(coldDiff), max(1.f, currentCold * .1f));
    warmDiff = sgn(warmDiff) * min(abs(warmDiff), max(1.f, currentWarm * .1f));
/*    if (coldDiff > currentCold * .1f || warmDiff > currentWarm * .1f) {
      DEBUG_PRINTLN(coldDiff);
      DEBUG_PRINTLN(warmDiff);
    } */
/*    currentCold = coldDiff <= 0 ? constrain(max(currentCold / MAX_BRIGHTNESS_DIFF_REL, currentCold - MAX_BRIGHTNESS_DIFF_ABS), targetCold, currentCold-2)
                                : constrain(min(currentCold * MAX_BRIGHTNESS_DIFF_REL, currentCold + MAX_BRIGHTNESS_DIFF_ABS), currentCold+2, targetCold);
    currentWarm = warmDiff <= 0 ? constrain(max(currentWarm / MAX_BRIGHTNESS_DIFF_REL, currentWarm - MAX_BRIGHTNESS_DIFF_ABS), targetWarm, currentWarm-2)
                                : constrain(min(currentWarm * MAX_BRIGHTNESS_DIFF_REL, currentWarm + MAX_BRIGHTNESS_DIFF_ABS), currentWarm+2, targetWarm);
*/
    currentCold += coldDiff;
    currentWarm += warmDiff;
    pwm_set_duty(currentCold, CW);
    pwm_set_duty(currentWarm, WW);
    pwm_start();
    // scale lux logarithmically
    matrix.control(MD_MAX72XX::INTENSITY, constrain(ceil(log(currentCold+currentWarm)/log(2) - 8), 1, MAX_INTENSITY));
//    DEBUG_PRINT(constrain(ceil(log(currentCold+currentWarm)/log(2) - 6), 1, MAX_INTENSITY));
    if (currentWarm == targetWarm && currentCold == targetCold) {
      DEBUG_PRINT('.');
    } else {
      switch (abs(sgn(warmDiff) + sgn(coldDiff))) {
        case 2: // ++ or --
          DEBUG_PRINT(coldDiff > 0 ? '>' : '<');
          break;
        case 1:
          DEBUG_PRINT(warmDiff + coldDiff > 0 ? '+' : '-');
          break;
        default:
          DEBUG_PRINT('X');
          break;
      }
    }

  } else {
    checkStaleData();
  }

  if (waitLeft > 5) {
    delay(waitLeft);
  }
}
