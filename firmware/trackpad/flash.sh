#!/bin/sh

sudo avrdude -c avr109 -v -P /dev/ttyACM0 -p atmega32u4 -U flash:w:trackpad.hex:i