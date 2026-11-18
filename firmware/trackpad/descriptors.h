#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/Device/HIDClassDevice.h>

#define TRACKPAD_EPADDR (ENDPOINT_DIR_IN | 1)
#define TRACKPAD_EPSIZE 8

typedef struct {
    USB_Descriptor_Configuration_Header_t Config;
	USB_Descriptor_Interface_t            HID_Interface;
	USB_HID_Descriptor_HID_t              Trackpad_HID;
	USB_Descriptor_Endpoint_t             Trackpad_INEndpoint;
} USB_Descriptor_Configuration_t;

enum InterfaceDescriptors_t {
	INTERFACE_ID_Trackpad = 0,
};

uint16_t CALLBACK_USB_GetDescriptor(
    const uint16_t wValue,
    const uint16_t wIndex,
    const void** const DescriptorAddress
) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif