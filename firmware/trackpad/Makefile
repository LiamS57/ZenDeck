MCU          = atmega32u4
ARCH         = AVR8
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = trackpad
SRC          = trackpad.c descriptors.c IQS5xx.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
BOOT_START   = 0x0000
LUFA_PATH    = ../lufa/LUFA
CC_FLAGS     = -DB -DBOOT_START_ADDR=$(BOOT_START)UL -DUSE_LUFA_CONFIG_HEADER -Iconfig/
LD_FLAGS     = -Wl,--section-start=.text=$(BOOT_START)

# Default target
all:

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk


# clean:
# 	rm $(TARGET).??*
# 	rm -Rf obj