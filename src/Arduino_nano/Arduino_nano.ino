#include <DHT.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define SOIL_PIN A0
#define DHT_PIN A2
#define RS_PIN 2
#define RX 3
#define TX 4
#define PUMP 5

typedef struct {
  float limit;
  int timer[5];
} DataBuffer_t;

DataBuffer_t dataBuffer = {
  100,
  { -1, -1, -1, -1, -1 }
};
int on = -1;
unsigned long milliseconds, delta, t;
StaticJsonDocument<225> doc;
DHT dht(DHT_PIN, DHT11);
SoftwareSerial dataSerial = SoftwareSerial(RX, TX);
bool is_ready = false;
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
  dht.begin();
  pinMode(PUMP, OUTPUT);
  pinMode(RS_PIN, INPUT);
  digitalWrite(PUMP, LOW);
  readDataFromEEPROM();
  delta = 0;
  t = millis();
  waitESPReady();
}

void loop() {
  updateTime();
  sendDataEvery(3000);
  pumpEvent();
  handleEvent();
  handleInputEvent();
  delay(200);
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

void updateTime() {
  milliseconds += (unsigned long)(millis() - delta);
  delta = millis();
  if (milliseconds >= 86400000) {
    milliseconds -= 86400000;
    getValueOn();
  }
}

void sendDataEvery(int mil) {
  if ((unsigned long)(millis() - t) > mil) {
    t = millis();
    sendData();
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
}

void sendReset() {
  Serial.println("RESET");
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
  if (on >= 0 && (int)(milliseconds / 60000) >= dataBuffer.timer[on]) {
    digitalWrite(PUMP, HIGH);
    nextValueOn();
  }
  if (getSoilValue() > dataBuffer.limit)
    digitalWrite(PUMP, LOW);
}

void handleEvent() {
  if (!isGotData()) return;
  enum CMD cmd = doc["cmd"];
  switch (cmd) {
    case SET_TIME:
      milliseconds = doc["value"];
      getValueOn();
      break;
    case CONTROL:
      int i;
      for (i = 0; i < 5; i++)
        dataBuffer.timer[i] = doc["timer"][i];
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
      milliseconds = doc["time"];
      getValueOn();
      sendReady();
      Serial.print("READY\t");
      Serial.println(milliseconds);
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
  while (dataBuffer.timer[on] < (int)(milliseconds / 60000)) {
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