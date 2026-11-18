#include "IQS5xx.h"
#include <avr/io.h>


void _i2c_reset(void);
bool _i2c_start(void);
bool _i2c_stop(void);
bool _i2c_write(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes);
bool _i2c_read(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes);
bool _i2c_send_addr(uint8_t addr);
bool _i2c_send_data(uint8_t data);
bool _i2c_recv_data(uint8_t ack);
bool _i2c_write_h(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes);
bool _i2c_read_h(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes);


void IQS5xx_AcknowledgeReset(IQS5xx *iqs) {
    static uint8_t sc0 = ACK_RESET;
    _i2c_write(iqs, SystemControl0_adr, &sc0, 1);
}

void IQS5xx_ProcessXY(IQS5xx *iqs, IQS5xxOutput *out) {
    // prep
    out->valid = false;
    out->click_l = false;
    out->click_r = false;

    // read
    // uint8_t tmp[30];
    uint8_t end_window_dbuf[1];
    _i2c_read(iqs, GestureEvents0_adr, &(iqs->dbuf[0]), 44);
    // if ((iqs->dbuf[3] & SNAP_TOGGLE) != 0) {
    //     _i2c_read(iqs, SnapStatus_adr, &tmp[0], 30);
    // }
    _i2c_write(iqs, 0xEEEE, &end_window_dbuf[0], 1);

    // process
    uint8_t sys_flags[2];
    sys_flags[0] = iqs->dbuf[2];
    sys_flags[1] = iqs->dbuf[3];
    uint8_t num_fingers = iqs->dbuf[4];
    // uint16_t abs_x[6], abs_y[6], touch_strength[6];
    // uint8_t area[6];

    if ((sys_flags[0] & SHOW_RESET) != 0) {
        IQS5xx_AcknowledgeReset(iqs);
        return;
    }
    if ((sys_flags[1] & SNAP_TOGGLE) != 0) {
        return;
    }
    if ((iqs->dbuf[0]) == SINGLE_TAP) {
        if (iqs->enabled) {
            // mouse left click
            out->click_l = true;
        }
    }
    if ((iqs->dbuf[0]) == TAP_AND_HOLD) {
        if (iqs->enabled) {
            // mouse right click
            out->click_r = true;
        }
    } else if ((iqs->dbuf[1]) == TWO_FINGER_TAP) {
        if ((iqs->tap_timer) != 0) {
            iqs->enabled = !(iqs->enabled); // toggle enable
        } else {
            iqs->tap_timer = 1;
        }
    }

    if (num_fingers != 0) {
        out->rel_x = (((iqs->dbuf[5]) << 8) | (iqs->dbuf[6]));
        out->rel_y = (((iqs->dbuf[7]) << 8) | (iqs->dbuf[8]));

        if (iqs->enabled) {
            if ((iqs->dbuf[1]) == SCROLL) {
                // Mouse.move(0, 0, (-i16RelY[1] / 4));
                out->scroll = true;
            } else {
                // Mouse.move(i16RelY[1], -i16RelX[1], 0);
                out->scroll = false;
            }
        }
        out->move = iqs->enabled;

        // for (uint8_t j = 0; j < 5; j++) {
        //     abs_x[j+1] = ((iqs->dbuf[(7*j)+9] << 8) | (iqs->dbuf[(7*j)+10]));
		// 	abs_y[j+1] = ((iqs->dbuf[(7*j)+11] << 8) | (iqs->dbuf[(7*j)+12]));
		// 	touch_strength[j+1] = ((iqs->dbuf[(7*j)+13] << 8) | (iqs->dbuf[(7*j)+14]));
		// 	area[j+1] = (iqs->dbuf[7*j+15]);
        // }
    }

    out->valid = true;
}


bool _i2c_write(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes) {
    bool success;
    for (uint8_t j = 0; j < 4; j++) {
        success = _i2c_write_h(iqs, reg_addr, data, num_bytes);
        if (success) return true;
    }
    return false;
}

bool _i2c_read(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes) {
    bool success;
    for (uint8_t j = 0; j < 4; j++) {
        success = _i2c_read_h(iqs, reg_addr, data, num_bytes);
        if (success) return true;
    }
    return false;
}

void _i2c_reset(void) {
    TWCR = 0;
    TWCR = (_BV(TWEN) | _BV(TWEA));
}

