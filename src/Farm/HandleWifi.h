#ifndef _HANDLEWIFI_H_
#define _HANDLEWIFI_H_

#define EEPROM_SIZE       97
#define SSID_ADDR         0
#define PWD_ADDR          33
#define MAX_LENGTH_SSID   33
#define MAX_LENGTH_PWD    64

struct HandleWifi_t {

  char ssid[MAX_LENGTH_SSID];

  char password[MAX_LENGTH_PWD];

  void (*begin)(void);

  void (*writeEEPROM)(void);

  void (*readEEPROM)(void);

  void (*setSSID)(const char[MAX_LENGTH_SSID]);

  void (*setPassword)(const char[MAX_LENGTH_PWD]);

  bool (*isConnected)(void);

  void (*onConnect)(void (*)(void));

  void (*clear)(void);
};

extern struct HandleWifi_t HandleWifi;

#endif // _HANDLEWIFI_H_