# PCBs

All PCB files have been edited through KiCad, but it should be trivial to import these into other editors. 

## Design Changes

Most of the actual changes have been made on the LHS/RHS Controller PCBs, but some of the other PCBs have been updated for newer KiCad version.

### Controller PCBs

#### ▶ Microcontroller Count Reduction

In the original RHS Controller, a third AVR microcontroller was used to offload processing for right-hand controls. I have adjusted the design to remove this microcontroller, and all inputs are now redirected back to the LHS Controller via an additional 10-pin FFC connector (J6 in LHS Controller). Likewise, the I2C bus connection on the first 10-pin FFC connector has been removed.

This configuration allows for all microcontrollers in the system to directly connect to the onboard USB hub (HS8836A). In the original design, if there was any need to reprogram the RHS microcontroller, it would have to be done manually through the SPI bus test pads with an in-system programmer; however, if all of them can connect to the hub, then they can all be re-flashed over USB while on the go.

One downside to this design is that it requires literally *every* pin on the gamepad Atmega32u4 to cover the rest of the inputs. The microcontroller is more than capable of handling this, but the ability to add anything else (like new buttons) is severely limited!

> [!WARNING]
In my current redesign, the rumble motors are unfortunately both on pins that use the **same timer** (Timer 4). This means they cannot use different PWM frequencies. I am currently looking into a firmware workaround for this, but if you use these PCB files, it might be helpful to swap the routing for one of the motors with another PWM-capable pin!

> [!NOTE]
In my implementation, I was unable to find an LED package that matched the footprints around the joystick holes. When looking at the LEDs specified in the original KiCad files (APA102), they match physically, but the pin layout is incorrect. I did not replace these and did not try to populate them when I ordered my boards, but the routing to the trackpad microcontroller has not been changed. YMMV!

#### ▶ Headphone Jack &rarr; I2C Header

I rarely use headphones with an aux cable nowadays, so my redesign replaces the headphone jack with a bit more board space and a header (J8 in LHS Controller) for I2C from the gamepad microcontroller. This bus connection can be used for anything, but its primary intended function in my case is to attach an IMU for gyroscopic aim-assist.

### Battery Management System (BMS)

*[COMING SOON]*