#define MINUTE 60
#define HOUR 60*60

uint32_t lastNetworkTime;
uint32_t msAtLastNetworkTime;

void setNetworkTime(uint32_t networkTime) {
  msAtLastNetworkTime = millis();
  lastNetworkTime = networkTime;
  // TODO if networkTime - userOffset exceeds the data, need to update dataOffset!
}

uint32_t now() {
  return lastNetworkTime + (millis() - msAtLastNetworkTime) / 1000;
}
