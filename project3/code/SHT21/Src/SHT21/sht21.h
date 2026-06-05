#ifndef __SHT21_H
#define __SHT21_H

#include "sys.h"

void SHT21_Init(void);
float SHT21_ReadTemperature(void);
float SHT21_ReadHumidity(void);

#endif
