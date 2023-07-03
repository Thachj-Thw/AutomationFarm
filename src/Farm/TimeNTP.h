#ifndef _TIMENTP_H_
#define _TIMENTP_H_

#include <Arduino.h>

struct TimeNTP_t {

  void (*begin)(void);

  unsigned long (*getCurrentMillis)(void);
};

extern struct TimeNTP_t TimeNTP;

#endif // _TIMENTP_H_