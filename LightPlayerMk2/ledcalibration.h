#include "TCS34725AutoGain.h"
TCS34725 tcs;

bool hasTCS() {
  digitalWrite(0, HIGH); // power TCS on D8
  delay(2);
  Wire.begin(14, 13); // SDA/CL -- R1: D5, D7
  tcs.attach(Wire, TCS34725::Mode::Idle);
  tcs.autoGain();
  return tcs.lux() != 0.0f;
}

void calibrate() {
  float brightest = PWM[PWMRESOLUTION - 1];
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    // from "warm" (low temp) to "cold" (high temp)
    pwm_set_duty((MIXRESOLUTION - i - 1) * brightest / MIXRESOLUTION, WW);
    pwm_set_duty(i * brightest / MIXRESOLUTION, CW);
    pwm_start();
    delay(50);
    if (i == 0) {
      tcs.autoGain(1000);
    } else {
      tcs.singleRead();
    }
    kelvinPerMix[i] = tcs.colorTemperature();
  }

  pwm_set_duty(0, CW);
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    // FIXME measure lux of each LED channel separately
    pwm_set_duty(PWM[i], WW);
    pwm_start();
    delay(50);
    tcs.autoGain(1000);
    luxPerPWM[i] = tcs.lux();
  }
  pwm_set_duty(0, CW);
  pwm_set_duty(0, WW);
  pwm_start();
}
