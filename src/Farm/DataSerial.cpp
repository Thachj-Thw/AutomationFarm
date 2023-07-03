#include "DataSerial.h"
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "TimeNTP.h"
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
static bool begined = false;
static unsigned long delta, mms;


static void sendControl() {
  if (!begined)
    return;
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = CONTROL;
  dataSerialDoc["limit"] = data_struct->limit;
  int i;
  for (i = 0; i < 5; i++)
    dataSerialDoc["timer"][i] = data_struct->timer[i];
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendCurrentTime() {
  if (!begined)
    return;
  mms = TimeNTP.getCurrentMillis();
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = SET_TIME;
  dataSerialDoc["value"] = mms;
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendPumpOn() {
  if (!begined)
    return;
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = PUMP_ON;
  serializeJson(dataSerialDoc, dataSerial);
}

static void sendPumpOff() {
  if (!begined)
    return;
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
  mms = TimeNTP.getCurrentMillis();
  dataSerialDoc.clear();
  dataSerialDoc["cmd"] = READY;
  dataSerialDoc["time"] = mms;
  serializeJson(dataSerialDoc, dataSerial);
}

static void begin(DataStruct *ds) {
  dataSerial.begin(9600, SERIAL_8N1, RX, TX);
  data_struct = ds;
  TimeNTP.begin();
  delta = millis();
  sendReady();
  begined = true;
}

static void handle() {
  if (!begined)
    return;
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
        break;
      case RESET:
        HandleWifi.clear();
        ESP.restart();
        break;
    }
  }
  mms += (unsigned long)(millis() - delta);
  delta = millis();
  if (mms >= 86400000)
    sendCurrentTime();
}

struct DataSerial_t DataSerial = {
  .begin = begin,
  .handle = handle,
  .sendControl = sendControl,
  .sendPumpOn = sendPumpOn,
  .sendPumpOff = sendPumpOff,
  .isGotDataSerial = isGotDataSerial,
};
