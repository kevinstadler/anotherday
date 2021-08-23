// how often does one need to do/store a measurement of a temporal variable
// when one wants to be sure to capture all changes which exceed the just
// noticable different of the variable, but not (much) more often that?

#define SECOND 1e3
#define MINUTE 60e3
#define AIR_INTERVAL 5*MINUTE

// all times in ms -- all intervals should be powers of the lowest one
#define MIN_INTERVAL uint32_t(SECOND / 8)
#define MAX_INTERVAL uint32_t(MINUTE)

#define RESET_INTERVAL uint32_t(SECOND)
#define MAX_INCREASE uint32_t(10*SECOND)

// if (newest - mean) > jnd -> RESET_INTERVAL
// else look at sd : mean ratio
// sd / mean > ? -> HALF interval
// sd / mean < ? -> DOUBLE interval

void sleep(uint32_t ms) {
  DEBUG_PRINT("Sleeping for ms");
  DEBUG_PRINTLN(ms);
  if (data->nextUpload > data->bootCount) {
    DEBUG_PRINT("Next upload after ");
    DEBUG_PRINT(data->nextUpload - data->bootCount);
    DEBUG_PRINTLN(" reboots");
  }

  nv->rtcData.bootCount++;
  if (data->nextUpload == data->bootCount) {
    DEBUG_PRINTLN("Upload attempt on next wake!");
  }
  #ifdef DEBUG
    Serial.flush();
  #endif
  nv->rtcData.msSinceNetworkTime += millis() + ms;
  ESP.deepSleep(ms * 1e3, (data->nextUpload == data->bootCount) ? WAKE_RF_DEFAULT : WAKE_RF_DISABLED); //WAKE_NO_RFCAL
}

void sleep() {
  // deduct to make up for time spent doing upload, measurements etc.
  sleep((millis() + 50 > data->readInterval) ? 50 : data->readInterval - millis());
}

void setNetworkTime(uint32_t time) {
  if (time != 0) {
    nv->rtcData.nextAir -= data->msSinceNetworkTime;
    nv->rtcData.msSinceNetworkTime = -millis();
    nv->rtcData.networkTime = time;
  }
}

uint32_t currentTime() {
  return data->networkTime + ( data->msSinceNetworkTime + millis() ) / 1000;
}
