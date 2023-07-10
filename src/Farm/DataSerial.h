#ifndef _DATASERIAL_H_
#define _DATASERIAL_H_

#include <Arduino.h>

typedef struct {
  float temp;
  float humidity;
  float soil;
  bool pump_on;
  float limit;
  int timer[5];
} DataStruct;

struct DataSerial_t {

  void (*begin)(DataStruct*);

  void (*handle)(void);

  void (*sendControl)(void);

  void (*sendPumpOn)(void);

  void (*sendPumpOff)(void);

  bool (*isGotDataSerial)(void);

  void (*sendTimeSet)(int, int, int, int, int, int);
};

extern struct DataSerial_t DataSerial;

#endif // _DATASERIAL_H_