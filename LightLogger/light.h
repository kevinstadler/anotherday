// can only accumulate 1024 per 2.4ms, so start at 4.8ms
#define REQUIRED_COUNT 1000

void getLight(File file) {
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);

  Wire.begin(D1, D2); // switch pins around
  if (!tcs.attach(Wire)) {
    Serial.println("Failed to attach Wire!");
  } else {
  
    tcs.gain(TCS34725::Gain::X01);
    tcs.enableColorTempAndLuxCalculation(true);
    TCS34725::RawData raw;

    byte gain = 1;
    // try up to 600ms
    // ideal loop with actual cycle multiples of 2.4, each of which is roughly 100ms: round(1:6 * 41.7)*2.4
    for (byte l = 0; l <= 6; l++) {
      // calculate good number of cycles
      float integrationTime = 2.4 * (l == 0 ? 2 : round(41.7 * l));
      tcs.integrationTime(integrationTime);
      while (!tcs.available()) {
        delay(integrationTime / 4);
      }
      raw = tcs.raw();
      if (raw.c >= REQUIRED_COUNT) {
        break;
      }

      // TODO check if reliable (saturation?)
//      TCS34725::Color color = tcs.color();
//      Serial.print(tcs.lux());
//      Serial.print(" Lux @ "); Serial.print(tcs.colorTemperature());
//      Serial.print(" K. "); Serial.print(raw.r);
//      Serial.print("/"); Serial.print(raw.g);
//      Serial.print("/"); Serial.print(raw.b);
//      Serial.print("/"); Serial.print(raw.c);
//      Serial.print(" -> "); Serial.print(color.r);
//      Serial.print("/"); Serial.print(color.g);
//      Serial.print("/"); Serial.println(color.b);

      float fulfillmentPotentialAtCurrentGain = raw.c == 0 ? 60 : integrationTime * REQUIRED_COUNT / (600 * raw.c * gain);
      // 1, 4, 16, 60
      if (fulfillmentPotentialAtCurrentGain >= 4) {
        if (fulfillmentPotentialAtCurrentGain >= 16) {
          if (fulfillmentPotentialAtCurrentGain >= 60) {
            tcs.gain(TCS34725::Gain::X60);
            gain = 60;
          } else {
            tcs.gain(TCS34725::Gain::X16);
            gain = 16;
          }
        } else {
          tcs.gain(TCS34725::Gain::X04);
          gain = 4;
        }
//        Serial.println("Switching gain to ");
//        Serial.println(gain);
      }
    }
    file.print(gain);
    file.print(",");
    file.print(integrationTime);
    file.print(",");
    file.print(raw.c);
    file.print(",");
    file.print(raw.r);
    file.print(",");
    file.print(raw.g);
    file.print(",");
    file.print(raw.b);
  }
  // cut power again
  digitalWrite(D3, LOW);

  data->lux[data->resetCount % N_RECORDS] = tcs.lux();
  data->cct[data->resetCount % N_RECORDS] = (uint16_t) tcs.colorTemperature();
}
