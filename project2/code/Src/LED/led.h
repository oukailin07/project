#ifndef __LED_H
#define __LED_H
#include "sys.h"

/* Buzzer on PB3, Alarm LED on PB4 */
#define BUZZER_PIN  PBout(3)
#define ALARM_LED   PBout(4)

void Alarm_Init(void);
void Alarm_On(void);
void Alarm_Off(void);
void Buzzer_Beep(u8 times, u16 duration_ms);

#endif
