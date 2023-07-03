#ifndef _MAIN_H_
#define _MAIN_H_

#include <ESPAsyncWebServer.h>
#include <Arduino.h>

struct Main_t{

  void (*begin)(void);

  void (*handle)(void);
};

extern struct Main_t Main;

#endif // _MAIN_H_