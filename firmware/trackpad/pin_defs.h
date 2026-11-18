#ifndef _PIN_DEFS_H_
#define _PIN_DEFS_H_

#include <avr/io.h>

#define TRACKPAD_SDA PD1
#define TRACKPAD_SCL PD0
#define TRACKPAD_READY PE6
#define TRACKPAD_I2C_ADDR 0x74

// swapped volume buttons due to interface on Bazzite
#define VOL_PLUS PD6
#define VOL_MINUS PD4

#endif