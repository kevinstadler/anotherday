#include <AsyncHTTPRequest_Generic.h>
AsyncHTTPRequest request;

#define DATAFILE "/light.csv"
#define N_ENTRIES 2000

typedef struct LightCondition {
  uint32_t ts;
  float illuminance;
  uint16_t cct;
} LightCondition;

LightCondition data[N_ENTRIES];
uint16_t ndata = 0;

// FIXME 16k.4 response is ok, at 19.5k OOM
void handleResponse(void* optParm, AsyncHTTPRequest* request, int readyState) {
  DEBUG_PRINTLN(readyState);
  switch (readyState) {
    case readyStateHdrsRecvd:
      DEBUG_PRINTLN(request->responseHTTPcode());
      return;
    case readyStateDone:
      DEBUG_PRINT("Received ");
      DEBUG_PRINT(request->available());
      DEBUG_PRINTLN(" bytes");
      // consume Content-Type header
      request->_response->readStringUntil('\n');
      request->_response->readStringUntil('\n');
      setNetworkTime(request->_response->readStringUntil('\n').toInt());
      // peek first entry and see if this was already loaded
      uint32_t firstTime = request->_response->peekStringUntil(',').toInt();
      if (firstTime == data[0].ts) {
        DEBUG_PRINTLN("Data isn't newer than what we already have, skipping.");
        return;
      }
      // loadData
      for (ndata = 0; request->_response->available() && ndata < N_ENTRIES; ndata++) {
        data[ndata].ts = request->_response->readStringUntil(',').toInt();
        data[ndata].illuminance = request->_response->readStringUntil(',').toFloat();
        data[ndata].cct = request->_response->readStringUntil('\n').toInt();
      }
    
      ndata--;
      DEBUG_PRINT(ndata);
      DEBUG_PRINT(" records spanning ");
      DEBUG_PRINT((data[ndata - 1].ts - data[0].ts)/3600.0);
      DEBUG_PRINTLN(" hours");
    
      // check if current network time is somehow out of bounds
      if (now() < data[ndata - 1].ts) {
        DEBUG_PRINTLN("Network time is behind data, setting to last available data point.");
        setNetworkTime(data[ndata - 1].ts);
      }
      nextDownload = max(data[ndata - 1].ts + HOUR, now() + 5 * MINUTE);

      // even if download didn't give new data, networktime might have changed!
      mustSetLight = true;
      DEBUG_PRINT("\nWorking with data that is ");
      DEBUG_PRINT((now() - data[ndata - 1].ts)/3600.0);
      DEBUG_PRINTLN(" hours old.");

      // new data: write it
  //    File file = SPIFFS.open(DATAFILE, "w");
  //    file.print(firstTime);
  //    file.close();
      digitalWrite(2, true);
  }
}

void checkStaleData() {
  if (now() > nextDownload) {
    status = DOWNLOADING;
    DEBUG_PRINT("Data is stale, fetching new");
    digitalWrite(2, false);
    nextDownload = now() + 5 * MINUTE;
    char target[100];
    sprintf(target, "http://192.168.2.2:8000/cgi-bin/light.py?d=%u", 32*OFFSET_RESOLUTION);
    if (request.open("GET", target)) {
      request.send();
    } else {
      DEBUG_PRINTLN("REQUEST FAILED");
      digitalWrite(2, true);
    }
  }
}
