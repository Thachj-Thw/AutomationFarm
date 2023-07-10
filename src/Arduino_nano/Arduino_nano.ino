#include <Wire.h>
#include <DHT.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define SOIL_PIN  A0
#define DHT_PIN   A2
#define RS_PIN    2
#define RX        3
#define TX        4
#define PUMP      5


typedef struct {
  float limit;
  int timer[5];
} DataBuffer_t;

DataBuffer_t dataBuffer = {
  100,
  { -1, -1, -1, -1, -1 }
};

RTC_DS1307 rtc;
int on = -1;
unsigned long t;
StaticJsonDocument<225> doc;
DHT dht(DHT_PIN, DHT11);
SoftwareSerial dataSerial = SoftwareSerial(RX, TX);
bool is_ready = false;
int lastTime;
unsigned int count = 0;
enum CMD {
  DATA,
  SET_TIME,
  CONTROL,
  PUMP_ON,
  PUMP_OFF,
  RESET,
  READY
};

void setup() {
  Serial.begin(115200);
  dataSerial.begin(9600);
  while (!dataSerial);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (!rtc.isrunning())
    Serial.println("RTC is not running!");
  dht.begin();
  pinMode(PUMP, OUTPUT);
  pinMode(RS_PIN, INPUT);
  digitalWrite(PUMP, LOW);
  readDataFromEEPROM();
  t = millis();
  sendReady();
  waitESPReady();
}

int currentTime() {
  DateTime now = rtc.now();
  return now.hour() * 60 + now.minute();
}

void loop() {
  if ((unsigned long)(millis() - t) > 1000) {
    t = millis();
    pumpEvent();
    if (++count >= 3) {
      sendData();
      count = 0;
    }
  }
  handleEvent();
  handleInputEvent();
}

void handleInputEvent() {
  if (digitalRead(RS_PIN) == LOW) {
    delay(20);
    unsigned long c = millis();
    while ((unsigned long)(millis() - c) < 3000)
      if (digitalRead(RS_PIN) == HIGH)
        return;
    sendReset();
    dataBuffer.limit = 100.0f;
    int i;
    for (i = 0; i < 5; ++i)
      dataBuffer.timer[i] = -1;
    writeDataToEEPROM();
  }
}

void sendData() {
  doc.clear();
  doc["cmd"] = DATA;
  doc["humi"] = dht.readHumidity();
  doc["temp"] = dht.readTemperature();
  doc["soil"] = getSoilValue();
  doc["pump"] = (bool)digitalRead(PUMP);
  serializeJson(doc, dataSerial);
  // serializeJson(doc, Serial);
  // Serial.println();
}

void sendReset() {
  doc.clear();
  doc["cmd"] = RESET;
  serializeJson(doc, dataSerial);
}

void sendReady() {
  doc.clear();
  doc["cmd"] = READY;
  doc["limit"] = dataBuffer.limit;
  int i;
  for (i = 0; i < 5; i++)
    doc["timer"][i] = dataBuffer.timer[i];
  serializeJson(doc, dataSerial);
}

void pumpEvent() {
  int c = currentTime();
  if (on >= 0 && c >= dataBuffer.timer[on]) {
    if (getSoilValue() < dataBuffer.limit)
      digitalWrite(PUMP, HIGH);
    nextValueOn();
  }
  if (getSoilValue() > dataBuffer.limit)
    digitalWrite(PUMP, LOW);
  if (c < lastTime) {
    getValueOn();
  }
  lastTime = c;
}

void handleEvent() {
  if (!isGotData()) return;
  enum CMD cmd = doc["cmd"];
  switch (cmd) {
    case SET_TIME:
      rtc.adjust(DateTime(doc["year"], doc["month"], doc["day"], doc["hour"], doc["minute"], doc["second"]));
      getValueOn();
      break;
    case CONTROL:
      int i;
      for (i = 0; i < 5; i++)
        dataBuffer.timer[i] = doc["timer"][i];
      printArray(dataBuffer.timer, 5);
      selectionSortTimer();
      getValueOn();
      dataBuffer.limit = doc["limit"];
      writeDataToEEPROM();
      break;
    case PUMP_ON:
      digitalWrite(PUMP, HIGH);
      sendData();
      break;
    case PUMP_OFF:
      digitalWrite(PUMP, LOW);
      sendData();
      break;
    case READY:
      if(doc["resp"])
        sendReady();
      is_ready = true;
      break;
  }
}

void waitESPReady() {
  Serial.println("Start wait ESP ready");
  while (!is_ready)
    handleEvent();
  Serial.println("End");
}

bool isGotData() {
  if (dataSerial.available()) {
    doc.clear();
    DeserializationError error = deserializeJson(doc, dataSerial);
    if (error) {
      Serial.print("ERROR: ");
      Serial.println(error.c_str());
      return false;
    }
    return true;
  }
  return false;
}

void selectionSortTimer() {
  int i, j, m;
  for (i = 0; i < 4; i++) {
    m = i;
    for (j = i + 1; j < 5; j++)
      if (dataBuffer.timer[j] < dataBuffer.timer[m])
        m = j;
    swap(&dataBuffer.timer[m], &dataBuffer.timer[i]);
  }
}

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

void getValueOn() {
  on = 0;
  int c = currentTime();
  while (dataBuffer.timer[on] < c) {
    nextValueOn();
    if (on < 0)
      break;
  }
}

void nextValueOn() {
  int i;
  for (i = on; i < 5; i++) {
    if (dataBuffer.timer[++on] >= 0)
      break;
  }
  if (on >= 5 || dataBuffer.timer[on] < 0)
    on = -1;
}

float persent(int a, int _0ps, int _100ps, bool limit) {
  float x = (a - _0ps) * 100.0 / (_100ps - _0ps);
  if (limit) {
    if (x <= 0) return 0.0;
    if (x >= 100) return 100.0;
  }
  return x;
}

float getSoilValue() {
  return persent(analogRead(SOIL_PIN), 535, 247, true);
}

void readDataFromEEPROM() {
  EEPROM.get(0, dataBuffer);
}

void writeDataToEEPROM() {
  EEPROM.put(0, dataBuffer);
}

void printArray(int arr[], int len) {
  Serial.print("{");
  int i;
  for (i = 0; i < len - 1; i++) {
    Serial.print(arr[i]);
    Serial.print(", ");
  }
  Serial.print(arr[len - 1]);
  Serial.println("}");
}