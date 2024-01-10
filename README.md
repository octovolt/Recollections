Recollections
=============

Recollections is a voltage memory module for Eurorack modular synthesizer systems offering 8 output
channels across 16 presets, 16 banks and 16 modules for a total of 32,768 recallable voltages. It
may be quickly reconfigured to function as a controller, a sequencer, a track-and-hold, a
sample-and-hold, a random voltage generator, or all of these at the same time and in conjunction
with each other. The data is stored on an SD card for later recall, and the storage files are easy
to read and edit.

![recollections-catalog](https://github.com/octovolt/Recollections/assets/78008936/e6ed3ae6-a958-4913-9924-764b3f795558)

Demo Videos
-----------
Please see the [Octopus Arts YouTube channel](https://www.youtube.com/@octopus-arts).

User Manual
-----------
A user manual may be found here:
[https://docs.google.com/document/d/1RPAPcG-z0c8NyQLCnBXKaI8nIInMwIazlKRt4L1SheU](https://docs.google.com/document/d/1RPAPcG-z0c8NyQLCnBXKaI8nIInMwIazlKRt4L1SheU)

Project Status
--------------
Recollections is currently a work in progress, with a goal date of January 15, 2023. I am currently
switching from the Teensy microcontroller board to the Raspberry Pi Pico board, primarily due to the
difference in price. Teensy-based PCB/panel sets are available now, but you can save $25 or more
in parts if you are patient enough to let me get the costs down. Also, I will be offering kits that
will bring the price down even further due to the need to source parts from multiple companies and
pay the shipping fees.

How to Compile for RP2040 / Raspberry Pi Pico
---------------------------------------------
Because this code was originally writing for Teensy 3.6 and Teensy 4.1, I decided to use the
[arduino-pico](https://github.com/earlephilhower/arduino-pico) library to enable it for the Pico.
Please use this to update your boards manager in the Arduino IDE prior to compiling so that
Raspberry Pi Pico becomes a possible compilation target.

Additionally, the following hack is required:
* Update files related to StreamUtils to include `<api/String.h>` instead of `<WString.h>`. Please
see [SDCard.h](https://github.com/octopus-arts/Recollections/blob/main/SDCard.h) for details.

How to Compile for Teensy
-------------------------
Teensy has the benefits of increased speed, an onboard SD card reader and true random number
generation. Unfortunately, it also has a 10-bit ADC rather than Pico's 12-bit, and it is about $25
USD more than the Pico. For this reason, the Pico is now the main target of this project. However,
you might care about the aforementioned benefits, or you might simply have a Teensy lying around
waiting for a project.

The following hacks are required for Teensy:
* Teensy 3.6 requires the [i2c_t3]() library to be included instead of the more standard `Wire`
library, so a few external libraries need to be changed to include `i2c_t3` instead. Please see
details at the top of
[Recollections.ino](https://github.com/octopus-arts/Recollections/blob/main/Recollections.ino).
This is not an issue for Teensy 4.1, which can use `Wire` instead.
* The `SD` library used for Teensy must be the Teensy-specific one. You may need to rename other
`SD.h` files to something like `SD-disabled.h` to get the compiler to use the correct library.

Additional Dependencies
-----------------------
In addition to `arduino-pico` and `api/String` for Pico, or `i2c_t3` for Teensy 3.6, Recollections
also depends on the following non-core libraries across all platforms:
* [Adafruit_MCP4728](https://github.com/adafruit/Adafruit_MCP4728)
* [Adafruit_Seesaw](https://github.com/adafruit/seesaw)
* [ArduinoJson](https://arduinojson.org/)
* [StackString](https://gitlab.com/arduino-libraries/stackstring)
* [StreamUtils](https://github.com/bblanchon/ArduinoStreamUtils)
