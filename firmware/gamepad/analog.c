#include "analog.h"
#include "pin_defs.h"
#include <avr/io.h>


static uint32_t adc_sums[6];
uint16_t adc_state[6];

Joystick_Axis_Config joy_lx;
Joystick_Axis_Config joy_ly;
Joystick_Axis_Config joy_rx;
Joystick_Axis_Config joy_ry;

uint16_t trig_l_u;
uint16_t trig_l_l;
uint16_t trig_r_u;
uint16_t trig_r_l;


/* --- Helper Functions --- */

/**
 * Reads value from a given ADC channel using registers.
 */
uint16_t read_adc_channel(uint8_t chan) {
    ADMUX = (ADMUX & 0xE0) | (chan & 0x07);
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC));
    return ADC;
}

/**
 * Fast-ish integer square root. Does not fit the entire 24-bit
 * range, but we don't need that anyway.
 */
uint16_t isqrt(uint24_t x) {
    uint24_t res = 0;
    uint24_t bit = 0x400000;

    while (bit > x) bit >>= 2;
    while (bit > 0) {
        if (x >= (res + bit)) {
            x -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }

    return (uint16_t)res;
}

/**
 * Remaps an ADC value to an int16_t based on the limits and 
 * center provided in an axis configuration struct.
 * Output: -1024 to 1024
 */
int16_t remap_axis(
    uint16_t adc,
    Joystick_Axis_Config* conf
) {
    uint16_t adc_clamped;
    uint24_t scaled;
    if (adc > conf->center) {
        adc_clamped = adc > conf->upper ? conf->upper : adc;
        scaled = (uint24_t)(adc_clamped - conf->center) << 10;
        return (int16_t)(scaled / (conf->upper - conf->center));
    } else if (adc < conf->center) {
        adc_clamped = adc < conf->lower ? conf->lower : adc;
        scaled = (uint24_t)(adc_clamped - conf->lower) << 10;
        return (int16_t)(scaled / (conf->center - conf->lower)) - 1024;
    }
    return 0;
}

/** Exponential smoothing step for a single axis center */
void _smooth_step_joystick_center_axis(uint16_t target, uint32_t *current) {
    int32_t diff = ((int32_t)target << 8) - (int32_t)(*current);
    *current += (CENTER_ALPHA * diff) >> 8;
}


/* --- Main Functions --- */

void setup_adc_hw(void) {
    MCUCR |= _BV(JTD);
    MCUCR |= _BV(JTD); // gotta do this twice
    DDRF &= ~(_BV(JOY_R_Y) | _BV(JOY_R_X) | _BV(JOY_L_Y) | _BV(JOY_L_X) | _BV(TRIGGER_L) | _BV(TRIGGER_R));
    DIDR0 |= (_BV(ADC0D) | _BV(ADC1D) | _BV(ADC4D) | _BV(ADC5D) | _BV(ADC6D) | _BV(ADC7D));
    ADMUX = _BV(REFS0);
    ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
}

void init_joysticks(void) {
    // take initial readings for joystick centers and trigger zeroes
    joy_lx.center_full = 0;
    joy_ly.center_full = 0;
    joy_rx.center_full = 0;
    joy_ry.center_full = 0;
    for (uint8_t j = 0; j < 4; j++) {
        read_adc();
        joy_lx.center_full += adc_state[0];
        joy_ly.center_full += adc_state[1];
        joy_rx.center_full += adc_state[2];
        joy_ry.center_full += adc_state[3];
    }
    // shift left for more precision, then divide to keep average
    joy_lx.center_full = (joy_lx.center_full << 8) / 4;
    joy_ly.center_full = (joy_ly.center_full << 8) / 4;
    joy_rx.center_full = (joy_rx.center_full << 8) / 4;
    joy_ry.center_full = (joy_ry.center_full << 8) / 4;

    // back shift for the 10-bit centers
    joy_lx.center = joy_lx.center_full >> 8;
    joy_ly.center = joy_ly.center_full >> 8;
    joy_rx.center = joy_rx.center_full >> 8;
    joy_ry.center = joy_ry.center_full >> 8;

    // setup limits
    joy_lx.upper = joy_lx.center + 10; joy_lx.lower = joy_lx.center - 10;
    joy_ly.upper = joy_ly.center + 10; joy_ly.lower = joy_ly.center - 10;
    joy_rx.upper = joy_rx.center + 10; joy_rx.lower = joy_rx.center - 10;
    joy_ry.upper = joy_ry.center + 10; joy_ry.lower = joy_ry.center - 10;
    trig_l_l = adc_state[4] - 1; trig_l_u = trig_l_l + 2;
    trig_r_l = adc_state[5] - 1; trig_r_u = trig_r_l + 2;
}

void read_adc(void) {
    for (uint8_t chan = 0; chan < 6; chan++) {
        adc_sums[chan] = 0;
    }

    for (uint8_t j = 0; j < ADC_SAMPLE_SIZE; j++) {
        adc_sums[0] += read_adc_channel(7); // LX
        adc_sums[1] += read_adc_channel(6); // LY
        adc_sums[2] += read_adc_channel(4); // RX
        adc_sums[3] += read_adc_channel(1); // RY
        adc_sums[4] += read_adc_channel(5); // LT
        adc_sums[5] += read_adc_channel(0); // RT
    }

    for (uint8_t chan = 0; chan < 6; chan++) {
        adc_state[chan] = (uint16_t)(adc_sums[chan] / ADC_SAMPLE_SIZE);
    }
}

void update_analog_limits(void) {
    if (adc_state[0] > joy_lx.upper) joy_lx.upper = adc_state[0];
    if (adc_state[0] < joy_lx.lower) joy_lx.lower = adc_state[0];

    if (adc_state[1] > joy_ly.upper) joy_ly.upper = adc_state[1];
    if (adc_state[1] < joy_ly.lower) joy_ly.lower = adc_state[1];

    if (adc_state[2] > joy_rx.upper) joy_rx.upper = adc_state[2];
    if (adc_state[2] < joy_rx.lower) joy_rx.lower = adc_state[2];
    
    if (adc_state[3] > joy_ry.upper) joy_ry.upper = adc_state[3];
    if (adc_state[3] < joy_ry.lower) joy_ry.lower = adc_state[3];

    if (adc_state[4] > trig_l_u) trig_l_u = adc_state[4];
    if (adc_state[4] < trig_l_l) trig_l_l = adc_state[4];
    
    if (adc_state[5] > trig_r_u) trig_r_u = adc_state[5];
    if (adc_state[5] < trig_r_l) trig_r_l = adc_state[5];
}

void update_joystick_centers(void) {
    int24_t joy_l_dx = (int24_t)adc_state[0] - joy_lx.center;
    int24_t joy_l_dy = (int24_t)adc_state[1] - joy_ly.center;
    if ((joy_l_dx*joy_l_dx + joy_l_dy*joy_l_dy) <= CENTER_CHECK_RADIUS_SQ) {
        // left joystick center exp. smoothing
        _smooth_step_joystick_center_axis(adc_state[0], &(joy_lx.center_full));
        _smooth_step_joystick_center_axis(adc_state[1], &(joy_ly.center_full));
        joy_lx.center = joy_lx.center_full >> 8;
        joy_ly.center = joy_ly.center_full >> 8;
    }

    int24_t joy_r_dx = (int24_t)adc_state[2] - joy_rx.center;
    int24_t joy_r_dy = (int24_t)adc_state[3] - joy_ry.center;
    if ((joy_r_dx*joy_r_dx + joy_r_dy*joy_r_dy) <= CENTER_CHECK_RADIUS_SQ) {
        // right joystick center exp. smoothing
        _smooth_step_joystick_center_axis(adc_state[2], &(joy_rx.center_full));
        _smooth_step_joystick_center_axis(adc_state[3], &(joy_ry.center_full));
        joy_rx.center = joy_rx.center_full >> 8;
        joy_ry.center = joy_ry.center_full >> 8;
    }
}

/** 
 * Obtain joystick mapping using current center and limits, 
 * while applying deadzones. Output: -32767 to 32767 (per axis)
 */
void map_joystick_axes(
    uint16_t x_adc, 
    uint16_t y_adc, 
    Joystick_Axis_Config* x_conf, 
    Joystick_Axis_Config* y_conf, 
    int16_t* x_out, 
    int16_t* y_out
) {
    // remap axes to [-1024, 1024]
    int16_t px = remap_axis(x_adc, x_conf);
    int16_t py = remap_axis(y_adc, y_conf);

    // calculate distance and inverse lerp with deadzones to range [0, 32767]
    uint16_t dist_p = isqrt((uint24_t)(px)*(px) + (uint24_t)(py)*(py));
    uint16_t dist_q = dist_p > DZ_JOY_OUTER ? DZ_JOY_OUTER : (dist_p < DZ_JOY_INNER ? DZ_JOY_INNER : dist_p);
    uint32_t scaled = (uint32_t)(dist_q - DZ_JOY_INNER) * 32767;
    dist_q = (uint16_t)(scaled / (DZ_JOY_OUTER - DZ_JOY_INNER));

    // re-apply dist_q to direction of px, py
    *x_out = (int16_t)(((int32_t)px * dist_q) / dist_p);
    *y_out = (int16_t)(((int32_t)py * dist_q) / dist_p);
}

/** 
 * Obtain trigger mapping using outer limits, whileresizing 
 * applying deadzones. Output: 0 to 32767
 */
uint16_t map_trigger_axis(
    uint16_t adc, 
    uint16_t upper, 
    uint16_t lower
) {
    uint16_t range = upper - lower;
    uint16_t dz_u = ((range * DZ_TRIG_OUTER) >> 6) + lower;
    uint16_t dz_l = ((range * DZ_TRIG_INNER) >> 6) + lower;
    uint16_t adc_clamped = adc > dz_u ? dz_u : (adc < dz_l ? dz_l : adc);
    uint32_t scaled = (uint32_t)(adc_clamped - dz_l) * 32767;
    return (uint16_t)(scaled / (dz_u - dz_l));
}


// int16_t adc_to_axis(uint16_t adc) { return (int16_t)(((int32_t)adc * 65535L / 1023L) - 32768L); }

// uint16_t adc_to_trigger(uint16_t adc, uint16_t min, uint16_t max) {
//     uint16_t adc_clamped = adc > max ? max : (adc < min ? min : adc);
//     uint32_t scaled = (uint32_t)(adc_clamped - min) * 32767L;
//     return (uint16_t)(scaled / (max - min));
// }