//#include "TCS34725.h"
#include "TCS34725AutoGain.h"
TCS34725 tcs;

bool hasTCS() {
  Wire.begin();
  return tcs.attach(Wire, TCS34725::Mode::Idle);
}

void calibrate() {
  float brightest = PWM[PWM_BITS - 1];
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    // from warm (low temp) to cold
    analogWrite(CW, i * brightest / MIXRESOLUTION);
    analogWrite(WW, (MIXRESOLUTION - i - 1) * brightest / MIXRESOLUTION);
    delay(50);
    tcs.autoGain(1000);
    kelvinPerMix[i] = tcs.colorTemperature();
    DEBUG_PRINTLN(kelvinPerMix[i]);
  }
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    analogWrite(CW, 0);
    analogWrite(WW, PWM[i]);
    delay(50);
    tcs.autoGain(1000);
    luxPerPWM[i] = tcs.lux();
    DEBUG_PRINTLN(luxPerPWM[i]);
  }
  analogWrite(CW, 0);
  analogWrite(WW, 0);
}
