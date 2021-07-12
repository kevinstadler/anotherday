#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>

#define DATAFILE "/light.csv"
#define N_ENTRIES 2000

typedef struct LightCondition {
  uint32_t ts;
  float illuminance;
  uint16_t cct;
} LightCondition;

LightCondition data[N_ENTRIES];
uint16_t ndata = 0;

bool downloadData();

void loadData() {
  if (!SPIFFS.exists(DATAFILE)) {
    if (!downloadData()) {
      DEBUG_PRINTLN("NO WAY");
      ESP.deepSleep(0);
    }
  }
  DEBUG_PRINTLN("Reading time series data from file");
  File file = SPIFFS.open(DATAFILE, "r");
  for (ndata = 0; file.available() && ndata < N_ENTRIES; ndata++) {
    data[ndata].ts = file.parseInt();
    data[ndata].illuminance = file.parseFloat();
    data[ndata].cct = file.parseInt();
  }
  file.close();

  ndata--;
  DEBUG_PRINT(ndata);
  DEBUG_PRINT(" records spanning ");
  DEBUG_PRINT((data[ndata - 1].ts - data[0].ts)/60);
  DEBUG_PRINTLN(" minutes");

  // check if current network time is somehow out of bounds
  if (now() < data[ndata - 1].ts) {
    DEBUG_PRINTLN("Network time is behind data, setting to last available data point.");
    setNetworkTime(data[ndata - 1].ts);
  }
}

bool downloadData() {
  digitalWrite(D4, false);
  if (!startWifi()) {
    return false;
  }
  // TODO switch to async
  // https://github.com/boblemaire/asyncHTTPrequest
  // with callback: https://github.com/khoih-prog/AsyncHTTPRequest_Generic/blob/master/examples/AsyncHTTPRequest_ESP/AsyncHTTPRequest_ESP.ino
  HTTPClient http;
  http.begin("http://192.168.2.2:8000/cgi-bin/light.py");
  if (http.GET() != HTTP_CODE_OK) {
    DEBUG_PRINTLN("HTTP status not OK");
    return false;
  }
  WiFiClient in = http.getStream();
  DEBUG_PRINT("Received ");
  DEBUG_PRINT(in.available());
  DEBUG_PRINTLN(" bytes");
  setNetworkTime(in.parseInt());
  // peak first entry and see if this was already loaded
  uint32_t firstTime = in.parseInt();
  if (firstTime == data[0].ts) {
    DEBUG_PRINTLN("Data isn't newer than what we already have, skipping.");
    return false;
  }
  // new data: write it
  File file = SPIFFS.open(DATAFILE, "w");
  file.print(firstTime);
  while (in.available()) {
    file.println(in.readStringUntil('\n'));
  }
  file.close();
  http.end();
  digitalWrite(D4, true);
  return true;
}
