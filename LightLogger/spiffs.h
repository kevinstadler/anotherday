// file persistence
//#include <FS.h>
#include <LittleFS.h>
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
  // pre-log timestamp to the first line that will be persisted
  // if the file has already been started, print a newline first
  if (file->size() == 0 || data->bootCount == 0) {
    DEBUG_PRINTLN(file->size());
    file->println(); // always lead in with a newline
//    file->print(data->networkTime, 16);
    file->print(data->networkTime);
    DEBUG_PRINTLN(file->size());
  }

  for (byte i = 0; i < data->cacheCount; i++) {
    writeLine(file, &(cache[i]));
    if (leaveOpen && i+1 == data->cacheCount) {
      break;
    }
    // skip: temp, humidity, uploadattempt, error code
    file->println("\t\t\t\t");
    // TODO test if this is enough newlines everywhere
    // TODO newline required before new timestamp
  }
  nv->rtcData.cacheCount = 0;
}

// FIXME write() writes the raw byte instead of a string representation??

bool uploadFile(File *file) {
  int32_t code = -999;
  digitalWrite(D4, LOW);
  if (startWifi()) {
    WiFiClient stream;
    HTTPClient http;
    DEBUG_PRINT("Sending HTTP request...");
    http.begin(stream, "http://192.168.2.2:8000/cgi-bin/log.py?file=lux.txt&mode=a&currentTimeEstimate=" + String(currentTime()));
    // on SPIFFS, read and write pointer apparently share position..
    file->seek(0);
    code = http.sendRequest("POST", file, file->size());
    if (code == HTTP_CODE_OK) {
      uint32_t ts = http.getString().toInt();
      if (ts < data->networkTime) { // don't accept timestamps in the past (or 0)
        DEBUG_PRINT("HTTP request succeeded but timestamp is devious, assuming data not saved.");
        code = -666;
      } else {
        DEBUG_PRINT("success. new timestamp: ");
        DEBUG_PRINTLN(ts);
        setNetworkTime(ts);
      }
    }
    http.end();
  } else {
    code = WiFi.status();
  }
  stopWifi();
  digitalWrite(D4, HIGH);

  if (code != HTTP_CODE_OK) {
    // write error code+newline to file
    DEBUG_PRINT("error: ");
    DEBUG_PRINTLN(code);
    file->println(code);
    file->close();
    return false;
  } else {
    // truncate file
    file->close();
    LittleFS.open(FILE, "w").close();
    return true;
  }
}
