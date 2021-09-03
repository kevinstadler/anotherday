#include "ESP8266WiFi.h"

WiFiEventHandler connectedEventHandler, disconnectedEventHandler;
uint32_t connectStart;

// WiFi.onStationModeConnected([&](const WiFiEventStationModeConnected& evt){
void onConnected(const WiFiEventStationModeGotIP &evt) {
  status = CONNECTED;
  DEBUG_PRINT("WiFi connected in ");
  DEBUG_PRINT(millis() - connectStart);
  DEBUG_PRINT("...");
  checkStaleData();
}
void onDisconnected(const WiFiEventStationModeDisconnected &event) {
  status = DISCONNECTED;
}

void setupWifi() {
  request.setTimeout(10);
  request.onReadyStateChange(handleResponse);
  connectedEventHandler = WiFi.onStationModeGotIP(*onConnected);
  disconnectedEventHandler = WiFi.onStationModeDisconnected(*onDisconnected);
  WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(192, 168, 2, 50), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
  connectStart = millis();
  WiFi.begin("QT", "quokkathedog");
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED && status >= CONNECTED) {
//    DEBUG_PRINTLN("Wifi already connected");
    return;
  }
//  WiFi.forceSleepWake();
  DEBUG_PRINTLN("Waiting for connection...");
/*  WiFi.waitForConnectResult(15e3);
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("WiFi error status: ");
    DEBUG_PRINTLN(WiFi.status());
    return false;
  }
  */
}
