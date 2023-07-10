#include "DataSerial.h"
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "HandleWifi.h"
#include "Config.h"

typedef enum {
  DATA,
  SET_TIME,
  CONTROL,
  PUMP_ON,
  PUMP_OFF,
  RESET,
  READY
} Cmd;

static StaticJsonDocument<255> dataSerialDoc;
static HardwareSerial dataSerial(2);
static DataStruct *data_struct;
static bool ready = false;

static void sendControl() {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = CONTROL;
  dataSerialDoc["limit"] = data_struct->limit;
  int i;
  for (i = 0; i < 5; i++)
    dataSerialDoc["timer"][i] = data_struct->timer[i];
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendTimeSet(int year, int month, int day, int hour, int minute, int second) {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = SET_TIME;
  dataSerialDoc["year"] = year;
  dataSerialDoc["month"] = month;
  dataSerialDoc["hour"] = hour;
  dataSerialDoc["minute"] = minute;
  dataSerialDoc["second"] = second;
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendPumpOn() {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = PUMP_ON;
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendPumpOff() {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = PUMP_OFF;
  serializeJson(dataSerialDoc, dataSerial);
}

static bool isGotDataSerial() {
  if (dataSerial.available()) {
    dataSerialDoc.clear();
    DeserializationError error = deserializeJson(dataSerialDoc, dataSerial);
    if (error) {
      Serial.print("ERROR: ");
      Serial.println(error.c_str());
      return false;
    }
    return true;
  }
  return false;
}

static void sendReady() {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = READY;
  dataSerialDoc["resp"] = true;
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendReadyWithoutResponse() {
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = READY;
  dataSerialDoc["resp"] = false;
  serializeJson(dataSerialDoc, dataSerial);
}

static void handle() {
  if (isGotDataSerial()) {
    Cmd cmd = dataSerialDoc["cmd"];
    switch (cmd) {
      case DATA:
        data_struct->temp = dataSerialDoc["temp"];
        data_struct->humidity = dataSerialDoc["humi"];
        data_struct->soil = dataSerialDoc["soil"];
        data_struct->pump_on = dataSerialDoc["pump"];
        break;
      case READY:
        data_struct->limit = dataSerialDoc["limit"];
        int i;
        for (i = 0; i < 5; ++i)
          data_struct->timer[i] = dataSerialDoc["timer"][i];
        if (ready)
          sendReadyWithoutResponse();
        ready = true;
        break;
      case RESET:
        HandleWifi.clear();
        ESP.restart();
        break;
    }
  }
}

static void begin(DataStruct *ds) {
  dataSerial.begin(9600, SERIAL_8N1, RX, TX);
  while (!dataSerial);
  data_struct = ds;
  sendReady();
  unsigned long t = millis();
  while (!ready) {
    handle();
    if ( (unsigned long)(millis() - t) > 1000 ) {
      t = millis();
      sendReady();
    }
  }
}

struct DataSerial_t DataSerial = {
  .begin = begin,
  .handle = handle,
  .sendControl = sendControl,
  .sendPumpOn = sendPumpOn,
  .sendPumpOff = sendPumpOff,
  .isGotDataSerial = isGotDataSerial,
  .sendTimeSet = sendTimeSet,
};
