#include "descriptors.h"
#include <avr/pgmspace.h>


const USB_Descriptor_HIDReport_Datatype_t PROGMEM GamepadReport[] = {
    HID_RI_USAGE_PAGE(8, 0x01),
    HID_RI_USAGE(8, 0x05),
    HID_RI_COLLECTION(8, 0x01), // application
        // left joystick
        HID_RI_USAGE(8, 0x01),
        HID_RI_COLLECTION(8, 0x00),
            HID_RI_USAGE(8, 0x30), // X
            HID_RI_USAGE(8, 0x31), // Y
            HID_RI_LOGICAL_MINIMUM(16, -32768),
            HID_RI_LOGICAL_MAXIMUM(16, 32767),
            HID_RI_REPORT_SIZE(8, 16),
            HID_RI_REPORT_COUNT(8, 2),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),

        // right joystick
        HID_RI_USAGE(8, 0x01),
        HID_RI_COLLECTION(8, 0x00),
            HID_RI_USAGE(8, 0x33), // RX
            HID_RI_USAGE(8, 0x34), // RY
            HID_RI_LOGICAL_MINIMUM(16, -32768),
            HID_RI_LOGICAL_MAXIMUM(16, 32767),
            HID_RI_REPORT_SIZE(8, 16),
            HID_RI_REPORT_COUNT(8, 2),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),

        // triggers
        HID_RI_USAGE(8, 0x32), // Z
        HID_RI_USAGE(8, 0x35), // RZ
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(16, 32767),
        HID_RI_REPORT_SIZE(8, 16),
        HID_RI_REPORT_COUNT(8, 2),
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

        // d-pad
        HID_RI_USAGE(8, 0x39),
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(8, 7),
        HID_RI_PHYSICAL_MINIMUM(16, 0),
        HID_RI_PHYSICAL_MAXIMUM(16, 315),
        HID_RI_UNIT(8, 0x14),
        HID_RI_REPORT_SIZE(8, 4),
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NULLSTATE),
        HID_RI_REPORT_SIZE(8, 4), // padding
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_INPUT(8, HID_IOF_CONSTANT),

        // buttons
        HID_RI_USAGE_PAGE(8, 0x09),
        HID_RI_USAGE(8, 1), // BTN_A
        HID_RI_USAGE(8, 2), // BTN_B
        HID_RI_USAGE(8, 4), // BTN_X
        HID_RI_USAGE(8, 5), // BTN_Y
        HID_RI_USAGE(8, 7), // BTN_TL
        HID_RI_USAGE(8, 8), // BTN_TR
        HID_RI_USAGE(8, 14), // BTN_THUMBL
        HID_RI_USAGE(8, 15), // BTN_THUMBR
        HID_RI_USAGE(8, 13), // BTN_MODE
        HID_RI_USAGE(8, 12), // BTN_START
        HID_RI_USAGE(8, 11), // BTN_SELECT
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(8, 1),
        HID_RI_REPORT_SIZE(8, 1),
        HID_RI_REPORT_COUNT(8, 11),
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_REPORT_SIZE(8, 5), // padding
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_INPUT(8, HID_IOF_CONSTANT),

        // vibration
        // HID_RI_USAGE_PAGE(8, 0x0F),
        // HID_RI_USAGE(8, 0x21),
        // HID_RI_COLLECTION(8, 0x02),
        //     HID_RI_USAGE(8, 0x70),
        //     HID_RI_REPORT_SIZE(8, 8),
        //     HID_RI_REPORT_COUNT(8, 2),
        //     HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        // HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0)
};

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
    .USBSpecification = VERSION_BCD(1, 1, 0),
    .Class = USB_CSCP_NoDeviceClass,
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,
    .VendorID = 0x03EB,
    .ProductID = 0x2043,
    .ReleaseNumber = VERSION_BCD(0, 1, 0),
    .ManufacturerStrIndex = NO_DESCRIPTOR,
    .ProductStrIndex = NO_DESCRIPTOR,
    .SerialNumStrIndex = NO_DESCRIPTOR,
    .NumberOfConfigurations = 1
};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {
        .Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces = 1,
        .ConfigurationNumber = 1,
	    .ConfigurationStrIndex = NO_DESCRIPTOR,
        .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),
        .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)  // may need more for rumble motors??
    },
    .HID_Interface = {
        .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},
        .InterfaceNumber = INTERFACE_ID_Gamepad,
        .AlternateSetting = 0x00,
        .TotalEndpoints = 1,
        .Class = HID_CSCP_HIDClass,
        .SubClass = HID_CSCP_NonBootSubclass,
        .Protocol = HID_CSCP_NonBootProtocol,
        .InterfaceStrIndex = NO_DESCRIPTOR
    },
    .Gamepad_HID = {
        .Header = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},
        .HIDSpec = VERSION_BCD(1, 1, 1),
        .CountryCode = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType = HID_DTYPE_Report,
        .HIDReportLength = sizeof(GamepadReport)
    },
    .Gamepad_INEndpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
        .EndpointAddress = GAMEPAD_EPADDR,
        .EndpointSize = GAMEPAD_EPSIZE,
        .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .PollingIntervalMS = 0x05
    }
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint16_t wIndex, const void** const DescriptorAddress) {
    const uint8_t DescriptorType = (wValue >> 8);
    // const uint8_t DescriptorNumber = (wValue & 0xFF);

    // redirect to address in progmem
    const void* Address = NULL;
    uint16_t Size = NO_DESCRIPTOR;
    switch (DescriptorType) {
        case DTYPE_Device:
            Address = &DeviceDescriptor;
            Size = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = &ConfigurationDescriptor;
            Size = sizeof(USB_Descriptor_Configuration_t);
            break;
        case HID_DTYPE_HID:
            Address = &ConfigurationDescriptor.Gamepad_HID;
            Size = sizeof(USB_HID_Descriptor_HID_t);
            break;
        case HID_DTYPE_Report:
            Address = &GamepadReport;
            Size = sizeof(GamepadReport);
            break;
    }

    *DescriptorAddress = Address;
	return Size;
}