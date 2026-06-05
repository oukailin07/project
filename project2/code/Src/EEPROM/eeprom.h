#ifndef __EEPROM_H
#define __EEPROM_H
#include "sys.h"

/* 24C02C 2Kbit EEPROM via software I2C
   SCL - PA8
   SDA - PA9
   Memory: 256 bytes (0x00 - 0xFF)
   Device address: 0xA0 (write), 0xA1 (read)
*/

/* EEPROM memory layout */
#define EE_ADDR_LANGUAGE           0x00    /* 1 byte: 0=Chinese, 1=English */
#define EE_ADDR_P_SET_H            0x01    /* p_set high byte (uint16 x10) */
#define EE_ADDR_P_SET_L            0x02    /* p_set low byte */
#define EE_ADDR_VERSION_MAJOR      0x03
#define EE_ADDR_VERSION_MINOR      0x04
#define EE_ADDR_MAX_VOL            0x05    /* max injection volume (mL x10) */
#define EE_ADDR_COMPRESS_TIME      0x06    /* compression time (minutes) */
#define EE_ADDR_P_RANGE_H          0x07    /* pressure sensor range high byte (kPa x10) */
#define EE_ADDR_P_RANGE_L          0x08    /* pressure sensor range low byte */
#define EE_ADDR_PRESSURE_HIST      0x10    /* 120 points x 2 bytes = 240 bytes */

void EEPROM_Init(void);
void EEPROM_WriteByte(u8 addr, u8 data);
u8   EEPROM_ReadByte(u8 addr);
void EEPROM_WriteHalfWord(u8 addr, u16 data);
u16  EEPROM_ReadHalfWord(u8 addr);

/* High-level config load/save */
void EEPROM_LoadDefaults(void);
void EEPROM_SaveConfig(void);
void EEPROM_LoadConfig(void);
void EEPROM_SavePressurePoint(u8 index, u16 pressure_x10);
u16  EEPROM_ReadPressurePoint(u8 index);

extern u8   g_ee_language;
extern u16  g_ee_p_set;            /* target pressure x10 (kPa) */
extern u16  g_ee_pressure_range;   /* sensor full-scale range x10 (kPa) */
extern u8   g_ee_version_major;
extern u8   g_ee_version_minor;
extern u8   g_ee_max_vol;          /* mL x10 */
extern u8   g_ee_compress_time;    /* minutes */

#endif
