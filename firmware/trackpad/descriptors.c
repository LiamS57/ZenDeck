#include "descriptors.h"
#include <avr/pgmspace.h>


const USB_Descriptor_HIDReport_Datatype_t PROGMEM TrackpadReport[] = {
    // trackpad
    HID_RI_USAGE_PAGE(8, 0x01),
    HID_RI_USAGE(8, 0x02),
    HID_RI_COLLECTION(8, 0x01), // application
        HID_RI_REPORT_ID(8, 1),
        HID_RI_USAGE(8, 0x01),
        HID_RI_COLLECTION(8, 0x00),
            // buttons
            HID_RI_USAGE_PAGE(8, 0x09),
            HID_RI_USAGE_MINIMUM(8, 1),
            HID_RI_USAGE_MAXIMUM(8, 3),
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(8, 1),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_REPORT_COUNT(8, 3),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_SIZE(8, 5), // padding
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
            // axes
            HID_RI_USAGE_PAGE(8, 0x01),
            HID_RI_USAGE(8, 0x30), // X
            HID_RI_USAGE(8, 0x31), // Y
            HID_RI_LOGICAL_MINIMUM(8, -127),
            HID_RI_LOGICAL_MAXIMUM(8, 127),
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_REPORT_COUNT(8, 2),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),
        HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0),

    // volume up/down
    HID_RI_USAGE_PAGE(8, 0x0C),
    HID_RI_USAGE(8, 0x01),
    HID_RI_COLLECTION(8, 0x01), // application
        HID_RI_REPORT_ID(8, 2),
        HID_RI_USAGE(8, 0xE9), // VOL_PLUS
        HID_RI_USAGE(8, 0xEA), // VOL_MINUS
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(8, 1),
        HID_RI_REPORT_SIZE(8, 1),
        HID_RI_REPORT_COUNT(8, 2),
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_REPORT_SIZE(8, 6), // padding
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_INPUT(8, HID_IOF_CONSTANT),
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
    .ProductID = 0x2041,
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
        .InterfaceNumber = INTERFACE_ID_Trackpad,
        .AlternateSetting = 0x00,
        .TotalEndpoints = 1,
        .Class = HID_CSCP_HIDClass,
        .SubClass = HID_CSCP_NonBootSubclass,
        .Protocol = HID_CSCP_NonBootProtocol,
        .InterfaceStrIndex = NO_DESCRIPTOR
    },
    .Trackpad_HID = {
        .Header = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},
        .HIDSpec = VERSION_BCD(1, 1, 1),
        .CountryCode = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType = HID_DTYPE_Report,
        .HIDReportLength = sizeof(TrackpadReport)
    },
    .Trackpad_INEndpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
        .EndpointAddress = TRACKPAD_EPADDR,
        .EndpointSize = TRACKPAD_EPSIZE,
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
            Address = &ConfigurationDescriptor.Trackpad_HID;
            Size = sizeof(USB_HID_Descriptor_HID_t);
            break;
        case HID_DTYPE_Report:
            Address = &TrackpadReport;
            Size = sizeof(TrackpadReport);
            break;
    }

    *DescriptorAddress = Address;
	return Size;
}