#include "TimeNTP.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP);

static void begin() {
  timeClient.begin();
  timeClient.setTimeOffset(+7 * 60 * 60);
}

unsigned long getCurrentMillis() {
  while (!timeClient.update())
    timeClient.forceUpdate();
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();
  return h * 3600000 + m * 60000 + s * 1000;
}

struct TimeNTP_t TimeNTP = {
  .begin = begin,
  .getCurrentMillis = getCurrentMillis
};
