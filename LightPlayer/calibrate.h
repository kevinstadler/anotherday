void calibrate() {
  float brightest = PWM[PWMRESOLUTION - 1];
  for (byte i = 0; i < MIXRESOLUTION; i++) {
    // from warm (low temp) to cold
    analogWrite(CW, i * brightest / MIXRESOLUTION);
    analogWrite(WW, (MIXRESOLUTION - i - 1) * brightest / MIXRESOLUTION);
    readWithAutoGain(&tcs);
    kelvinPerMix[i] = tcs.colorTemperature();
    Serial.println(kelvinPerMix[i]);
  }
  for (byte i = 0; i < PWMRESOLUTION; i++) {
    analogWrite(CW, 0);
    analogWrite(WW, PWM[i]);
    readWithAutoGain(&tcs);
    luxPerPWM[i] = tcs.lux();
    Serial.println(luxPerPWM[i]);
  }
}
