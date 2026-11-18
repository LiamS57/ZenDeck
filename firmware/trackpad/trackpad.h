#ifndef _TRACKPAD_H_
#define _TRACKPAD_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/Device/HIDClassDevice.h>

#include "descriptors.h"


#define IS_PRESSED(pin_reg, bit) ((pin_reg & _BV(bit)) ? 0 : 1)
#define CLAMP(val, min, max) (val > max ? max : (val < min ? min : val))

typedef struct {
    uint8_t BUTTONS;
    int8_t X;
    int8_t Y;
} USB_TrackpadReport_Data_t;

typedef struct {
    uint8_t VOL_BUTTONS;
} USB_VolumeReport_Data_t;


void SetupHardware(void);
void SetupIQS(void);

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