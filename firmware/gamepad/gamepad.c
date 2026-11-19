#include "gamepad.h"
#include "pin_defs.h"
#include "analog.h"
#include <avr/io.h>

#define PWM_TOP 799
#define ABS(x) (x < 0 ? -x : x)


static uint8_t PrevHIDReportBuffer[sizeof(USB_GamepadReport_Data_t)];
USB_ClassInfo_HID_Device_t Gamepad_Interface = {
    .Config = {
        .InterfaceNumber = INTERFACE_ID_Gamepad,
        .ReportINEndpoint = {
            .Address = GAMEPAD_EPADDR,
            .Size = GAMEPAD_EPSIZE,
            .Banks = 1
        },
        .PrevReportINBuffer = PrevHIDReportBuffer,
        .PrevReportINBufferSize = sizeof(PrevHIDReportBuffer)
    }
};

static uint16_t prev_button_states[DEBOUNCE_COUNT];
static uint8_t prev_button_states_j;
static uint16_t buttons_deb;

void read_buttons(void);
uint8_t read_dpad(void);

void set_vibe(uint16_t strength);


int main(void) {
    for (uint8_t j = 0; j < DEBOUNCE_COUNT; j++) {
        prev_button_states[j] = 0;
    }
    prev_button_states_j = 0;
    buttons_deb = 0;

    SetupHardware();
    GlobalInterruptEnable();

    init_joysticks();

    while (true) {
        HID_Device_USBTask(&Gamepad_Interface);
        USB_USBTask();
    }
}

void SetupHardware(void) {
    // disable watchdog
    MCUSR &= ~_BV(WDRF);
	wdt_disable();
    // disable clock division
    clock_prescale_set(clock_div_1);

    // digital input setup
    DDRB &= 0x00; // all pins in port b are in use!
    PORTB |= 0xFF; // all pins need pullup resistors enabled!
    DDRC &= ~(_BV(DPAD_D) | _BV(DPAD_L));
    PORTC |= (_BV(DPAD_D) | _BV(DPAD_L));
    DDRD &= ~(_BV(BUTTON_A) | _BV(BUTTON_B) | _BV(BUTTON_START) | _BV(BUTTON_SELECT));
    PORTD |= (_BV(BUTTON_A) | _BV(BUTTON_B) | _BV(BUTTON_START) | _BV(BUTTON_SELECT));
    DDRE &= ~_BV(BUTTON_MENU);
    PORTE |= _BV(BUTTON_MENU);

    // ADC setup
    setup_adc_hw();

    // i2c setup

    // rumble pwm setup
    // DDRD |= (_BV(RUMBLE_L) | _BV(RUMBLE_R));
    DDRD |= _BV(RUMBLE_L);
    TCCR4B = (TCCR4B & 0xF0) | 0x02; // prescale 2
    TC4H = (PWM_TOP >> 8);
    OCR4C = (PWM_TOP & 0xFF);
    TCCR4C = (TCCR4C & ~_BV(COM4D0)) | _BV(COM4D1) | _BV(PWM4D);
    set_vibe(0); // initially off

    // USB setup
    USB_Init();
}

void EVENT_USB_Device_Connect(void) {}
void EVENT_USB_Device_Disconnect(void) {}
void EVENT_USB_Device_ConfigurationChanged(void) {
    HID_Device_ConfigureEndpoints(&Gamepad_Interface);
    USB_Device_EnableSOFEvents();
}
void EVENT_USB_Device_ControlRequest(void) {
    HID_Device_ProcessControlRequest(&Gamepad_Interface);
}
void EVENT_USB_Device_StartOfFrame(void) {
    HID_Device_MillisecondElapsed(&Gamepad_Interface);
}

// uint16_t lerp(uint16_t a, uint16_t b, uint8_t t) {
//     return (uint16_t)(((((int32_t)b - a) * t) >> 8) + a);
// }

