#include "HandleWifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include <EEPROM.h>

#define LED         33
static WebServer server(80);
static StaticJsonDocument<MAX_LENGTH_SSID + MAX_LENGTH_PWD> jsonDocument;
static void (*callback)(void);

static void setSSID(const char n_ssid[MAX_LENGTH_SSID]) {
  int i;
  for (i = 0; i < MAX_LENGTH_SSID; ++i) {
    HandleWifi.ssid[i] = n_ssid[i];
    if (n_ssid[i] == '\0')
      break;
  }
}

static void setPassword(const char n_pwd[MAX_LENGTH_PWD]) {
  int i;
  for (i = 0; i < MAX_LENGTH_PWD; ++i) {
    HandleWifi.password[i] = n_pwd[i];
    if (n_pwd[i] == '\0')
      break;
  }
}

static void readEEPROM() {
  int i;
  int addr = SSID_ADDR;
  for (i = 0; i < MAX_LENGTH_SSID; ++i) {
    HandleWifi.ssid[i] = EEPROM.read(addr++);
    if (HandleWifi.ssid[i] > 127) {
      HandleWifi.ssid[i] = '\0';
      break;
    }
    if (HandleWifi.ssid[i] == '\0')
      break;
  }
  addr = PWD_ADDR;
  for (i = 0; i < MAX_LENGTH_PWD; ++i) {
    HandleWifi.password[i] = EEPROM.read(addr++);
    if (HandleWifi.password[i] > 127) {
      HandleWifi.password[i] = '\0';
      break;
    }
    if (HandleWifi.password[i] == '\0')
      break;
  }
}

static void writeEEPROM() {
  int i;
  int addr = SSID_ADDR;
  for (i = 0; i < MAX_LENGTH_SSID; ++i) {
    EEPROM.write(addr++, HandleWifi.ssid[i]);
    if (HandleWifi.ssid[i] == '\0')
      break;
  }
  addr = PWD_ADDR;
  for (i = 0; i < MAX_LENGTH_PWD; ++i) {
    EEPROM.write(addr++, HandleWifi.password[i]);
    if (HandleWifi.password[i] == '\0')
      break;
  }
  EEPROM.commit();
}

static String homePage() {
  String http = "<!DOCTYPE html><html> <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} button,select,input{margin: 5px 15px;}</style></head><body><h1>WiFi Setting</h1>";
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("no networks found");
    http += "<h2>No networks found</h2>";
  } else {
    http += "<label>SSID</label><select id=\"ssid\">";
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
      http += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    http += "</select><div><label>Password</label><input type=\"password\" id=\"password\"></div><button onclick=\"submit()\">Submit</button><script>function submit(){const ssid=document.querySelector(\"#ssid\").value;const password=document.querySelector(\"#password\").value;if(password.length<8||password.length>63){alert(\"Password must be between 8 - 63 characters\");return};const json={\"ssid\":ssid,\"pass\":password};var xhr=new XMLHttpRequest();xhr.open(\"POST\",\"/submit\");xhr.setRequestHeader(\"Content-Type\",\"application/json;charset=UTF-8\");xhr.send(JSON.stringify(json));xhr.responseType=\"text\";xhr.onload=()=>{if(xhr.readyState==4&&xhr.status==200){alert(xhr.response);}}</script>";
  }
  http += "</body></html>\r\n";
  return http;
}

static void setWiFisoftAP(){
  IPAddress softAP_ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAP("AutomationFarm");
  WiFi.softAPConfig(softAP_ip, gateway, subnet);
}

static void blinkLed(int _time, int _delay) {
  int i;
  for (i = 0; i < _time * 2; i++){
    digitalWrite(LED, !digitalRead(LED));
    delay(_delay);
  }
}

static void ledOn() {
  digitalWrite(LED, LOW);
}

static void ledOff() {
  digitalWrite(LED, HIGH);
}

static bool connectWifi(const char *ssid, const char *pass) {
  WiFi.begin(ssid, pass);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED){
    if ((unsigned long)(millis() - t) >= 10000) {
      return false;
    }
      blinkLed(1, 500);
  }
  Serial.print("WiFi connect success\nIP: ");
  Serial.print(WiFi.localIP());
  Serial.print(':');
  Serial.println(SERVER_PORT);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  if (callback)
    callback();
  ledOff();
  return true;
}

static bool isConnected(){
  return WiFi.status() == WL_CONNECTED;
}

static void startServer() {
  setWiFisoftAP();
  server.on("/", HTTP_GET, []() {
    Serial.println("GET /");
    server.send(200, "text/html", homePage());
  });
  server.on("/submit", HTTP_POST, []() {
    if (server.hasArg("plain") == false)
      return;
    deserializeJson(jsonDocument, server.arg("plain"));
    const char* ssid = jsonDocument["ssid"];
    const char* pass = jsonDocument["pass"];
    bool conn_success;
    if (*pass == '\0')
      conn_success = connectWifi(ssid, NULL);
    else if (strlen(pass) >= 8)
      conn_success = connectWifi(ssid, pass);
    else
      Serial.println("Password Invalid");
    if (!conn_success)
      server.send(404, "text/html", "Connect faild! Check your password");
    else {
      server.send(200, "text/html", "Connection successfully!\n" + String(WiFi.localIP()) + ":" + String(SERVER_PORT) + "\nMAC Address: " + String(WiFi.macAddress()));
      server.close();
      WiFi.eraseAP();
      setSSID(ssid);
      setPassword(pass);
      writeEEPROM();
    }
  });
  server.begin();
  Serial.println("Server started");
  blinkLed(5, 300);
  while (!isConnected()){
    server.handleClient();
    delay(100);
  }
}

static void tryConnectWifi() {
  /*
  WL_NO_SHIELD	      255
  WL_CONNECTED	      3
  WL_IDLE_STATUS	    0
  WL_NO_SSID_AVAIL	  1
  WL_SCAN_COMPLETED	  2
  WL_CONNECT_FAILED	  4
  WL_CONNECTION_LOST	5
  WL_DISCONNECTED	    6
  */
  if (HandleWifi.ssid[0] == '\0' || strlen(HandleWifi.password) < 8) {
    startServer();
  } else {
    bool success;
    if (HandleWifi.password[0] == '\0')
      success = connectWifi(HandleWifi.ssid, NULL);
    else
      success = connectWifi(HandleWifi.ssid, HandleWifi.password);
    if (!success)
      startServer();
  }
}

static void onConnect(void (*func)(void)) {
  callback = func;
}

static void clear() {
  setSSID("");
  setPassword("");
  writeEEPROM();
}

static void begin() {
  pinMode(LED, OUTPUT);
  ledOn();
  EEPROM.begin(EEPROM_SIZE);
  readEEPROM();
  WiFi.config(LOCAL_IP, GATEWAY, SUBNET, DNS);
  tryConnectWifi();
}

struct HandleWifi_t HandleWifi = {
  .begin = begin,
  .writeEEPROM = writeEEPROM,
  .readEEPROM = readEEPROM,
  .setSSID = setSSID,
  .setPassword = setPassword,
  .isConnected = isConnected,
  .onConnect = onConnect,
  .clear = clear
};