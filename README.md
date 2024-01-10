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
Please see the [Octovolt YouTube channel](https://www.youtube.com/@octopus-arts).

User Manual
-----------
A user manual may be found here:
[https://docs.google.com/document/d/1RPAPcG-z0c8NyQLCnBXKaI8nIInMwIazlKRt4L1SheU](https://docs.google.com/document/d/1RPAPcG-z0c8NyQLCnBXKaI8nIInMwIazlKRt4L1SheU)

Project Status
--------------
Recollections is currently a work in progress, with a goal date of February 14, 2024. The code was 
originally written for Teensy 3.6, then for Teensy 4.1, but due to the high cost of those 
microcontroller boards, I chose to switch to the Raspberry Pi Pico. The Pico is about $30 cheaper, 
but it's slower, most notably with reading and writing to the SD Card. On the plus side, beyond the 
cheaper price, the Pico supports 12-bit data on its ADC, so the resolution of the voltages is truly 
4096 steps rather than the 1024 steps of Teensy's 10-bit ADC. However, I will continue to make 
Teensy-based PCB/panel sets available for the hard core DIYer who wants faster SD Card writes and
does not care about the various benefits of the Pico. Please [contact me](https://octovolt.xyz/contact) 
if you are interested in using a Teensy 3.6 or 4.1. I will also be offering both PCB/panel sets and 
full kits for the Pico. While the kits are more expensive than the boards, they are much more convenient
and avoid the shipping costs that would be incurred by ordering parts from multiple suppliers.

Compiling the Code
------------------
Across all platforms, I use the Arduino IDE to compile the code. Sometimes I wish I was using pure C++
and CMake, but the many libraries offered by the Arduino ecosystem has kept me from doing this. Maybe 
someday.

How to Compile for RP2040 / Raspberry Pi Pico
---------------------------------------------
Because this code was originally writing for Teensy 3.6 and Teensy 4.1, I decided to use the
[arduino-pico](https://github.com/earlephilhower/arduino-pico) library to enable it for the Pico.
Please use this to update your boards manager in the Arduino IDE prior to compiling so that
Raspberry Pi Pico becomes a possible compilation target.

Additionally, the following hack is required:
* Update files related to StreamUtils to include `<api/String.h>` instead of `<WString.h>`. Please
see [SDCard.h](https://github.com/octovolt/Recollections/blob/main/SDCard.h) for details.

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
[Recollections.ino](https://github.com/octovolt/Recollections/blob/main/Recollections.ino).
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

Contributing
------------
Pull requests are absolutely welcome, but we should probably discuss your idea before you expect to
merge it into the repo -- maybe even before you write the code. In terms of formatting, I do expect
strict indenting with 2 spaces, not tabs. Otherwise, please see the code for the various conventions
I use. Most are standard, a few are idiosyncratic, but I want to keep the code consistent.

My priorities for future improvements:

* Unit tests. I'm ashamed at the lack of tests. Pull requests with tests will be the most appreciated.
* I'm very interested in using both MIDI and i2c to control the module. MIDI would require an expansion
module, but I believe the code can be written for i2c and a future MIDI expansion module would simply
translate MIDI into i2c commands.
* I'd also like to leverage i2c to send and receive messages between more than one Recollections
module to create bigger sequencers. That is, I envision two linked Recollections modules creating a 
32-step sequencer, or even four modules creating a 64-step sequencer.
* I sometimes wonder how I might create polychronic sequences on the different output channels. That is,
it would be great to have a 16-step sequence on channel 1, an 8-step sequence on channel 2, and a 5-step
sequence on channel 3. However, this could introduce a lot of complexity not only within the UI, but also 
within the data itself.

Thanks
------
Thanks for taking a look at this project. It's been a labor of love since I first conceived of the idea on
my way to Europe in June of 2021. Unfortunately, I came down with covid while in London, so to take my mind 
off the horrible cough I started writing the code for Recollections. I designed the first version of the 
circuit and laid out the PCB when I got back to California, but many iterations have happened since then.


