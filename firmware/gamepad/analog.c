#include "analog.h"
#include "pin_defs.h"
#include <avr/io.h>


static uint32_t adc_sums[6];
uint16_t adc_state[6];

uint32_t joy_l_center_x;
uint16_t joy_l_xu;
uint16_t joy_l_xl;
uint32_t joy_l_center_y;
uint16_t joy_l_yu;
uint16_t joy_l_yl;
uint32_t joy_r_center_x;
uint16_t joy_r_xu;
uint16_t joy_r_xl;
uint32_t joy_r_center_y;
uint16_t joy_r_yu;
uint16_t joy_r_yl;

uint16_t trig_l_u;
uint16_t trig_l_l;
uint16_t trig_r_u;
uint16_t trig_r_l;


/* --- Helper Functions --- */

uint16_t read_adc_channel(uint8_t chan) {
    ADMUX = (ADMUX & 0xE0) | (chan & 0x07);
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC));
    return ADC;
}

/**
 * Fast(?) inverse linear interpolation between deadzones of 
 * the axis, incorporating changing deadzones.
 */
uint16_t fast_axis_ilerp(
    uint16_t lim_lower,  //  10-bit
    uint16_t lim_upper,  // 10-bit
    uint16_t dz_lower,  // technically 7-bit because 0 - 64
    uint16_t dz_upper,  //   ^ ^ ^
    uint16_t adc  // 10-bit
) {
    // uint32_t range = (uint32_t)lim_upper - lim_lower;
    // uint32_t n1 = ((uint32_t)(adc - lim_lower) << 8) - (range * dz_lower);
    // uint32_t n2 = range * (dz_upper - dz_lower);
    // return (uint16_t)(((n1 >> 1) * 32767L) / (n2 >> 1));

    // uint16_t adc_clamped = adc > lim_upper ? lim_upper : (adc < lim_lower ? lim_lower : adc);
    // uint32_t scaled = (uint32_t)(adc_clamped - lim_lower) * 32767L;
    // return (uint16_t)(scaled / (lim_upper - lim_lower));

    uint16_t range = lim_upper - lim_lower;
    uint16_t dz_u_real = ((range * dz_upper) >> 6) + lim_lower;
    uint16_t dz_l_real = ((range * dz_lower) >> 6) + lim_lower;
    uint16_t adc_clamped = adc > dz_u_real ? dz_u_real : (adc < dz_l_real ? dz_l_real : adc);
    uint32_t scaled = (uint32_t)(adc_clamped - dz_l_real) * 32767L;
    return (uint16_t)(scaled / (dz_u_real - dz_l_real));
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
    joy_l_center_x = 0;
    joy_l_center_y = 0;
    joy_r_center_x = 0;
    joy_r_center_y = 0;
    for (uint8_t j = 0; j < 4; j++) {
        read_adc();
        joy_l_center_x += adc_state[0];
        joy_l_center_y += adc_state[1];
        joy_r_center_x += adc_state[2];
        joy_r_center_y += adc_state[3];
    }
    // shift left for more precision, then divide to keep average
    joy_l_center_x = (joy_l_center_x << 8) / 4;
    joy_l_center_y = (joy_l_center_y << 8) / 4;
    joy_r_center_x = (joy_r_center_x << 8) / 4;
    joy_r_center_y = (joy_r_center_y << 8) / 4;

    // setup limits
    joy_l_xu = (joy_l_center_x >> 8) + 10; joy_l_xl = (joy_l_center_x >> 8) - 10;
    joy_l_yu = (joy_l_center_y >> 8) + 10; joy_l_yl = (joy_l_center_y >> 8) - 10;
    joy_r_xu = (joy_r_center_x >> 8) + 10; joy_r_xl = (joy_r_center_x >> 8) - 10;
    joy_r_yu = (joy_r_center_y >> 8) + 10; joy_r_yl = (joy_r_center_y >> 8) - 10;
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
    if (adc_state[0] > joy_l_xu) joy_l_xu = adc_state[0];
    if (adc_state[0] < joy_l_xl) joy_l_xl = adc_state[0];

    if (adc_state[1] > joy_l_yu) joy_l_yu = adc_state[1];
    if (adc_state[1] < joy_l_yl) joy_l_yl = adc_state[1];

    if (adc_state[2] > joy_r_xu) joy_r_xu = adc_state[2];
    if (adc_state[2] < joy_r_xl) joy_r_xl = adc_state[2];
    
    if (adc_state[3] > joy_r_yu) joy_r_yu = adc_state[3];
    if (adc_state[3] < joy_r_yl) joy_r_yl = adc_state[3];

    if (adc_state[4] > trig_l_u) trig_l_u = adc_state[4];
    if (adc_state[4] < trig_l_l) trig_l_l = adc_state[4];
    
    if (adc_state[5] > trig_r_u) trig_r_u = adc_state[5];
    if (adc_state[5] < trig_r_l) trig_r_l = adc_state[5];
}

