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
  if (WiFi.mode(WIFI_RESUME, &nv->wss)) {  // couldn't resume, or no valid saved WiFi state yet
    Serial.print("resumed, time til connection: ");
  } else {
    Serial.print("can't resume, first config, time til connection: ");
    WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(192, 168, 2, 50), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
    WiFi.begin("QT", "quokkathedog");
  }
  WiFi.waitForConnectResult(15e3);
  Serial.println(millis() - wifi);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi error status: ");
    Serial.println(WiFi.status());
    return false;
  }
  return true;
}

void stopWifi() {
  WiFi.mode(WIFI_SHUTDOWN, &nv->wss); // save the WiFi state for faster reconnection
  WiFi.forceSleepBegin();
}

#include <ESP8266HTTPClient.h>

void getTime() {
  if (startWifi()) {
    HTTPClient http;
    http.begin("http://192.168.2.2:8000/cgi-bin/log.py");
    if (http.GET() == HTTP_CODE_OK) {
      // this will start working from v3
      //ret = http.getString().toInt();
      setNetworkTime(http.getStream().parseInt());
      Serial.print("Network time is ");
      Serial.println(data->networkTime);
    } else {
      Serial.println("HTTP response not ok");
    }
    http.end();
  }
}

uint32_t uploadFile(String filename) {
  uint32_t ret = 0;
  if (startWifi()) {
    File file = SPIFFS.open(filename, "r");
    HTTPClient http;
    http.begin("http://192.168.2.2:8000/cgi-bin/log.py?file=lux.txt&mode=a&currentTimeEstimate=" + String(currentTime()));
    int code = http.sendRequest("POST", &file, file.size());
    if (code == HTTP_CODE_OK) {
      ret = http.getStream().parseInt();
      setNetworkTime(ret);
      file.close();
      // truncate by opening in write mode, it gets closed immediately below
      file = SPIFFS.open(filename, "w");
    } else {
      Serial.print("failed: ");
      Serial.println(code);
    }
    file.close();
    http.end();
  }
  stopWifi();
  return ret;
}
