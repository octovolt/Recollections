/**
 * Recollections: Constants
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_MCP4728.h>
#include <SPI.h>
#include <string>

// Included for SD/FS constants
#ifdef CORE_TEENSY
  // This needs to be the Teensy-specific version of this. Rename others to disambiguate.
  #include <SD.h>
#else
  #include <SDFS.h>
#endif

#include "typedefs.h"

#ifndef RECOLLECTIONS_CONSTANTS_H_
#define RECOLLECTIONS_CONSTANTS_H_

#ifndef FILE_WRITE_BEGIN
  #define FILE_WRITE_BEGIN 2 // only defined in Teensy's SD library, apparently
#endif

// ----------------------------------- Bit Depth ---------------------------------------------------

#define MAX_UNSIGNED_12_BIT 4095
#define MAX_UNSIGNED_10_BIT 1023
#define MAX_UNSIGNED_8_BIT 255

#define PERCENTAGE_MULTIPLIER_10_BIT 0.000977517106549 // 1/1023 for 10-bit
#define PERCENTAGE_MULTIPLIER_12_BIT 0.0002442002442002 // 1/4095 for 12-bit
#define PERCENTAGE_MULTIPLIER_32_BIT 0.0000000004656613 // 1/2147483647 for 32-bit

#define VOLTAGE_VALUE_MAX 4095
#define VOLTAGE_VALUE_MID 2047

// ---------------------------------- Neotrellis ---------------------------------------------------

#define DEFAULT_BRIGHTNESS 100
#define COLOR_VALUE_MAX 255 // max brightness, relative to brightness setting
#define DIMMED_COLOR_MULTIPLIER 0.15

// ------------------------------ Timing and Flashing ----------------------------------------------

#define MOD_DEBOUNCE_TIME 300
#define FLASH_TIME 120
#define LONG_PRESS_TIME 1500
#define DEFAULT_TRIGGER_LENGTH 20

#define SAVE_CONFIRMATION_MAX_FLASHES 4

// ------------------------------ Hardware Environment ---------------------------------------------

// The version of the hardware expressed as a semver. See https://semver.org/
std::string const HARDWARE_SEMVER = "0.3.0";

// Whether we are powering the microcontroller through USB for the sake of development or debugging
bool const USB_POWERED = false;

// Whether the SD card is required to boot up the module. Used for development and debugging.
bool const REQUIRE_SD_CARD = true;

// ------------------------------------- SD Card ---------------------------------------------------

// Calculated with https://arduinojson.org/v6/assistant
#define BANK_JSON_DOC_DESERIALIZATION_SIZE 16384 // 14639 required
#define BANK_JSON_DOC_SERIALIZATION_SIZE 16384 // 14496 required
#define CONFIG_JSON_DOC_DESERIALIZATION_SIZE 1024 // 868 required
#define CONFIG_JSON_DOC_SERIALIZATION_SIZE 768 // 688 required
#define MODULE_JSON_DOC_DESERIALIZATION_SIZE 512 // 410 required
#define MODULE_JSON_DOC_SERIALIZATION_SIZE 384 // 336 required

#define CONFIG_SD_PATH "Recollections/Config.txt"
#define MODULE_SD_PATH_PREFIX "Recollections/Module_"

#ifdef ARDUINO_TEENSY41
  const uint8_t SD_CS_PIN = BUILTIN_SDCARD;
#elif defined(ARDUINO_TEENSY36)
  // // Use built-in SD for SPI on Teensy 3.5/3.6
  // // Teensy 4.0 use first SPI port.
  // // SDCARD_SS_PIN is defined for the built-in SD on some boards.
  // #ifndef SDCARD_SS_PIN
  //   const uint8_t SD_CS_PIN = SS;
  // #else
  //   // Assume built-in SD is used.
  const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
  // #endif
#else
  const uint8_t SD_CS_PIN = 13;
#endif

// Different ways to open files on the SD card
uint8_t const SD_READ_CREATE = (uint8_t)(O_READ | O_CREAT);

// --------------------------- Microcontroller Board Pins ------------------------------------------

#ifdef CORE_TEENSY

  // REMEMBER that pins on the left side of Teensy start with GND and only then
  // begin counting from 0, so the second pin is 0, the third pin is 1, etc.

  // Analog inputs
  /** Control voltage input to be recorded. */
  uint8_t const CV_INPUT = A6; // (originally A0? pin "14" on Teensy 3.6)
  uint8_t const UNCONNECTED_ANALOG_PIN = 255; // not used on Teensy

  // Digital inputs
  /** Gate to start/stop automatic recording. Recording occurs when the gate is high. */
  uint8_t const REC_INPUT = 0;
  /** Gate to advance the current preset to the next preset. */
  uint8_t const ADV_INPUT = 3;
  /** Button (gate) that acts as a modifier for keys or as an escape to quit the current screen */
  uint8_t const MOD_INPUT = 4;
  /** Gate that determines if a key is being pressed. Avoids unnecessary polling. */
  uint8_t const TRELLIS_INTERRUPT_INPUT = 5;
  /** [EXPANSION] Gate to reverse the direction of preset advancement. */
  uint8_t const REV_INPUT = 6;
  /** [EXPANSION] Gate to reset the preset advancement to the first preset. */
  uint8_t const RESET_INPUT = 7;
  /** [EXPANSION] Gate to advance the current bank to the next bank. */
  uint8_t const BANK_ADV_INPUT = 14;
  /** [EXPANSION] Gate to reverse the direction of bank advancement. */
  uint8_t const BANK_REV_INPUT = 15;

  // Digital outputs
  uint8_t const BOARD_LED = 13;

  // Digital i2c pins - leader
  uint8_t const SCL0 = 19;
  uint8_t const SDA0 = 18;

  // Digital i2c pins - follower
  // NOTE: On hardware v.0.3.0, in an attempt at multi-platform design, pins 16 & 37 are tied
  // together as are pins 17 & 38. Not sure if this will work.
  #ifdef ARDUINO_TEENSY41
    uint8_t const SCL1 = 16;
    uint8_t const SDA1 = 17;
  #else
    uint8_t const SCL1 = 37;
    uint8_t const SDA1 = 38;
  #endif

  // Digital SPI pins - SD card - unused on Teensy but defining to be consistent for the compiler
  uint8_t const SPI_RX = 255;
  uint8_t const SPI_CSN = 255;
  uint8_t const SPI_SCK = 255;
  uint8_t const SPI_TX = 255;

