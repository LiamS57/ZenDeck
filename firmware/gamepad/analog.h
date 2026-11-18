#ifndef _ANALOG_H_
#define _ANALOG_H_

#include <avr/io.h>


#define ADC_SAMPLE_SIZE 5

#define DZ_JOY_OUTER 57  // 90% of 64
#define DZ_JOY_INNER 10  // 15% of 64
#define DZ_TRIG_OUTER 44  // 70% of 64
#define DZ_TRIG_INNER 14  // 20% of 64

#define CENTER_ALPHA 3  // out of 256
#define CENTER_CHECK_RADIUS_SQ 625L  // 25^2


extern uint16_t adc_state[6];

// joystick centers use exp. smoothing, so using bigger integers
// to keep fractional parts

extern uint32_t joy_l_center_x;  // left joystick x-axis center
extern uint16_t joy_l_xu;  // left joystick x-axis upper limit
extern uint16_t joy_l_xl;  // left joystick x-axis lower limit
extern uint32_t joy_l_center_y;  // left joystick y-axis center
extern uint16_t joy_l_yu;  // left joystick y-axis upper limit
extern uint16_t joy_l_yl;  // left joystick y-axis lower limit
extern uint32_t joy_r_center_x;  // right joystick x-axis center
extern uint16_t joy_r_xu;  // right joystick x-axis upper limit
extern uint16_t joy_r_xl;  // right joystick x-axis lower limit
extern uint32_t joy_r_center_y;  // right joystick y-axis center
extern uint16_t joy_r_yu;  // right joystick y-axis upper limit
extern uint16_t joy_r_yl;  // right joystick y-axis lower limit

extern uint16_t trig_l_u;  // left trigger upper limit
extern uint16_t trig_l_l;  // left trigger lower limit
extern uint16_t trig_r_u;  // right trigger upper limit
extern uint16_t trig_r_l;  // right trigger lower limit

extern void setup_adc_hw(void);
extern void init_joysticks(void);
extern void read_adc(void);
extern void update_analog_limits(void);
extern void update_joystick_centers(void);
extern int16_t map_joystick_axis(uint16_t adc, uint16_t upper, uint16_t lower, uint16_t center);
extern uint16_t map_trigger_axis(uint16_t adc, uint16_t upper, uint16_t lower);


#endif