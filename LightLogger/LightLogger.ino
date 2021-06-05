// _INTERVAL constants
#include "timing.h"

// RTC memory data storage between reboots
#include "rtc.h"

// connection
#include "wifi.h"
// file persistence
#include <FS.h>
#define FILE "/light.txt"
File file;

#include "TCS34725.h"
TCS34725 tcs;
#include "light.h"

#include <DHTesp.h>
DHTesp dht;
TempAndHumidity dhtData;

void setup() {
  // TODO if voltage is too low, go straight to sleep
  
  Serial.begin(74880);
 
  if (data->networkTime == 0 || !ESP.getResetReason().equals("Deep-Sleep Wake")) {
    Serial.println("Resetting");
    data->sleepTime = RESET_INTERVAL;
    data->resetCount = 0;
    data->msSinceNetworkTime = - millis();
    data->uploadAttempts = 0;
    // connect to wifi
    getTime();
  }

  if (data->networkTime == 0) {
    data->sleepTime *= 2;
    Serial.println("Sleeping prematurely");
    sleep();
    // buhao
  }

//  Serial.print("Reset #");
//  Serial.println(data->resetCount);

  SPIFFS.begin();
  file = SPIFFS.open(FILE, "a");
  file.print(data->resetCount);
  file.print(",");
  if (data->uploadAttempts >= 0) {
    file.print(data->uploadAttempts);
  }
  file.print(",");
  file.print(data->networkTime);
  file.print(",");
  file.print((data->msSinceNetworkTime + millis()) / 1000);
  file.print(",");

  getLight(file);
//  file.print(data->lux[data->resetCount % N_RECORDS]);
//  file.print(",");
//  file.print(data->cct[data->resetCount % N_RECORDS]);

  file.print(",");

  // FIXME only measure voltage if wifi is on?
  getVoltage();
  file.print(data->v[data->resetCount % N_RECORDS]);
  file.print(",");

  // don't need to get temperature every time -- every 5.something minutes max
  if (data->shouldUpload >= 0 || (data->resetCount * data->sleepTime) % 320 == 0) {
    getAir();
    file.print(dhtData.temperature);
    file.print(",");
    file.print((int16_t) dhtData.humidity);
  } else {
    file.print(",");
  }
  file.println();

  int fileSize = file.size();
  file.close();

  // TODO check if Wifi (modem) is on instead?
  if (data->uploadAttempts >= 0) {
    // TODO incorporate time correction (RTC drift)?
    if (uploadFile(FILE) != 0) {
      fileSize = 0;
      data->uploadAttempts = -1;
    } else {
      data->uploadAttempts++;
    }
  }

  // at 5k filesize constraint and 40 sec write interval there was one upload every ~1.2 hours,
  // each of which triggered the battery to recharge again.
  // TODO add OR 1h expired?
  if (data->uploadAttempts >= 0) {
    data->uploadAttempts++;
  } else if (data->resetCount == 0 ||
             ((data->v[data->resetCount % N_RECORDS] >= 740 && fileSize > 20000))) {
    data->uploadAttempts = 0;
  }

  // TODO calculate deep sleep time based on battery voltage
  // https://electronics.stackexchange.com/questions/32321/lipoly-battery-when-to-stop-draining

  data->sleepTime = RESET_INTERVAL;
  sleep();
}

void getVoltage() {
  // 5.09 (usb) = 1016/1017
  // 4.18 (batt no load) = 834/835
  // 4.15 (batt w/ load) = 831
  data->v[data->resetCount % N_RECORDS] = analogRead(A0);
  // * .005 to get to voltage, +- 2%ish
  // => 4.2V = 840
  // => 4V = 800 (at least 85% battery here)
}
// 3.9 -> 80% (780)
// 3.8 -> 60% (760)
// 3.7 -> 40% (740)
// 3.6 -> 20% (720)
// 3.5 -> 10% (700)

void getAir() {
  pinMode(D7, OUTPUT);
  digitalWrite(D7, HIGH);
  delay(1000);
  dht.setup(D6, DHTesp::DHT22);
  delay(1000);
  dhtData = dht.getTempAndHumidity();
  if (dht.getStatus() != DHTesp::ERROR_NONE) {
    Serial.printf("Error reading local sensor: %s\n", dht.getStatusString());
  }
  digitalWrite(D7, LOW);
}

void loop() {
  // should not get here
  sleep(0);
}