void read_buttons(void) {
    // get newest button states
    uint16_t states = (IS_PRESSED(PINB, DPAD_U) << 0) // dpad_up
                    | (IS_PRESSED(PINC, DPAD_D) << 1) // dpad_down
                    | (IS_PRESSED(PINC, DPAD_L) << 2) // dpad_left
                    | (IS_PRESSED(PINB, DPAD_R) << 3) // dpad_right
                    | (IS_PRESSED(PIND, BUTTON_A) << 4)
                    | (IS_PRESSED(PIND, BUTTON_B) << 5)
                    | (IS_PRESSED(PINB, BUTTON_X) << 6)
                    | (IS_PRESSED(PINB, BUTTON_Y) << 7)
                    | (IS_PRESSED(PINB, BUTTON_L1) << 8)
                    | (IS_PRESSED(PINB, BUTTON_R1) << 9)
                    | (IS_PRESSED(PINB, JOY_L_PRESS) << 10)
                    | (IS_PRESSED(PINB, JOY_R_PRESS) << 11)
                    | (IS_PRESSED(PINE, BUTTON_MENU) << 12)
                    | (IS_PRESSED(PIND, BUTTON_START) << 13)
                    | (IS_PRESSED(PIND, BUTTON_SELECT) << 14);

    // store in buffer and cycle pointer
    prev_button_states[prev_button_states_j] = states;
    prev_button_states_j = (prev_button_states_j + 1) % DEBOUNCE_COUNT;

    // check buffer for debounced (valid) changes
    uint16_t deb_pressed = 0xFFFF; // button states that have been pressed long enough
    uint16_t deb_unpressed = 0x0000; // button states that have remained unpressed long enough
    for (uint8_t j = 0; j < DEBOUNCE_COUNT; j++) {
        deb_pressed &= prev_button_states[j];
        deb_unpressed |= prev_button_states[j];
    }
    uint16_t deb_valid = deb_pressed | deb_unpressed;

    // update debounced (for valid changes only)
    buttons_deb = (states & deb_valid) | (buttons_deb & ~deb_valid);
}

uint8_t read_dpad(void) {
    uint8_t dpad = (uint8_t)(buttons_deb & 0x0F);

    if ((dpad & 0b0001) == 0b0001) return 0; // up
    if ((dpad & 0b1001) == 0b1001) return 1; // up, right
    if ((dpad & 0b1000) == 0b1000) return 2; // right
    if ((dpad & 0b1010) == 0b1010) return 3; // down, right
    if ((dpad & 0b0010) == 0b0010) return 4; // down
    if ((dpad & 0b0110) == 0b0110) return 5; // down, left
    if ((dpad & 0b0100) == 0b0100) return 6; // left
    if ((dpad & 0b0101) == 0b0101) return 7; // up, left
    return 8;
}

void set_vibe(uint16_t strength) {
    uint16_t val = strength > PWM_TOP ? PWM_TOP : strength;
    TC4H = val >> 8;
    OCR4D = val & 0xFF;
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    uint8_t* const ReportID,
    const uint8_t ReportType,
    void* ReportData,
    uint16_t* const ReportSize
) {
    USB_GamepadReport_Data_t* GamepadReport = (USB_GamepadReport_Data_t*)ReportData;

    read_adc();
    update_joystick_centers();
    update_analog_limits();
    GamepadReport->JOY_LX = map_joystick_axis(adc_state[0], &joy_lx);
    GamepadReport->JOY_LY = -map_joystick_axis(adc_state[1], &joy_ly);
    GamepadReport->JOY_RX = map_joystick_axis(adc_state[2], &joy_rx);
    GamepadReport->JOY_RY = -map_joystick_axis(adc_state[3], &joy_ry);
    GamepadReport->TRIG_L = 32767L - map_trigger_axis(adc_state[4], trig_l_u, trig_l_l);
    GamepadReport->TRIG_R = map_trigger_axis(adc_state[5], trig_r_u, trig_r_l);

    read_buttons();
    GamepadReport->DPAD = read_dpad();
    GamepadReport->BUTTONS = buttons_deb >> 4; // hopping over dpad

    *ReportSize = sizeof(USB_GamepadReport_Data_t);
    return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    const uint8_t ReportID,
    const uint8_t ReportType,
    const void* ReportData,
    const uint16_t ReportSize
) {
    // none
}