/**
 * Recollections: Constants
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_MCP4728.h>
#include <SD.h>
#include <SPI.h>

#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_CONSTANTS_H_
#define VOLTAGE_MEMORY_CONSTANTS_H_

#define MAX_UNSIGNED_32_BIT 4294967295
#define MAX_UNSIGNED_12_BIT 4095
#define MAX_UNSIGNED_10_BIT 1023

#define WHITE 0xFFFFFF0
#define RED 0x5500000
#define GREEN 0x0055000
#define BLUE 0x0000770
#define YELLOW 0x7777000
#define PURPLE 0x3300FF0
#define ORANGE 0x7733000

#define DEFAULT_BRIGHTNESS 100
#define COLOR_VALUE_MAX 255 // max brightness, relative to brightness setting 

// All of these value are based on 10-bit, because the resolution of the ADC is 10-bit. 
// These are stepped up to 12-bit for output in real time.
#define VOLTAGE_VALUE_MAX 1023
#define VOLTAGE_VALUE_MID 511
#define PERCENTAGE_MULTIPLIER_10_BIT 0.000977517106549 // 1/1023 for 10-bit

#define PERCENTAGE_MULTIPLIER_32_BIT 0.0000000004656613 // 1/2147483647 for 32-bit

#define MOD_DEBOUNCE_TIME 200
#define FLASH_TIME 120
#define LONG_PRESS_TIME 2000
#define WRITE_AFTER_KEY_RELEASE_TIME 3000

// Calculated with https://arduinojson.org/v6/assistant
#define CONFIG_JSON_DOC_DESERIALIZATION_SIZE 768 // 570 required
#define MODULE_JSON_DOC_DESERIALIZATION_SIZE 512 // 444 required
#define BANK_JSON_DOC_DESERIALIZATION_SIZE 12288 // 9801 required
#define CONFIG_JSON_DOC_SERIALIZATION_SIZE 512 // 496 required
#define MODULE_JSON_DOC_SERIALIZATION_SIZE 384 // 352 required
#define BANK_JSON_DOC_SERIALIZATION_SIZE 12288 // 9712 required

// For random number generation on Teensy 3.6.
// See: https://forum.pjrc.com/threads/48745-Teensy-3-6-Random-Number-Generator
// See also setup() in VoltageMemory.ino and Utils::random().
#define RNG_CR_GO_MASK 0x1u
#define RNG_CR_HA_MASK 0x2u
#define RNG_CR_INTM_MASK 0x4u
#define RNG_CR_CLRI_MASK 0x8u
#define RNG_CR_SLP_MASK 0x10u
#define RNG_SR_OREG_LVL_MASK 0xFF00u
#define RNG_SR_OREG_LVL_SHIFT 8
#define RNG_SR_OREG_LVL(x) (((uint32_t)(((uint32_t)(x))<<RNG_SR_OREG_LVL_SHIFT))&RNG_SR_OREG_LVL_MASK)
#define SIM_SCGC6_RNGA ((uint32_t)0x00000200)

#define CONFIG_SD_PATH "Recollections/Config.txt"
#define MODULE_SD_PATH_PREFIX "Recollections/Module_"

// Use built-in SD for SPI modes on Teensy 3.5/3.6.
// Teensy 4.0 use first SPI port.
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Different ways to open files on the SD card
uint8_t const SD_READ_CREATE = (uint8_t)(O_READ | O_CREAT);

// Analog inputs
/** Control voltage input to be recorded. */
uint8_t const CV_INPUT = A6; // (originally A0? pin "14" on Teensy 3.6)

// Digital inputs - REMEMBER that pins on the left side of Teensy start with GND and only then begin 
// counting from 0, so the second pin is 0, the third pin is 1, etc.
/** Button (gate) that acts as a modifying key for other commands or an escape key to quit the current mode */
uint8_t const MOD_INPUT = 4; // could be 2 in the future?
/** Gate to advance the current step to the next step. */ 
uint8_t const ADV_INPUT = 3;
/** Gate to start/stop recording. Recording occurs while the gate is high. This only applies to automatic recording. */
uint8_t const REC_INPUT = 0; // could be 4 in the future ?
/** Gate that determines if the NeoTrellis is being pressed. This saves cycles by avoiding unnecessary polling. */
uint8_t const TRELLIS_INTERRUPT_INPUT = 5;
/** [EXPANSION ONLY] Gate to reverse the direction of step advancement. */
uint8_t const REV_INPUT = 6;
/** [EXPANSION ONLY] Gate to reset the step advancement to the first step. */
uint8_t const RESET_INPUT = 7;
/** [EXPANSION ONLY] Gate to advance the current bank to the next bank. */
uint8_t const BANK_ADV_INPUT = 16;
/** [EXPANSION ONLY] Gate to reverse the direction of bank advancement. */
uint8_t const BANK_REV_INPUT = 17;

// Digital outputs
uint8_t const TEENSY_LED = 13;

// Digital i2c outputs
// SDA is pin 18 on Teensy 3.x and 4.x
// SCL is pin 19 on Teensy 3.x and 4.x
// Please note that we need pull up resistors of about 2k to 5k Ohms for the i2C bus.

/**
 * Mode constants.
 * 
 * Please note: these are referred to as either "sections" or "screens" in the user manual. There 
 * are five major "sections": step selection, channel editing, recording, global editing, and bank 
 * selection. Some of these have additional "screens" other than their first.
 */
typedef struct Mode {
  // Step selection. 
  // This is the primary, default mode.
  // Color: white
  Mode_t STEP_SELECT = 0;

  // Select the channel on which to perform operations, such as recording while in STEP_SELECT.
  // Color: white
  Mode_t STEP_CHANNEL_SELECT = 1;

  // Intermediary mode allowing navigation to all five major modes.
  // Colors: blue, red, yellow, green
  Mode_t MODE_SELECT = 2;

  // Edit all 16 steps for a single channel, or configure channels for gates or CV.
  // Color: yellow
  Mode_t EDIT_CHANNEL_SELECT = 3;
  Mode_t EDIT_CHANNEL_VOLTAGES = 4;

  // Record voltages, either manually across the 8 channels for a single step, or for 
  // multiple steps on a single channel while the ADV input is receiving a clock/gate/trigger.
  // This is also the mode where one can set up automatic recording.
  // Color: red
  Mode_t RECORD_CHANNEL_SELECT = 5;

  // Global editing of steps, including whether steps are addressed at all when a gate/trigger/clock
  // is received at the ADV input.
  // Color: green
  Mode_t GLOBAL_EDIT = 6;

  // Select a new bank from memory. Each bank has 16 steps.
  // Color: blue
  Mode_t BANK_SELECT = 7;

  // The module has gotten into an error state
  // Color: red
  Mode_t ERROR = 8;
} Mode;
Mode constexpr MODE;

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