#else // assume Raspberry Pi Pico

  // Analog inputs
  /** Control voltage input to be recorded. */
  uint8_t const CV_INPUT = 26;
  uint8_t const UNCONNECTED_ANALOG_PIN = 28; // noise for randomSeed()

  // Digital inputs
  /** Gate to start/stop automatic recording. Recording occurs when the gate is high. */
  uint8_t const REC_INPUT = 14;
  /** Gate to advance the current preset to the next preset. */
  uint8_t const ADV_INPUT = 13;
  /** Button (gate) that acts as a modifier for keys or as an escape to quit the current screen */
  uint8_t const MOD_INPUT = 15;
  /** Gate that determines if a key is being pressed. Avoids unnecessary polling. */
  uint8_t const TRELLIS_INTERRUPT_INPUT = 12;

  // Expansion
  /** Gate to reverse the direction of preset advancement. */
  uint8_t const REV_INPUT = 0;
  /** Gate to reset the preset advancement to the first preset. */
  uint8_t const RESET_INPUT = 1;
  /** Gate to advance the current bank to the next bank. */
  uint8_t const BANK_ADV_INPUT = 2;
  /** Gate to reverse the direction of bank advancement. */
  uint8_t const BANK_REV_INPUT = 3;
  /** Gate to toggle between two banks */
  uint8_t const BANK_TOGGLE = 8;
  /** Gate input reserved for future development */
  uint8_t const RESERVED_INPUT = 9;

  // Digital outputs
  uint8_t const BOARD_LED = 25; // does this have a normal pin number? will this work?

  // Digital i2c pins - leader
  uint8_t const RECOLLECTIONS_SDA0 = 4;
  uint8_t const RECOLLECTIONS_SCL0 = 5;

  // Digital i2c pins - follower
  uint8_t const RECOLLECTIONS_SDA1 = 6;
  uint8_t const RECOLLECTIONS_SCL1 = 7;

  // Digital SPI pins - SD card
  uint8_t const RECOLLECTIONS_SPI_RX = 16;
  uint8_t const RECOLLECTIONS_SPI_CSN = 17;
  uint8_t const RECOLLECTIONS_SPI_SCK = 18;
  uint8_t const RECOLLECTIONS_SPI_TX = 19;

