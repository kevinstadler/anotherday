//#define DEBUG 1
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// RTC memory data storage between reboots
#include "rtc.h"
// _INTERVAL constants
#include "timing.h"

// connection
#include "wifi.h"

#include "spiffs.h"
#include "light.h"

#include <DHTesp.h>
DHTesp dht;
TempAndHumidity dhtData;

uint16_t v;

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println();
  #endif
 
  // if voltage is too low, go straight to sleep for an hour
  v = getVoltage();
/*  if (v < 700) {
    DEBUG_PRINTLN("Low power");
    sleep(60*MINUTE);
  }*/

  if (!ESP.getResetReason().equals("Deep-Sleep Wake")) {
    DEBUG_PRINTLN("Looks like first boot, initialising.");
    delay(2000); // give some time to flash
    nv->rtcData.networkTime = 0;
    nv->rtcData.msSinceNetworkTime = - millis();
    nv->rtcData.bootCount = 0;
    nv->rtcData.nextUpload = 0;
    nv->rtcData.nextAir = 0;
    nv->rtcData.readInterval = RESET_INTERVAL;
    nv->rtcData.lastLux = 0;
    nv->rtcData.lastCct = 0;
    nv->rtcData.uploadAttempt = 0;
    nv->rtcData.cacheCount = 0;
//    DEBUG_PRINTLN(sizeof(*(&nv->wss))); // 152
//    DEBUG_PRINTLN(sizeof(*(&nv->rtcData))); // 40
//    DEBUG_PRINTLN(sizeof(*(&nv->cache))); // 120 for 6
//    DEBUG_PRINTLN(sizeof(*nv)); // 304 for 6
//    DEBUG_PRINT("Wiping FS....");
//    DEBUG_PRINTLN(LittleFS.format() ? "success!" : "failed!");
  }
}

void loop() {
  int fileSize = 0;
  // if it's data worth writing OR we're about to upload
  if (getLight() || shouldUpload()) {

    DEBUG_PRINT("Writing to cache slot ");
    DEBUG_PRINTLN(data->cacheCount);
    nv->cache[data->cacheCount].bootCount = data->bootCount;
    nv->cache[data->cacheCount].sSinceNetworkTime = (data->msSinceNetworkTime + millis()) / 1000;
    nv->cache[data->cacheCount].gainCyclesV = (gain << 24) | (nCycles << 16) | v;
    nv->cache[data->cacheCount].cr = (raw.c << 16) | raw.r;
    nv->cache[data->cacheCount].gb = (raw.g << 16) | raw.b;

    // persistence and/or upload
    if (++(nv->rtcData.cacheCount) == LOG_CACHE || shouldUpload()) {

      if (shouldUpload()) {
        // TODO fix disconnect bug: https://github.com/esp8266/Arduino/issues/5527
        // https://github.com/esp8266/Arduino/issues/2235#issuecomment-248916617 says to just
        // call disconnect() beforehand.
        WiFi.disconnect();
      }

      if (data->bootCount == 0) {
        // connect to wifi to get timestamp before first write
        DEBUG_PRINTLN("Getting initial network time");
        getTime();
        if (data->networkTime == 0) {
          // buhao
          nv->rtcData.readInterval = max(data->readInterval, data->readInterval << 1);
          DEBUG_PRINTLN("Initial clock check failed, scheduling another connection in 2");
          nv->rtcData.nextUpload += 2;
        }
      }

      if (!LittleFS.begin()) {
        DEBUG_PRINTLN("FS error, shutting down");
        sleep(0);
      }
      DEBUG_PRINTLN("Persisting cache");

      #ifdef DEBUG
        FSInfo info;
        LittleFS.info(info);
        DEBUG_PRINT("FS status: ");
        DEBUG_PRINT(info.usedBytes);
        DEBUG_PRINT(" bytes used of total ");
        DEBUG_PRINTLN(info.totalBytes);
      #endif

      File file = LittleFS.open(FILE, shouldUpload() ? "a+" : "a");

      // don't need to get temperature every time -- every 5.something minutes max,
      // and only if we have enough time (3 seconds) between light data reads
      bool logAir = data->msSinceNetworkTime >= data->nextAir && data->readInterval >= 3e3;

      bool leaveOpen = logAir || shouldUpload();
      persistCache(&file, leaveOpen);
      if (leaveOpen) {
        file.print('\t');
        getAir();
        file.print(dhtData.temperature);
        file.print('\t');
        file.print((int16_t) dhtData.humidity);
        nv->rtcData.nextAir = data->msSinceNetworkTime + AIR_INTERVAL;
        file.print('\t');
        if (shouldUpload()) {
          file.print(++(nv->rtcData.uploadAttempt));
          // leave last line open for possible error code
          file.print('\t');
        } else {
          file.println('\t');
        }
      }
    
      fileSize = file.size();
    
      // at 5k filesize constraint and 40 sec write interval there was one upload every ~1.2 hours,
      // each of which triggered the battery to recharge again.
      if (!shouldUpload()) {
        file.close();
      } else {
        digitalWrite(D4, false);
        if (uploadFile(&file)) {
          fileSize = 0;
          nv->rtcData.uploadAttempt = 0;
        } else {
          // exponentially delay upload
          setNextUpload(data->nextUpload + ( data->uploadAttempt >= 8 ? 255 : (1 << data->uploadAttempt)));
        }
        digitalWrite(D4, true);
      }
      LittleFS.end();
    }
  }

  // decide if next reboot should be upload
  if (v >= 740 && // enough power
      data->readInterval > 10e3 && // at least 10s read interval so we don't mess up dense readings
      // consider next upload time -- 50KB size or 1 hour, whichever comes first
      (fileSize > 50000 || data->msSinceNetworkTime >= 60*60e3)) {
    DEBUG_PRINTLN("Queue upload attempt at next reboot");
    setNextUpload(data->bootCount + 1);
  }

  DEBUG_PRINTLN();
  sleep();
}

// * .005 to get to voltage, +- 2%ish
// 5.09 (usb) = 1016/1017
// 4.18 (batt no load) = 834/835
// 4.15 (batt w/ load) = 831
// 3.9 -> 80% (780)
// 3.8 -> 60% (760)
// 3.7 -> 40% (740)
// 3.6 -> 20% (720)
// 3.5 -> 10% (700)
uint16_t getVoltage() {
  return analogRead(A0);
}

void getAir() {
  pinMode(D7, OUTPUT);
  digitalWrite(D7, HIGH);
  delay(1000);
  dht.setup(D6, DHTesp::DHT22);
  delay(1000);
  dhtData = dht.getTempAndHumidity();
  if (dht.getStatus() != DHTesp::ERROR_NONE) {
    DEBUG_PRINTLN("Error reading local sensor");
    // dht.getStatusString()
  }
  digitalWrite(D7, LOW);
}
