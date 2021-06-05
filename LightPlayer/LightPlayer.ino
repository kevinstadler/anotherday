#include "ESP8266WiFi.h"

// 'cold' white i.e. high Kelvin
#define CW D7
// 'warm' white i.e. low Kelvin
#define WW D5
#include "lookuptable.h"

void setup() {
  pinMode(CW, OUTPUT);
  analogWrite(CW, 0);
  pinMode(WW, OUTPUT);
  analogWrite(WW, 0);

  Serial.begin(115200);
  loadLookupTable();
}

void setLight(float lux, uint16_t cct) {
    uint16_t intensity = lookupPWM(lux);
    float cold = lookupColdPortion(cct);
    analogWrite(CW, cold * intensity);
    analogWrite(WW, (1 - cold) * intensity);
}

float illuminance = 500.0; // lux
uint16_t cct = 4500; // kelvin

float ills[3] = { 200, 1000, 2000 };
uint16_t ccts[3] = { 3600, 6500, 4000 };
byte ind = 0;

void loop() {
  float nextIlluminance = ills[ind];
  uint16_t nextCct = ccts[ind];
  uint32_t duration = 3e3; // in ms
  
  // JND for illuminance: 7.4% (~1/13th) of value -- or better just one for every PWM level
  uint16_t pwmSteps = abs(lookupPWM(nextIlluminance) - lookupPWM(illuminance));
  // JND for CCT: ~100 at < 4000K, ~500 at > 5000k, say 1/40th of value?
  // -> allow any point-change to be HALF of the JND, i.e.
  // max 1/50th of current value
  uint16_t dCct = nextCct - cct;
  // TODO limit steps to even lower number based on output resolution?
  uint16_t nsteps = max(pwmSteps, (uint16_t) ceil(50 * dCct / cct));
  Serial.print(illuminance);
  Serial.print("->");
  Serial.print(nextIlluminance);
  Serial.print(" / ");
  Serial.print(cct);
  Serial.print("->");
  Serial.print(nextCct);
  Serial.print(": ");
  Serial.print(nsteps);
  Serial.println(" steps");

  for (uint16_t i = 1; i <= nsteps; i++) {
    setLight(((nsteps - i) * illuminance + i * nextIlluminance) / nsteps, ((nsteps - i) * cct + i * nextCct) / nsteps);
    // don't wait on exiting the loop
/*    Serial.print(i);
    Serial.print("/");
    Serial.print(nsteps);
    Serial.print(": ");
    Serial.print(((nsteps - i) * illuminance + i * nextIlluminance) / nsteps);
    Serial.print(", ");
    Serial.println(((nsteps - i) * cct + i * nextCct) / nsteps); */
    if (i < nsteps) {
      delay(duration / nsteps);
    }
  }
  illuminance = nextIlluminance;
  cct = nextCct;
  ind = (ind+1) % 3;
}
