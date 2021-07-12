#include "ESP8266WiFi.h"

volatile bool connected = false;

uint32_t connectStart;

WiFiEventHandler connectedEventHandler, disconnectedEventHandler;

// WiFi.onStationModeConnected([&](const WiFiEventStationModeConnected& evt){
void onConnected(const WiFiEventStationModeGotIP &evt) {
  connected = true;
  DEBUG_PRINT("WiFi connected in ");
  DEBUG_PRINTLN(millis() - connectStart);
}

void onDisconnected(const WiFiEventStationModeDisconnected &event) {
  connected = false;
  DEBUG_PRINTLN("DISCONNECTED");
}

bool startWifi() {
  if (connectedEventHandler == NULL) {
    connectedEventHandler = WiFi.onStationModeGotIP(*onConnected);
    disconnectedEventHandler = WiFi.onStationModeDisconnected(*onDisconnected);
    DEBUG_PRINT("Setting up WiFi...");
    WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(192, 168, 2, 50), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
    connectStart = millis();
    WiFi.begin("QT", "quokkathedog");
    return false;
  }
  if (WiFi.status() == WL_CONNECTED && connected) {
    DEBUG_PRINTLN("Wifi already connected");
    return true;
  }
//  WiFi.forceSleepWake();
  DEBUG_PRINTLN("Waiting for connection...");
  WiFi.waitForConnectResult(15e3);
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("WiFi error status: ");
    DEBUG_PRINTLN(WiFi.status());
    return false;
  }
  return true;
}
