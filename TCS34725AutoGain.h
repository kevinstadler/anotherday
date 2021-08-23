uint8_t gain = 1;
uint8_t nCycles = 1;
float integrationTime = 2.4;

// mapping from TCS34725::Gain enum to actual gain factors
const byte tcsGainLevels[4] = { 1, 4, 16, 60 };

void waitUntilData(TCS34725 *tcs, bool singleRead = true) {
  tcs->write8(TCS34725::Reg::ENABLE, (uint8_t) TCS34725::Mask::ENABLE_PON | (uint8_t) TCS34725::Mask::ENABLE_AIEN | (uint8_t) TCS34725::Mask::ENABLE_AEN);
  DEBUG_PRINT(integrationTime);
  DEBUG_PRINT("ms @ ");
  DEBUG_PRINT(gain);
  DEBUG_PRINT('x');
  while (!tcs->available()) {
    DEBUG_PRINT('.');
    delay(integrationTime / 4 + 1); // should add 4ms extra over the whole course
  }
  DEBUG_PRINTLN(tcs->raw().c);
  if (singleRead) {
    tcs->write8(TCS34725::Reg::ENABLE, (uint8_t) TCS34725::Mask::ENABLE_PON | (uint8_t) TCS34725::Mask::ENABLE_AIEN);
  }
}

// DN40 recommendation is green count > 10, clear count > 100
void readWithAutoGain(TCS34725 *tcs, uint16_t minCount = 1000, TCS34725::Gain initGain = TCS34725::Gain::X01) {
  tcs->write8(TCS34725::Reg::ENABLE, (uint8_t) TCS34725::Mask::ENABLE_PON);

  // TAKE 1: minimal cycles, default gain
  const uint16_t minCycles = ceil(minCount / 1024.0 + .5); // heuristic
  integrationTime = 2.4 * minCycles;
  tcs->integrationTime(integrationTime);
  tcs->gain(initGain);
  gain = tcsGainLevels[(int) initGain];

  waitUntilData(tcs);

  TCS34725::RawData raw = tcs->raw();
  if (raw.c >= minCount) {
    return;
  }

  // aggressively increase gain to prevent using uneccessarily long integration times,
  // but without exceeding analog saturation
  
  uint16_t unitGainCountsPerCycle = max(1, raw.c / (gain * minCycles));
  // gain limit that avoids analog saturation (don't go over 950 just in case)
  byte maxGain = min(60, 950 / unitGainCountsPerCycle);
  DEBUG_PRINT("Max gain before analog saturation is ");
  DEBUG_PRINTLN(maxGain);
  byte newGain = 0;
  for (byte i = 0; i < 4; i++) {
    if (tcsGainLevels[3 - i] <= maxGain) {
      newGain = 3 - i;
      break;
    }
  }
  // what multiple of long cycle would i need at this gain to hit the minCount?
  // if the minimum count is within reach, only increase gain.
  byte longCycles = ((raw.c * tcsGainLevels[newGain] / gain) >= minCount) ? 0 : min(6.0, ceil(minCount / (41.7 * tcsGainLevels[newGain]  * unitGainCountsPerCycle)));

  tcs->gain((TCS34725::Gain) newGain);
  gain = tcsGainLevels[newGain];

  // TAKE 2: minimal cycle and/or all 100ms multiples up to 600ms, with adjusted gain.
  // ideal loop with actual measurement length multiples of the base cycle of 2.4ms,
  // each of which is roughly 100ms (i.e. 41.7 cycles): round(1:6 * 41.7)*2.4
  while (longCycles <= 6) {
    // calculate next number of cycles
    nCycles = longCycles == 0 ? minCycles : round(41.7 * longCycles);

//    for (byte i = 0; i < maxGain; i++) {
      // could we reach the threshold with the current gain and integration time?
//      if ((2.4 * nCycles * tcsGainLevels[i] * raw.c) / (integrationTime * gain) >= minCount) {
//        newGain = i;
//        DEBUG_PRINT("New gain: ");
//        DEBUG_PRINTLN(tcsGainLevels[newGain]);
//        break;
//      }
//    }
    integrationTime = 2.4 * nCycles;
    tcs->integrationTime(integrationTime);
    waitUntilData(tcs);

    raw = tcs->raw();
    if (raw.c >= minCount) {
      break;
    }
    longCycles++;
  }
}
