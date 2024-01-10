/**
 * @file
 *
 * Recollections: a voltage memory Eurorack module
 *
 * Copyright 2022 William Edward Fisher.
 *
 * Target platforms: Teensy 3.6, Teensy 4.1
 * and Raspberry Pi Pico connected to an external SD card holder.
 */

// Teensy 4.x uses Wire.h, but Teensy 3.x uses i2c_t3.h.
//
// Include directives in the following files must be updated in the same way as they are handled in
// the conditional statment below.
//
// Adafruit_seesaw.h
// Adafruit_I2CDevice.h <- this also needs the following at the top for 3.6: typedef i2c_t3 TwoWire;
// Adafruit_MCP4728.h
// Adafruit_MCP4728.cpp
#if defined(ARDUINO_TEENSY36)
  #include <i2c_t3.h> // ~/Documents/Arduino/libraries/
#else
  // for Teensy 4.x and Pico
  #include <Wire.h> // /Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/
#endif

// Without Adafruit, this project never would have happened. Please buy things from Adafruit.
#include <Adafruit_MCP4728.h>
#include <Adafruit_NeoTrellis.h>

// JSON is used to store data on the SD card
// https://arduinojson.org/
#include <ArduinoJson.h>

#ifdef CORE_TEENSY
  // Random number generation:
  // On Teensy (or any AVR processor), we will use the Entropy library. On RP2040, we will need to
  // settle for the pseudorandomness of stdlib's srand() seeded from a noisy unconnected pin
  // and rand().
  // Entropy is included with Teenyduino and is found in
  // /Applications/Teensyduino.app/Contents/Java/hardware/teensy/avr/libraries/
  // Entropy source code: https://code.google.com/archive/p/avr-hardware-random-number-generation/
  #include <Entropy.h>
#else
  #include <stdlib.h> // for srand()
#endif

#include <SD.h>
#ifndef CORE_TEENSY // Not Teensy, so this is Pico
  #include <SPI.h>
#endif

#include "Config.h"
#include "Keys.h"
#include "Hardware.h"
#include "Input.h"
#include "Nav.h"
#include "SDCard.h"
#include "State.h"
#include "Utils.h"
#include "constants.h"
#include "typedefs.h"

// State instance. Initial values provided in setup().
State state;

////////////////////////////////////////// KEY EVENTS //////////////////////////////////////////////

/**
 * @brief Callback for key presses
 *
 * @param evt The key event, a struct.
 */
TrellisCallback handleKeyEvent(keyEvent evt) {
  state = Keys::handleKeyEvent(evt, state);
  return 0;
}

////////////////////////////////////// SETUP AND LOOP  /////////////////////////////////////////////

/**
 * @brief Initializes and begins communication with the SD card.
 *
 * @return true
 * @return false
 */
bool setupSDCard() {
  delay(200); // Seems to work better on Teensy, not sure why

  Serial.println("Attempting to open SD card");
  bool success = false;

  #ifdef CORE_TEENSY
    success = SD.begin(SD_CS_PIN);
  #else // Not Teensy, so this is Pico
    success = SD.begin(RECOLLECTIONS_SPI_CSN, 24000000); // 24 MHz
  #endif

  if (!success) {
    Serial.println("SD card failed, or is not present");
    return false;
  } else {
    Serial.println("SD card is working.");
  }
  return true;
}

/**
 * @brief Initializes the config struct, based on the config file in the SD card.
 *
 * @return true
 * @return false
 */
bool setupConfig() {
  // default values
  state.config.brightness = DEFAULT_BRIGHTNESS;
  state.config.colors = {
    .white = {255,255,255},
    .red = {85,0,0},
    .blue = {0,0,119},
    .yellow = {119,119,0},
    .green = {0,85,0},
    .purple = {51,0,255},
    .orange = {119,51,0},
    .magenta = {119,0,119},
    .black = {0,0,0},
  };
  state.config.controllerOrientation = 1;
  state.config.currentModule = 0;
  state.config.isAdvancingMaxInterval = 10000;
  state.config.isClockedTolerance = 0.1;
  state.config.randomOutputOverwrites = 1;

  // overwrite defaults if anything is in the Config.txt file
  if (REQUIRE_SD_CARD) {
    state.config = SDCard::readConfigFile(state.config);
  }

  return true;
}

