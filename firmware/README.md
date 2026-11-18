# Firmware

Instead of using Arduino sketches (or Arduino.h in general), I have opted to write individual AVR C programs for each microcontroller to try and maximize space efficiency (primarily since both LHS and RHS gamepad microcontrollers are combined into one in my redesign).

In order to build these, make sure you have installed/downloaded the following:
- An AVR C toolchain (such as `avr-gcc`)
- The most recent [LUFA library](https://github.com/abcminiuser/lufa)

After setting these up and ensuring the `LUFA_PATH` variable in each Makefile points to your LUFA library directory, you can build each program using the following commands:

```
make clean
make
```

For flashing, I use `avrdude` on the final built .hex file (as shown in the `flash.sh` scripts in each subdirectory), but other flashers should work fine as well.

## Bootloader

If you purchase the Atmega32u4 microcontrollers directly (such as through Digikey), the preloaded factory bootloader is only capable of one-time programming as it requires a full chip erase during writing. As such, I recommend installing another bootloader using the SPI interface test pads.

I recommend *Caterina*, which is available in the [ArduinoCore-avr](https://github.com/arduino/ArduinoCore-avr) repository. You can use one of the provided .hex files that best matches the configuration (Atmega32u4, 5V, 16MHz), or build it as needed (using [LUFA-111009](https://github.com/abcminiuser/lufa/releases/tag/LUFA-111009)). This will allow you to reprogram the microcontroller many times over USB -- similar to an Arduino -- in case you want to make firmware edits on the go.

## Gamepad

The original sketch for the gamepad utilizes the XInput library to emulate an Xbox controller. However, since I want to incorporate IMU-based gyroscopic aim-assist, I opted to use USB HID reports through LUFA to build a generic Linux-compatible gamepad. This allows for quite a bit of additional flexibility -- in theory, any input defined in the [USB HID Usage Tables (HUT)](https://www.usb.org/hid) can be added to the report descriptor (for example, a button macro to mute/unmute!)

### Joystick/Trigger Automatic Calibration

*[COMING SOON]*

### TODO

- Implement dual (split) rumble motor control
- Edit report descriptor for rumble motors
- Implement IMU reading/processing
- Edit report descriptor for IMU

## Trackpad

Similar to the gamepad program, USB HID reports are used to inform the host system of any input from the trackpad. For this code, however, I use **two separate HID reports** to handle volume buttons and trackpad motion, respectively.

Additionally, since I opted not to use the LED system, I have not included any code for LED color operation. However, the current PCB design still utilizes pins for LED operation, so it should still be possible given the right diodes are installed. I may explore this in future commits.

### IQS5xx.c/.h

These files manage communication with the trackpad module itself. They are directly based on the original driver files included with the original project's ZenDeck_Trackpad sketch, but modified for pure AVR C (no Arduino.h). They also incorporate portions of the original I2C.cpp/.h files to handle I2C calls.

## Battery Management System (BMS)

*[COMING SOON]*