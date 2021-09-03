// which state properties follow from what:
// networkTime + millis() + offset => timestamp whose data should be displayed right now
// timestamp => data index + offset/progress into next data point => pwm + timer delay

// all offsets are in seconds before now
// if the age of the latest datapoint exceeds the smallest poti offset, this one needs to be increased
uint32_t dataOffset;
// this one is controlled by the user/poti input
uint16_t userOffset;

uint32_t getTargetTime() {
  return now() - dataOffset - userOffset;
}

byte readPoti() {
  // >> 5 for 32 levels with great stability
  // >> 4 for 64 levels can use for extra sensitivity
  return analogRead(A0) >> 5;
}

uint16_t poti;

void setUserOffset(uint16_t units) {
  userOffset = 60 * MINUTE + units * OFFSET_RESOLUTION;
}

uint16_t i;
void setLight(bool reposition = true) {
  DEBUG_PRINTLN();
  if (reposition) {
    uint32_t targetTime = getTargetTime();
    // wind back to earlier if necessary
    while (targetTime < data[i].ts && i > 0) {
      i--;
    }
    while (targetTime > data[i+1].ts && i < ndata-2) {
      i++;
    }
/*    DEBUG_PRINT("target ");
    DEBUG_PRINT(targetTime);
    DEBUG_PRINT(" -> seeking to ");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(data[i].ts);
    DEBUG_PRINT("-");
    DEBUG_PRINT(data[i+1].ts);
    DEBUG_PRINTLN(")");*/
  }
  DEBUG_PRINT(i+1);
  DEBUG_PRINT("/");
  DEBUG_PRINT(ndata);
  DEBUG_PRINT(": ");
  DEBUG_PRINT(data[i].illuminance);
  DEBUG_PRINT(" lux @ ");
  DEBUG_PRINT(data[i].cct);
  DEBUG_PRINT(" K (for ");
  DEBUG_PRINT(data[i+1].ts - data[i].ts);
  DEBUG_PRINT("s) ");
  uint32_t intensity = lookupPWM(data[i].illuminance);
  float cold = lookupColdPortion(data[i].cct);
  if (cold == 0 || cold == 1) {
    DEBUG_PRINT("FLASH WARNING: ");
  }
  targetCold = ceil(cold * intensity);
  targetWarm = ceil((1 - cold) * intensity);
}
