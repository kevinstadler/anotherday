#include <ESP8266WiFi.h>
#include <include/WiFiState.h> // WiFiState structure details

typedef struct RtcData {
// TODO store/check state https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/RTCUserMemory/RTCUserMemory.ino
//  uint32_t crc32;
  uint32_t bootCount;
  uint32_t networkTime;
  int32_t msSinceNetworkTime;
  uint32_t nextUpload; // count
  uint32_t nextAir; // ms since network time
  uint32_t readInterval; // in ms
  float lastLux;
  uint32_t lastCct;
  uint32_t uploadAttempt;
  uint32_t cacheCount; // this is the next index that will be written to (up to LOG_CACHE-1)
} RtcData;

// cache data and only persist into SPIFFS when full
// 1 log line is count(int32) + ms(int32) + gain(byte) + ncycles(byte) + 4x16bit + voltage(int16) = 20 byte
typedef struct LogRecord {
  uint32_t bootCount;
  uint32_t sSinceNetworkTime;
  // bytes don't work so gotta get a bit messy...
  uint32_t gainCyclesV;
  uint32_t cr;
  uint32_t gb;
} LogRecord;

#define LOG_CACHE 20

// RTC user memory is 128 * 32bit = 512 byte
struct nv_s {
  // core's WiFi save state is 152 byte, leaving 360 byte
//  WiFiState wss;
  // this is 40 byte
  RtcData rtcData;
  // so could cache up to 23 log lines at a time
  LogRecord cache[LOG_CACHE];
};
static nv_s* nv = (nv_s*) RTC_USER_MEM; // user RTC RAM area

// since Arduino 3.*, THESE TWO STRUCTURES WORK FOR READING ONLY
static RtcData *data = &nv->rtcData;
static LogRecord *cache = nv->cache;

void setNextUpload(uint32_t nextUpload) {
  // only allow *increasing* this value
  nv->rtcData.nextUpload = max(nextUpload, data->nextUpload);
}

bool shouldUpload() {
  return data->bootCount == data->nextUpload;
}