#endif

// ------------------------------------- Screens ---------------------------------------------------

/**
 * Screen constants.
 *
 * Please note: these are referred to as either "sections" or "screens" in the user manual. There
 * are five major "sections": preset selection, channel editing, recording, global editing, and bank
 * selection. Some of these have additional "screens" other than their first.
 */
typedef struct Screen {
  // Preset selection.
  // Color: white
  Screen_t PRESET_SELECT = 0;

  // Select the channel on which to perform operations, such as recording while in PRESET_SELECT.
  // Color: white
  Screen_t PRESET_CHANNEL_SELECT = 1;

  // Intermediary screen allowing navigation to all five major sections.
  // Colors: blue, red, yellow, green
  Screen_t SECTION_SELECT = 2;

  // Configure channels for gates, CV or random.
  // Color: yellow
  Screen_t EDIT_CHANNEL_SELECT = 3;

  // Edit any of the 16 voltages for a single channel, or configure them to be locked, inactive or
  // random.
  // Color: yellow
  Screen_t EDIT_CHANNEL_VOLTAGES = 4;

  // Record voltages, either manually on any of the 8 channels for a single preset, or across
  // multiple presets on a single channel while the ADV input is receiving a clock/gate/trigger.
  // This is also the screen where one can set up automatic recording.
  // Color: red
  Screen_t RECORD_CHANNEL_SELECT = 5;

  // Global editing of presets, including whether presets are addressed at all when a
  // gate/trigger/clock is received at the ADV input.
  // Color: green
  Screen_t GLOBAL_EDIT = 6;

  // Select a new bank from memory. Each bank has 16 presets and 8 channels.
  // Color: blue
  Screen_t BANK_SELECT = 7;

  // Load an entirely new module (banks, presets, channels) from the SD card.
  // Colors: green, magenta
  Screen_t MODULE_SELECT = 8;

  // The module has entered an error state
  // Color: red
  Screen_t ERROR = 9;
} Screen;
Screen constexpr SCREEN;

// ----------------------------------- Quadrants ---------------------------------------------------

/**
 * Constants for the four quadrants of the 16 buttons.
 */
typedef struct Quadrant {
  Quadrant_t INVALID = 0;
  Quadrant_t NW = 1;
  Quadrant_t NE = 2;
  Quadrant_t SW = 3;
  Quadrant_t SE = 4;
} Quadrant;
Quadrant constexpr QUADRANT;

// --------------------------------- DAC Channels --------------------------------------------------

/**
 * The i2C addresses of the two 4-channel digital to analog converters.
 */
uint8_t const DAC_1_I2C_ADDRESS = 0x60;
uint8_t const DAC_2_I2C_ADDRESS = 0x61;

/**
 * The four channels of an MCP4728 DAC arranged as an array for the sake of syntactic sugar.
 * Do not use this array directly. Use setChannel() instead.
 * Worth noting that these are just constants that are reused across the two DAC instqnces.
 */
MCP4728_channel_t const DAC_CHANNELS[] = {
  MCP4728_CHANNEL_A,
  MCP4728_CHANNEL_B,
  MCP4728_CHANNEL_C,
  MCP4728_CHANNEL_D
};

#endif
