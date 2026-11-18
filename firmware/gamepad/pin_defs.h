#ifndef _PIN_DEFS_H_
#define _PIN_DEFS_H_

#include <avr/io.h>

#define BUTTON_X PB1 //X
#define BUTTON_Y PB0 //Y
#define BUTTON_A PD2 //A
#define BUTTON_B PD3 //B

#define BUTTON_L1 PB5 //L1
#define BUTTON_R1 PB3 //R1

#define JOY_L_X PF7 //LX
#define JOY_L_Y PF6 //LY
#define JOY_L_PRESS PB4 //L3

#define JOY_R_X PF4 //RX
#define JOY_R_Y PF1 //RY
#define JOY_R_PRESS PB2 //R3

#define DPAD_U PB6 //UP
#define DPAD_D PC6 //DOWN
#define DPAD_L PC7 //LEFT
#define DPAD_R PB7 //RIGHT

#define BUTTON_MENU PE6 //Menu
#define BUTTON_START PD4 //Start
#define BUTTON_SELECT PD5 //Select

#define TRIGGER_L PF5 //LZ
#define TRIGGER_R PF0 //RZ

#define RUMBLE_L PD7 //Rumble
#define RUMBLE_R PD6 //Rumble_RHS

#endif