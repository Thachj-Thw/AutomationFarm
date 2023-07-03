#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <WiFi.h>

#define          SERVER_PORT   1152
static IPAddress LOCAL_IP      (192, 168, 0, 201);
static IPAddress GATEWAY       (192, 168, 0, 1);
static IPAddress SUBNET        (255, 255, 255, 0);
static IPAddress DNS           (192, 168, 0, 1);

#define          TX            15
#define          RX            13

#endif // _CONFIG_H_