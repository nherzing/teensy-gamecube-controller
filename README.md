# Teensy GameCube controller USB HID Adapter
Uses a Teensy 3.5 to make a Nintendo GameCube controller appear as a USB HID.

# Hardware Setup
The only required connections (your controller may vary) are:
* 3.3V SGNL
* 3.3V
* either GND (they are connected on my controller

This code uses GPIO D0 for the 3.3V SGNL which is pin 2 on the Teensy 3.5.

## GameCube Controller Adapter Pinout
Reading with rounded bit on top
|left|center|right|
|---|-------|-----|
|GND | - | 3.3V|
| 5V| 3.3V SGNL | GND|

## Porting to other Teensy devices
Some of the signal processing code is clock-speed dependent but is not written in a generic fashion. Other than that, I believe it should work on other Teensy devices.