void update_joystick_centers(void) {
    int24_t joy_l_dx = (int24_t)adc_state[0] - (joy_l_center_x >> 8);
    int24_t joy_l_dy = (int24_t)adc_state[1] - (joy_l_center_y >> 8);
    if ((joy_l_dx*joy_l_dx + joy_l_dy*joy_l_dy) <= CENTER_CHECK_RADIUS_SQ) {
        // left joystick center exp. smoothing
        _smooth_step_joystick_center_axis(adc_state[0], &joy_l_center_x);
        _smooth_step_joystick_center_axis(adc_state[1], &joy_l_center_y);
    }

    int24_t joy_r_dx = (int24_t)adc_state[2] - (joy_r_center_x >> 8);
    int24_t joy_r_dy = (int24_t)adc_state[3] - (joy_r_center_y >> 8);
    if ((joy_r_dx*joy_r_dx + joy_r_dy*joy_r_dy) <= CENTER_CHECK_RADIUS_SQ) {
        // right joystick center exp. smoothing
        _smooth_step_joystick_center_axis(adc_state[2], &joy_r_center_x);
        _smooth_step_joystick_center_axis(adc_state[3], &joy_r_center_y);
    }
}

/** 
 * Obtain joystick mapping using current center and limits, 
 * while applying deadzones. Output: -32767 to 32767
 */
int16_t map_joystick_axis(
    uint16_t adc, 
    uint16_t upper, 
    uint16_t lower, 
    uint16_t center
) {
    // uint16_t dz_outer;
    // uint16_t dz_inner;
    if (adc > center) {
        // use upper limit and center
        // dz_outer = lerp(center, upper, DZ_JOY_OUTER);
        // dz_inner = lerp(center, upper, DZ_JOY_INNER);
        // return (int16_t)adc_to_trigger(adc, dz_inner, dz_outer);
        return (int16_t)fast_axis_ilerp(center, upper, DZ_JOY_INNER, DZ_JOY_OUTER, adc);
    } else if (adc < center) {
        // use center and lower limit
        // dz_outer = lerp(center, lower, DZ_JOY_OUTER);
        // dz_inner = lerp(center, lower, DZ_JOY_INNER);
        // return (int16_t)adc_to_trigger(adc, dz_outer, dz_inner) - 32767L;
        return (int16_t)fast_axis_ilerp(lower, center, 64 - DZ_JOY_OUTER, 64 - DZ_JOY_INNER, adc) - 32767L;
    }
    return 0;
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
    // uint16_t dz_outer = lerp(lower, upper, DZ_TRIG_OUTER);
    // uint16_t dz_inner = lerp(lower, upper, DZ_TRIG_INNER);
    // return adc_to_trigger(adc, dz_inner, dz_outer);
    return fast_axis_ilerp(lower, upper, DZ_TRIG_INNER, DZ_TRIG_OUTER, adc);
}


// int16_t adc_to_axis(uint16_t adc) { return (int16_t)(((int32_t)adc * 65535L / 1023L) - 32768L); }

// uint16_t adc_to_trigger(uint16_t adc, uint16_t min, uint16_t max) {
//     uint16_t adc_clamped = adc > max ? max : (adc < min ? min : adc);
//     uint32_t scaled = (uint32_t)(adc_clamped - min) * 32767L;
//     return (uint16_t)(scaled / (max - min));
// }