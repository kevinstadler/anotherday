// file persistence
#include <FS.h>
#define FILE "/light.txt"

// functions for transferring rtc log cache to spiffs

void writeLine(File *file, LogRecord *record) {
  file->print('\t');
  file->print(record->bootCount);
  file->print('\t');
  file->print(record->sSinceNetworkTime);
  file->print('\t');
  file->print(record->gainCyclesV >> 24);
  file->print('\t');
  file->print((uint8_t) (record->gainCyclesV >> 16));
  file->print('\t');
  file->print(record->cr >> 16);
  file->print('\t');
  file->print(record->cr & 0xFFFF);
  file->print('\t');
  file->print(record->gb >> 16);
  file->print('\t');
  file->print(record->gb & 0xFFFF);
  file->print('\t');
  file->print(record->gainCyclesV & 0xFFFF);
}

void persistCache(File *file, bool leaveOpen = false) {
  for (byte i = 0; i < data->cacheCount; i++) {
    writeLine(file, &(cache[i]));
    if (leaveOpen && i+1 == data->cacheCount) {
      break;
    }
    // skip: temp, humidity, uploadattempt
    file->println("\t\t\t");
  }
  data->cacheCount = 0;
}

uint32_t uploadFile() {
  uint32_t ret = 0;
  if (startWifi()) {
    File file = SPIFFS.open(FILE, "r");
    HTTPClient http;
    DEBUG_PRINT("Sending HTTP request...");
    http.begin("http://192.168.2.2:8000/cgi-bin/log.py?file=lux.txt&mode=a&currentTimeEstimate=" + String(currentTime()));
    int code = http.sendRequest("POST", &file, file.size());
    if (code == HTTP_CODE_OK) {
      ret = http.getStream().parseInt();
      setNetworkTime(ret);
      file.close();
      DEBUG_PRINT("success.");
      // truncate by opening in write mode, it gets closed immediately below
      file = SPIFFS.open(FILE, "w");
    } else {
      DEBUG_PRINT("failed: ");
      DEBUG_PRINT(code);
    }
    file.close();
    http.end();
  }
  stopWifi();
  return ret;
}
