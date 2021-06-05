byte gain;
float integrationTime;

void readWithAutoGain(TCS34725 *tcs, uint16_t minCount = 1000, bool discardFirst = true) {
    tcs->gain(TCS34725::Gain::X01);
    tcs->enableColorTempAndLuxCalculation(true);
    TCS34725::RawData raw;

    gain = 1;
    // try up to 600ms
    // ideal loop with actual cycle multiples of 2.4, each of which is roughly 100ms: round(1:6 * 41.7)*2.4
    for (byte l = 0; l <= 6; l++) {
      // calculate good number of cycles
      float newIntegrationTime = 2.4 * (l == 0 ? 2 : round(41.7 * l));
      tcs->integrationTime(newIntegrationTime);
      if (discardFirst) {
        // consume one away
        delay(integrationTime);
        tcs->available();
      }
      integrationTime = newIntegrationTime;
      while (!tcs->available()) {
        delay(integrationTime / 4);
      }
      raw = tcs->raw();
      if (raw.c >= minCount) {
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

      float fulfillmentPotentialAtCurrentGain = raw.c == 0 ? 60 : integrationTime * minCount / (600 * raw.c * gain);
      // 1, 4, 16, 60
      if (fulfillmentPotentialAtCurrentGain >= 4) {
        if (fulfillmentPotentialAtCurrentGain >= 16) {
          if (fulfillmentPotentialAtCurrentGain >= 60) {
            tcs->gain(TCS34725::Gain::X60);
            gain = 60;
          } else {
            tcs->gain(TCS34725::Gain::X16);
            gain = 16;
          }
        } else {
          tcs->gain(TCS34725::Gain::X04);
          gain = 4;
        }
//        Serial.println("Switching gain to ");
//        Serial.println(gain);
      }
    }
  }
