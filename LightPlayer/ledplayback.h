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
  return analogRead(A0) >> 5;
}

uint16_t poti;

void setUserOffset(uint16_t minutes) {
  userOffset = (60 + minutes) * MINUTE;
}

// current animation is between keyframes i and i+1, and can be broken into discrete steps
uint16_t i;
uint16_t nSteps;
uint32_t stepDuration; // in ms
uint16_t currentStep;
uint32_t nextStepTime; // millis()

void calculateSteps(bool seekIndex = true) {
  uint32_t targetTime = getTargetTime();
  if (seekIndex) {
    // wind back to earlier if necessary
    while (targetTime < data[i].ts && i > 0) {
      i--;
    }
    while (targetTime > data[i+1].ts && i < ndata-2) {
      i++;
    }
    DEBUG_PRINT("target ");
    DEBUG_PRINT(targetTime);
    DEBUG_PRINT(" -> seeking to ");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(data[i].ts);
    DEBUG_PRINT("-");
    DEBUG_PRINT(data[i+1].ts);
    DEBUG_PRINTLN(")");
  }
  // should be in [0,999]
  uint32_t progress = (1000 * (targetTime - data[i].ts)) / (data[i+1].ts - data[i].ts);

  DEBUG_PRINT("\n\ni=");
  DEBUG_PRINT(i);
  DEBUG_PRINT('.');
  DEBUG_PRINT(progress);
  DEBUG_PRINT('/');
  DEBUG_PRINT(ndata-1);
  DEBUG_PRINT(" (");
  DEBUG_PRINT(data[i+1].ts - data[i].ts);
  DEBUG_PRINT("s)");
  DEBUG_PRINT(": ");
  DEBUG_PRINT(data[i].illuminance);
  DEBUG_PRINT("lux @ ");
  DEBUG_PRINT(data[i].cct);
  DEBUG_PRINT("K -> ");
  DEBUG_PRINT(data[i+1].illuminance);
  DEBUG_PRINT("lux @ ");
  DEBUG_PRINT(data[i+1].cct);
  DEBUG_PRINT("K => ");

  float illuminance = ((1000 - progress) * data[i].illuminance + progress * data[i+1].illuminance) / 1000;
  uint16_t cct = ((1000 - progress) * data[i].cct + progress * data[i+1].cct) / 1000;
  // don't need to have more steps than the difference in pwm levels or cct JND (no more than 100)
  int pwmSteps = max(1, abs(lookupPWM(illuminance) - lookupPWM(data[i+1].illuminance)));
  // TODO limit steps to even lower number based on output resolution
  nSteps = max(pwmSteps, abs(data[i+1].cct - cct) / 100);
  DEBUG_PRINT(nSteps);
  DEBUG_PRINT(" step(s) of ");
  currentStep = 0;
  stepDuration = 1000 * (data[i+1].ts - targetTime) / nSteps;
  nextStepTime = millis();
  DEBUG_PRINT(stepDuration / 1000.0);
  DEBUG_PRINTLN("s each");
}
