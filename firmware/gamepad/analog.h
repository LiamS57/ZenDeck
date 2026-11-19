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


typedef struct {
    uint32_t center_full;
    uint16_t center;
    uint16_t upper;
    uint16_t lower;
} Joystick_Axis_Config;


extern uint16_t adc_state[6];

// joystick centers use exp. smoothing, so using bigger integers
// to keep fractional parts

extern Joystick_Axis_Config joy_lx;  // left joystick x-axis
extern Joystick_Axis_Config joy_ly;  // left joystick y-axis
extern Joystick_Axis_Config joy_rx;  // right joystick x-axis
extern Joystick_Axis_Config joy_ry;  // right joystick y-axis

extern uint16_t trig_l_u;  // left trigger upper limit
extern uint16_t trig_l_l;  // left trigger lower limit
extern uint16_t trig_r_u;  // right trigger upper limit
extern uint16_t trig_r_l;  // right trigger lower limit

extern void setup_adc_hw(void);
extern void init_joysticks(void);
extern void read_adc(void);
extern void update_analog_limits(void);
extern void update_joystick_centers(void);
extern int16_t map_joystick_axis(uint16_t adc, Joystick_Axis_Config* conf);
extern uint16_t map_trigger_axis(uint16_t adc, uint16_t upper, uint16_t lower);


#endif