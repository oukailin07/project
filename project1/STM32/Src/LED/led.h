#ifndef __LED_H
#define __LED_H
#include "sys.h"

#define BUZZER_PIN  PBout(3)
#define ALARM_LED   PBout(4)

void Alarm_Init(void);
void Alarm_On(void);
void Alarm_Off(void);

#endif