bool _i2c_start(void) {
    TWCR = (_BV(TWINT) | _BV(TWSTA) | _BV(TWEN));
    timeout_counter = 0;
    while (!(TWCR & _BV(TWINT))) {
        if (HAS_TIMED_OUT) {
            _i2c_reset();
            return false;
        }
        timeout_counter++;
    }
    if ((TWSR & 0xF8) == 0x08) return true; // start
    if ((TWSR & 0xF8) == 0x10) return true; // repeated start
    if ((TWSR & 0xF8) == 0x38) _i2c_reset(); // lost arbitration
    return false;
}

bool _i2c_stop(void) {
    TWCR = (_BV(TWINT) | _BV(TWSTO) | _BV(TWEN));
    timeout_counter = 0;
    while (TWCR & _BV(TWSTO)) {
        if (HAS_TIMED_OUT) {
            _i2c_reset();
            return false;
        }
        timeout_counter++;
    }
    return true;
}

bool _i2c_send_addr(uint8_t addr) {
    TWDR = addr;
    TWCR = (_BV(TWINT) | _BV(TWEN));
    timeout_counter = 0;
    while (!(TWCR & _BV(TWINT))) {
        if (HAS_TIMED_OUT) {
            _i2c_reset();
            return false;
        }
        timeout_counter++;
    }

    if (((TWSR & 0xF8) == 0x18) || ((TWSR & 0xF8) == 0x40)) {
        return true;
    }
    if (((TWSR & 0xF8) == 0x20) || ((TWSR & 0xF8) == 0x48)) {
        _i2c_stop();
    } else {
        _i2c_reset();
    }
    return false;
}

bool _i2c_send_data(uint8_t data) {
    TWDR = data;
    TWCR = (_BV(TWINT) | _BV(TWEN));
    timeout_counter = 0;
    while (!(TWCR & _BV(TWINT))) {
        if (HAS_TIMED_OUT) {
            _i2c_reset();
            return false;
        }
        timeout_counter++;
    }

    if ((TWSR & 0xF8) == 0x28) {
        return true;
    }
    if ((TWSR & 0xF8) == 0x30) {
        _i2c_stop();
    } else {
        _i2c_reset();
    }
    return false;
}

bool _i2c_recv_data(uint8_t ack) {
    TWCR = (_BV(TWINT) | _BV(TWEN) | (ack ? _BV(TWEA) : 0));
    timeout_counter = 0;
    while (!(TWCR & _BV(TWINT))) {
        if (HAS_TIMED_OUT) {
            _i2c_reset();
            return false;
        }
        timeout_counter++;
    }

    if ((TWSR & 0xF8) == 0x38) { // lost arbitration
        _i2c_reset();
        return false;
    }

    if ((((TWSR & 0xF8) == 0x58) && (!ack)) || (((TWSR & 0xF8) == 0x50) && (ack))) {
        return true;
    } else {
        return false;
    }
}

bool _i2c_write_h(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes) {
    if (!_i2c_start()) return false;
    if (!_i2c_send_addr((iqs->i2c_address) << 1)) return false;
    if (!_i2c_send_data( (uint8_t)(reg_addr >> 8) )) return false;
    if (!_i2c_send_data( (uint8_t)reg_addr )) return false;
    for (uint8_t j = 0; j < num_bytes; j++) {
        if (!_i2c_send_data(*data)) return false;
        data++;
    }
    if (!_i2c_stop()) return false;
    return true;
}

bool _i2c_read_h(IQS5xx *iqs, uint16_t reg_addr, uint8_t *data, uint8_t num_bytes) {
    if (num_bytes == 0) num_bytes++;
    if (!_i2c_start()) return false;
    if (!_i2c_send_addr((iqs->i2c_address) << 1)) return false;
    if (!_i2c_send_data( (uint8_t)(reg_addr >> 8) )) return false;
    if (!_i2c_send_data( (uint8_t)reg_addr )) return false;
    if (!_i2c_start()) return false;
    if (!_i2c_send_addr(((iqs->i2c_address) << 1) + 0b1)) return false;
    for (uint8_t j = 0; j < num_bytes; j++) {
        if (j == (num_bytes - 1)) {
            if (!_i2c_recv_data(0)) return false;
        } else {
            if (!_i2c_recv_data(1)) return false;
        }
        *data = TWDR;
        data++;
    }
    if (!_i2c_stop()) return false;
    return true;
}