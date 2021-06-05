#include <ESP8266WiFi.h>
#include <include/WiFiState.h> // WiFiState structure details

#define N_RECORDS 5

// you can add anything else here that you want to save, must be 4-byte aligned
typedef struct RtcData {
// TODO store/check state https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/RTCUserMemory/RTCUserMemory.ino
  uint32_t crc32;
  uint32_t networkTime;
  int32_t msSinceNetworkTime;
  uint32_t sleepTime = RESET_INTERVAL; // in seconds
  float lux[N_RECORDS];
  uint16_t cct[N_RECORDS];
  uint16_t v[N_RECORDS];
  int32_t uploadAttempts; // -1 = no upload, otherwise count up
  uint32_t resetCount;
} RtcData;

struct nv_s {
  WiFiState wss; // core's WiFi save state
  RtcData rtcData;
};
static nv_s* nv = (nv_s*) RTC_USER_MEM; // user RTC RAM area
static RtcData *data = &nv->rtcData;

void sleep(int seconds) {
  data->resetCount++;
/*  Serial.print("Sleeping for ");
  Serial.print(data->sleepTime);
  if (data->shouldUpload) {
    Serial.println(" then Wifi!");
  } else {
    Serial.println(" then NO Wifi!");
  } */
  Serial.print("Sleeping for ");
  Serial.println(seconds);
  if (data->uploadAttempts >= 0) {
    Serial.println("Upload attempt on next wake!");
  }
  data->msSinceNetworkTime += millis() + 1e3 * seconds;
  // TODO if (rebootCount - lastUpload - uploadInterval) == (1 << uploadAttempts)
  ESP.deepSleep(seconds * 1e6, (data->uploadAttempts >= 0) ? WAKE_RF_DEFAULT : WAKE_RF_DISABLED); //WAKE_NO_RFCAL
}

void sleep() {
  sleep(data->sleepTime);
}

void setNetworkTime(uint32_t time) {
  if (time != 0) {
    data->msSinceNetworkTime = - millis();
    data->networkTime = time;
  }
}

uint32_t currentTime() {
  return data->networkTime + ( data->msSinceNetworkTime + millis() ) / 1000;
}