/**
 * @brief Initializes and begins communication with the DACs and the NeoTrellis.
 *
 * @return true
 * @return false
 */
bool setupPeripheralHardware() {
  // Give peripherals time to boot up fully before attempting to talk to them.
  delay(100);

  // DACs
  // In hardware before version 0.4.0, the USB port is only accessible by removing dac1.
  if (!(USB_POWERED && (HARDWARE_SEMVER.compare("0.4.0") < 0))) {
    if (state.config.dac1.begin(DAC_1_I2C_ADDRESS, &Wire)) {
      Serial.println("dac1 began successfully");
    } else {
      Serial.println("dac1 did not begin successfully");
      return false;
    }
  }
  if (state.config.dac2.begin(DAC_2_I2C_ADDRESS, &Wire)) {
    Serial.println("dac2 began successfully");
  } else {
    Serial.println("dac2 did not begin successfully");
    return false;
  }

  // NeoTrellis elastomer keys
  if (state.config.trellis.begin(0x2E)) {
    Serial.println("NeoTrellis began successfully");
  } else {
    Serial.println("NeoTrellis did not begin successfully");
    return false;
  }

  // Reduce brightness for lower power consumption
  state.config.trellis.pixels.setBrightness(state.config.brightness);

  for(uint8_t i = 0; i < NEO_TRELLIS_NUM_KEYS; i++){
    state.config.trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    state.config.trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    state.config.trellis.registerCallback(i, handleKeyEvent);
  }
  Serial.println("NeoTrellis fully activated");

  return true;
}

/**
 * @brief Sets up the state object, where all application state is stored. This method also reads
 * from the SD card to recreate that state the module was using prior to the last power down.
 *
 * @return true
 * @return false
 */
bool setupState() {
  // ephemeral state
  if (state.screen != SCREEN.ERROR) {
    state.screen = SCREEN.PRESET_SELECT;
  }
  state.advanceBankAddend = 1;
  state.advancePresetAddend = 1;
  state.flash = true;
  state.flashesSinceRandomColorChange = 0;
  state.initialLoopCompleted = false;
  state.initialModHoldKey = -1;
  state.keyPressesSinceModHold = 0;
  state.lastFlashToggle = 0;
  state.navHistoryIndex = 0;
  state.randomColorShouldChange = true;
  state.readyForAdvInput = true;
  state.readyForBankAdvanceInput = true;
  state.readyForBankReverseInput = true;
  state.readyForKeyPress = true;
  state.readyForModPress = true;
  state.readyForRecInput = true;
  state.readyForResetInput = true;
  state.readyForReverseInput = true;
  state.readyForPresetSelection = false;
  state.selectedKeyForCopying = -1;
  state.selectedKeyForRecording = -1;
  for (uint8_t i = 0; i < 16; i++) {
    if (i < 4) {
      state.navHistory[i] = SCREEN.PRESET_SELECT;
    }
    state.pasteTargetKeys[i] = false;
  }
  Serial.println("Successfully set up transient state");

  // default state

  // Core data -- preserved in Module.txt
  // Keep this in sync with SDCard::readModuleFile().
  // If adding or removing anything here, please recalculate the size constants for the JSON
  // documents required for storing the data on the SD card. See constants.h.
  state.currentPreset = 0;
  state.currentBank = 0;
  state.currentChannel = 0;
  for (uint8_t i = 0; i < 16; i++) {
    state.removedPresets[i] = false;
  }

  // Bank data -- preserved in Bank_<bank-index>.txt
  //
  // Keep this in sync with State::readBankFromSDCard().
  // If adding or removing anything here, please recalculate the size constants for the JSON
  // documents required for storing the data on the SD card. See constants.h.
  //
  // Also keep this in sync with State::pasteBanks().
  //
  // Indices are bank, preset, channel.
  for (uint8_t i = 0; i < 16; i++) {
    for (uint8_t j = 0; j < 16; j++) {
      for (uint8_t k = 0; k < 8; k++) {
        state.activeVoltages[i][j][k] = true;
        state.autoRecordChannels[i][k] = false;
        state.gateChannels[i][k] = false;
        state.gateVoltages[i][j][k] = false;
        state.lockedVoltages[i][j][k] = false;
        state.randomInputChannels[i][k] = false;
        state.randomOutputChannels[i][k] = false;
        state.randomVoltages[i][j][k] = false;
        state.voltages[i][j][k] = VOLTAGE_VALUE_MID;
      }
    }
  }
  Serial.println("Successfully set up default state");

  // persisted state

  if (REQUIRE_SD_CARD) {
    state = SDCard::readModuleDirectory(state);
    Serial.println("Successfully set up persisted state");
  }

  return true;
}

