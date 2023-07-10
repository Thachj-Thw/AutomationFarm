#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "HandleWifi.h"
#include "ESP32Camera.h"
#include "Html.h"
#include "main.h"
#include "DataSerial.h"
#include "Config.h"
#define FLASH 4

static bool flash_on = false;
static unsigned long t, t2, mms;
static AsyncWebServer server(SERVER_PORT);
static ESP32Camera camera = ESP32Camera(CAMERA_MODEL_AI_THINKER);
static StaticJsonDocument<1024> jsonDocument;
static DataStruct data_struct;

typedef enum {
  SUCCESS,
  ERROR,
  ALL,
} typeAdd;

static void addHeader(AsyncWebServerResponse *resp) {
  resp->addHeader("Access-Control-Allow-Origin", "*");
  resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
  resp->addHeader("Access-Control-Allow-Methods", "POST, GET");
  resp->addHeader("X-Content-Type-Options", "nosniff");
  resp->addHeader("Cache-Control", "no-cache");
}

static void addDataToResponse(AsyncResponseStream *resp, typeAdd s) {
  addHeader(resp);
  jsonDocument.clear();
  if (s == ALL) {
    jsonDocument["status"] = 0;
    jsonDocument["temp"] = data_struct.temp;
    jsonDocument["humidity"] = data_struct.humidity;
    jsonDocument["soil"] = data_struct.soil;
    jsonDocument["pump"] = data_struct.pump_on;
    jsonDocument["flash"] = flash_on;
    jsonDocument["soil_set"] = data_struct.limit;
    for (int i = 0; i < 5; ++i)
      jsonDocument["timer"][i] = data_struct.timer[i];
  } else
    jsonDocument["status"] = s;
  serializeJson(jsonDocument, *resp);
}

static bool waitForPumpOn(unsigned long timeout) {
  unsigned long ti = millis();
  while ((unsigned long)(millis() - ti) < timeout) {
    DataSerial.handle();
    if (data_struct.pump_on)
      return true;
  }
  return false;
}

static bool waitForPumpOff(unsigned long timeout) {
  unsigned long ti = millis();
  while ((unsigned long)(millis() - ti) < timeout) {
    DataSerial.handle();
    if (!data_struct.pump_on)
      return true;
  }
  return false;
}

static void setupRouting() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxlen, size_t index) {
      size_t len = min(maxlen, strlen(index_html) - index);
      memcpy(buffer, (char*)index_html + index, len);
      return len;
    });
    addHeader(response);
    request->send(response);
  });

  server.on("/api/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
    addDataToResponse(response, ALL);
    request->send(response);
  });

  server.on("/api/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    JpgCapture capture = camera.getJPGCapture();
    AsyncWebServerResponse *response = request->beginChunkedResponse("image/jpg", [capture](uint8_t *buffer, size_t maxlen, size_t index){
      size_t len = min(maxlen, capture.len - index);
      memcpy(buffer, capture.buf + index, len);
      return len;
    });
    addHeader(response);
    request->send(response);
  });

  server.on(
    "/api/set", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      jsonDocument.clear();
      AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
      DeserializationError error = deserializeJson(jsonDocument, (char *)data, len);
      if (error) {
        addDataToResponse(response, ERROR);
        request->send(response);
      }
      else {
        data_struct.limit = jsonDocument["soil"];
        int i;
        for (i = 0; i < 5; ++i)
          data_struct.timer[i] = jsonDocument["timer"][i];
        addDataToResponse(response, SUCCESS);
        request->send(response);
        DataSerial.sendControl();
      }
    });
  
  server.on(
    "/api/set_time", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      jsonDocument.clear();
      AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
      DeserializationError error = deserializeJson(jsonDocument, (char *)data, len);
      if (error) {
        addDataToResponse(response, ERROR);
        request->send(response);
      }
      else {
        DataSerial.sendTimeSet(
          jsonDocument["year"],
          jsonDocument["month"],
          jsonDocument["day"],
          jsonDocument["hour"],
          jsonDocument["minute"],
          jsonDocument["second"]
        );
        serializeJson(jsonDocument, Serial);
        addDataToResponse(response, SUCCESS);
        request->send(response);
      }
    }
  );

  server.on("/api/pump_on", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
    DataSerial.sendPumpOn();
    if (waitForPumpOn(3000))
      addDataToResponse(response, SUCCESS);
    else
      addDataToResponse(response, ERROR);
    request->send(response);
  });

  server.on("/api/pump_off", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
    DataSerial.sendPumpOff();
    if (waitForPumpOff(3000))
      addDataToResponse(response, SUCCESS);
    else
      addDataToResponse(response, ERROR);
    request->send(response);
  });

  server.on("/api/flash_on", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
    digitalWrite(FLASH, HIGH);
    flash_on = (bool)digitalRead(FLASH);
    if (flash_on)
      addDataToResponse(response, SUCCESS);
    else
      addDataToResponse(response, ERROR);
    request->send(response);
  });

  server.on("/api/flash_off", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json;charset=UTF-8");
    digitalWrite(FLASH, LOW);
    flash_on = (bool)digitalRead(FLASH);
    if (flash_on)
      addDataToResponse(response, ERROR);
    else
      addDataToResponse(response, SUCCESS);
    request->send(response);
  });
  server.begin();
}

static void onWifiConnected() {
  DataSerial.begin(&data_struct);
  setupRouting();
}

static void begin() {
  Serial.begin(115200);
  pinMode(FLASH, OUTPUT);
  digitalWrite(FLASH, LOW);
  camera.begin(FRAMESIZE_VGA);
  HandleWifi.onConnect(onWifiConnected);
  HandleWifi.begin();
  t = millis();
}

static void handle() {
  DataSerial.handle();
}

struct Main_t Main = {
  .begin = begin,
  .handle = handle
};
