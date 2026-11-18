#include "trackpad.h"
#include "pin_defs.h"
#include "IQS5xx.h"
#include <avr/io.h>
#include <string.h>

static uint8_t PrevHIDReportBuffer[sizeof(USB_TrackpadReport_Data_t)];
USB_ClassInfo_HID_Device_t Trackpad_Interface = {
    .Config = {
        .InterfaceNumber = INTERFACE_ID_Trackpad,
        .ReportINEndpoint = {
            .Address = TRACKPAD_EPADDR,
            .Size = TRACKPAD_EPSIZE,
            .Banks = 1
        },
        .PrevReportINBuffer = PrevHIDReportBuffer,
        .PrevReportINBufferSize = sizeof(PrevHIDReportBuffer)
    }
};
static USB_TrackpadReport_Data_t prev_buffer_trackpad_report;
static USB_VolumeReport_Data_t prev_buffer_volume_report;

static IQS5xx iqs;
static IQS5xxOutput iqs_out;


int main(void) {
    SetupIQS();
    SetupHardware();
    GlobalInterruptEnable();

    while (true) {
        HID_Device_USBTask(&Trackpad_Interface);
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
    DDRD &= ~(_BV(VOL_PLUS) | _BV(VOL_MINUS) | _BV(TRACKPAD_SDA) | _BV(TRACKPAD_SCL));
    PORTD |= (_BV(VOL_PLUS) | _BV(VOL_MINUS) | _BV(TRACKPAD_SDA) | _BV(TRACKPAD_SCL));
    DDRE &= ~_BV(TRACKPAD_READY);
    PORTE |= _BV(TRACKPAD_READY);

    // i2c setup
    TWSR &= ~0b11;
    TWBR = 72;
    TWCR |= (_BV(TWEN) | _BV(TWEA));

    // USB setup
    USB_Init();
}

void SetupIQS(void) {
    iqs.enabled = true;
    iqs.i2c_address = TRACKPAD_I2C_ADDR;
    iqs.tap_timer = 0;
    iqs_out.valid = false;
}

void EVENT_USB_Device_Connect(void) {}
void EVENT_USB_Device_Disconnect(void) {}
void EVENT_USB_Device_ConfigurationChanged(void) {
    HID_Device_ConfigureEndpoints(&Trackpad_Interface);
    USB_Device_EnableSOFEvents();
}
void EVENT_USB_Device_ControlRequest(void) {
    HID_Device_ProcessControlRequest(&Trackpad_Interface);
}
void EVENT_USB_Device_StartOfFrame(void) {
    HID_Device_MillisecondElapsed(&Trackpad_Interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    uint8_t* const ReportID,
    const uint8_t ReportType,
    void* ReportData,
    uint16_t* const ReportSize
) {
    // trackpad data gathering
    USB_TrackpadReport_Data_t buf_trackpad;
    buf_trackpad.BUTTONS = 0;
    buf_trackpad.X = 0;
    buf_trackpad.Y = 0;
    if (IS_PRESSED(PINE, TRACKPAD_READY)) {
        IQS5xx_ProcessXY(&iqs, &iqs_out);
        if (iqs_out.valid) {
            if (iqs_out.move) {
                if (iqs_out.scroll) {
                    //
                } else {
                    buf_trackpad.X = (int8_t)CLAMP(iqs_out.rel_y, -127, 127);
                    buf_trackpad.Y = (int8_t)CLAMP(-iqs_out.rel_x, -127, 127);
                }
            }
            buf_trackpad.BUTTONS = (iqs_out.click_l ? 0b01 : 0b0) | (iqs_out.click_r ? 0b10 : 0b0);
        }
    }
    bool trackpad_changed = (buf_trackpad.BUTTONS ^ prev_buffer_trackpad_report.BUTTONS)
                          | (buf_trackpad.X ^ prev_buffer_trackpad_report.X)
                          | (buf_trackpad.Y ^ prev_buffer_trackpad_report.Y);

    // volume data gathering
    USB_VolumeReport_Data_t buf_volume;
    buf_volume.VOL_BUTTONS = (IS_PRESSED(PIND, VOL_PLUS) << 0)
                           | (IS_PRESSED(PIND, VOL_MINUS) << 1);
    bool volume_changed = (buf_volume.VOL_BUTTONS ^ prev_buffer_volume_report.VOL_BUTTONS);

    // report sending
    if (volume_changed) { // this first as it is less frequent
        *ReportID = 2;
        *ReportSize = sizeof(USB_VolumeReport_Data_t);
        memcpy(ReportData, &buf_volume, *ReportSize);
        memcpy(&prev_buffer_volume_report, &buf_volume, *ReportSize);
        return true;
    }
    if (trackpad_changed) {
        *ReportID = 1;
        *ReportSize = sizeof(USB_TrackpadReport_Data_t);
        memcpy(ReportData, &buf_trackpad, *ReportSize);
        memcpy(&prev_buffer_trackpad_report, &buf_trackpad, *ReportSize);
        return true;
    }
    *ReportSize = 0;
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