/**
 * @brief Runs on power up and prior to loop()
 */
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting set up");

  // Set up the local I2C bus where the MCU is the controller/leader.
  // This communicats with the NeoTrellis elastomer keys and the MCP4728 DACs.
  // On Teensy, I2C is on pins 18 and 19 (Wire's default), on Pico it is on pins 4 and 5.
  #ifndef CORE_TEENSY // Not Teensy, so this is Pico
    analogReadResolution(12); // 12-bit ADC on the Pico
    Wire.setSDA(RECOLLECTIONS_SDA0);
    Wire.setSCL(RECOLLECTIONS_SCL0);
  #endif
  Wire.begin();

  pinMode(ADV_INPUT, INPUT);
  pinMode(MOD_INPUT, INPUT);
  pinMode(REC_INPUT, INPUT);
  pinMode(TRELLIS_INTERRUPT_INPUT, INPUT);
  pinMode(BOARD_LED, OUTPUT);

  if (REQUIRE_SD_CARD) {
    if (!setupSDCard()) {
      state.screen = SCREEN.ERROR;
    }
  }

  bool setUpConfigSuccessfully = setupConfig();
  if (!setUpConfigSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  // bool setUpHardwareSuccessfully = setupPeripheralHardware();
  // if (!setUpHardwareSuccessfully) {
  //   state.screen = SCREEN.ERROR;
  // }

  bool setUpStateSuccessfully = setupState();
  if (!setUpStateSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  #ifdef CORE_TEENSY
    Entropy.Initialize();
  #else
    srand(analogRead(UNCONNECTED_ANALOG_PIN));
  #endif

  digitalWrite(BOARD_LED, 1); // to indicate that the microcontroller is alive and well

  Serial.println("Completed set up");
}

/**
 * @brief Runs repeatedly; main execution loop of the entire program.
 *
 * Please note that the order of operations here is important.
 */
void loop() {
  // unsigned long loopStartTime = millis();

  // state = Hardware::updateFlashTiming(loopStartTime, state);

  // // error screen returns early
  // if (state.screen == SCREEN.ERROR) {
  //   Hardware::reflectState(state);
  //   return;
  // }

  // // Handle key events, inputs and recording.
  // // These drive all of the state changes other than flash timing.
  // if (!digitalRead(TRELLIS_INTERRUPT_INPUT)) {
  //   state.config.trellis.read(false);
  // }
  // state = State::recordContinuously(Input::handleInput(loopStartTime, state));

  // // reflect state
  // if (!Hardware::reflectState(state)) {
  //   state.screen = SCREEN.ERROR;
  // }

  // initial loop completed -- this is for debugging only. TODO: remove.
  if (!state.initialLoopCompleted) {
    Serial.println("--- Initial loop completed ---");
    state.initialLoopCompleted = true;
  }
}
