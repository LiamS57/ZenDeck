#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/Device/HIDClassDevice.h>

#include "descriptors.h"
#include "analog.h"


#define IS_PRESSED(pin_reg, bit) ((pin_reg & _BV(bit)) ? 0 : 1)

/* -- Major Options -- */

// #define MANUAL_CALIB
#define DEBOUNCE_COUNT 5

/* -- Vibration Maximums -- */

#define VIBE_L_MAX 100
#define VIBE_R_MAX 100

/* -- USB HID -- */

typedef struct {
    int16_t JOY_LX;
    int16_t JOY_LY;
    int16_t JOY_RX;
    int16_t JOY_RY;
    uint16_t TRIG_L;
    uint16_t TRIG_R;
    uint8_t DPAD;
    uint16_t BUTTONS;
} USB_GamepadReport_Data_t;


void SetupHardware(void);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_StartOfFrame(void);

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    uint8_t* const ReportID,
    const uint8_t ReportType,
    void* ReportData,
    uint16_t* const ReportSize
);

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    const uint8_t ReportID,
    const uint8_t ReportType,
    const void* ReportData,
    const uint16_t ReportSize
);


#endif