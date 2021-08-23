#include <ESP8266WiFi.h>

bool startWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  uint32_t wifi = millis();
  WiFi.forceSleepWake();
  //if (WiFi.shutdownValidCRC(&nv->wss)) {  // if we have a valid WiFi saved state
  //  memcpy((uint32_t*) &nv->rtcData.rstCount + 1, (uint32_t*) &nv->wss, sizeof(nv->wss)); // save a copy of it
  //}
//  if (WiFi.mode(WIFI_RESUME, &nv->wss)) {  // couldn't resume, or no valid saved WiFi state yet
//    DEBUG_PRINT("Resumed, time til connection: ");
//  } else {
    DEBUG_PRINT("Connecting to WiFi...");
    WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
//    WiFi.setAutoConnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(192, 168, 2, 50), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
    WiFi.begin("QT", "quokkathedog");
//  }
  WiFi.waitForConnectResult(15e3);
  DEBUG_PRINTLN(millis() - wifi);
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("WiFi error status: ");
    DEBUG_PRINTLN(WiFi.status());
    return false;
  }
  return true;
}

void stopWifi() {
//  WiFi.mode(WIFI_SHUTDOWN, &nv->wss); // save the WiFi state for faster reconnection
  WiFi.forceSleepBegin();
}

#include <ESP8266HTTPClient.h>

void getTime() {
  if (startWifi()) {
    WiFiClient stream;
    HTTPClient http;
    http.begin(stream, "http://192.168.2.2:8000/cgi-bin/log.py");
    uint16_t r = http.GET();
    if (r == HTTP_CODE_OK) {
      // until 2.7: http.getStream().parseInt());
      // this will start working from v3:
      setNetworkTime(http.getString().toInt());
      DEBUG_PRINT("Network time is ");
      DEBUG_PRINTLN(data->networkTime);
    } else {
      DEBUG_PRINT("HTTP response not ok: ");
      DEBUG_PRINTLN(r);
    }
    http.end();
  }
}
