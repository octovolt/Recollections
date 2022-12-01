/**
 * @file
 *
 * Recollections: a voltage memory Eurorack module
 *
 * Copyright 2022 William Edward Fisher.
 *
 * Target platform: Teensy 3.6 and Teensy 4.1.
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
#if defined(ARDUINO_TEENSY32) || defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#include <i2c_t3.h> // for Teensy 3.x
#else
#include <Wire.h> // for Teensy 4.x (and Arduino)
#endif

#include "typedefs.h"

#include <Adafruit_MCP4728.h>
#include <Adafruit_NeoTrellis.h>
#include <ArduinoJson.h>
#include <Entropy.h>
#include <SD.h>
#include <SPI.h>

#include "Config.h"
#include "Grid.h"
#include "Hardware.h"
#include "Nav.h"
#include "State.h"
#include "Utils.h"
#include "constants.h"

// State instance. Initial values provided in setup().
State state;

bool initialLoop = 1;

////////////////////////////////////////// KEY EVENTS //////////////////////////////////////////////

/**
 * @brief Callback for key presses
 *
 * @param evt The key event, a struct.
 */
TrellisCallback handleKeyEvent(keyEvent evt) {
  state = Grid::handleKeyEvent(evt, state);
  return 0;
}

//////////////////////////////////// HANDLE MOD BUTTON /////////////////////////////////////////////

void handleModButton() {
  // When MOD_INPUT is low, the button is being pressed.
  // We have a debounce scheme here with the readyForModPress flag. Once the button is pressed, we
  // say we are not readyForModPress until the button is released and the debounce time has elapsed.
  if (state.readyForModPress && !digitalRead(MOD_INPUT)) {
    state.readyForModPress = 0;
    state.timeModPressed = millis();
  }
  // When MOD_INPUT is high, the button is no longer being pressed.
  // When the button is not being pressed, but we are still not ready for a new press, and the
  // debounce time has elapsed, we then clear the readyForModPress state to become ready for a new
  // button press. Note that if millis() overflows and starts over at zero, this will be treated as
  // if the debounce time has elapsed. In theory, this would only happen if the program was running
  // for over 50 days.
  else if (
    !state.readyForModPress && digitalRead(MOD_INPUT) &&
    ((millis() - state.timeModPressed > MOD_DEBOUNCE_TIME) ||
     millis() < state.timeModPressed)
  ) {
    if (state.initialKeyPressedDuringModHold >= 0) {
      state.initialKeyPressedDuringModHold = -1;
      state.keyPressesSinceModHold = 0;
      if (state.selectedKeyForCopying >= 0) {
        pasteFromCopyAction();
      }
    }
    else if (state.screen == SCREEN.STEP_SELECT) {
      state = Nav::goForward(state, SCREEN.SECTION_SELECT);
    }
    else {
      state = Nav::goBack(state);
    }
    state.readyForModPress = 1;
  }
  // MOD button long press in STEP_SELECT screen navigates to STEP_CHANNEL_SELECT screen.
  if (
    state.screen == SCREEN.STEP_SELECT &&
    state.readyForModPress == 0 &&
    state.initialKeyPressedDuringModHold < 0 &&
    millis() - state.timeModPressed > LONG_PRESS_TIME
  ) {
    state.initialKeyPressedDuringModHold = 69; // faking this to prevent immediate navigation back
    state = Nav::goForward(state, SCREEN.STEP_CHANNEL_SELECT);
  }
}

void pasteFromCopyAction() {
  if (state.selectedKeyForCopying < 0) {
    Serial.printf("%s %u \n", "selectedKeyForCopying is unexpectedly", state.selectedKeyForCopying);
    return;
  }
  switch (state.screen) {
    case SCREEN.BANK_SELECT:
      state = State::pasteBanks(state);
      break;
    case SCREEN.EDIT_CHANNEL_SELECT:
      state = State::pasteChannels(state);
      break;
    case SCREEN.EDIT_CHANNEL_VOLTAGES:
      state = State::pasteChannelStepVoltages(state);
      break;
    case SCREEN.GLOBAL_EDIT:
      state = State::pasteGlobalSteps(state);
      break;
  }
  state.selectedKeyForCopying = -1;
}

////////////////////////////////////////// CHANGE STEP /////////////////////////////////////////////

/**
 * @brief Change the current step to the provided step index.
 *
 * @param newStep Index of the new step.
 */
void changeStep(uint8_t newStep) {
  if (newStep > 15) {
    state.currentStep = 0; // to do: error handling
    return;
  }
  state.currentStep = newStep;

  // Below we handle skipping the current step, if needed.
  //
  // WARNING! ACHTUNG! PELIGRO!
  //
  // Make sure to prevent an infinite loop before calling advanceStep()! We cannot allow all steps
  // to be removed, but if somehow they are, do not call advanceStep().
  bool allStepsRemoved = 1;
  for (u_int8_t i = 0; i < 16; i++) {
    if (!state.removedSteps[i]) {
      allStepsRemoved = 0;
      break;
    }
  }
  if (!allStepsRemoved && state.removedSteps[state.currentStep]) {
    advanceStep();
  }
}

/**
 * @brief Change the current step to the next step and calculate the expected gate length.
 */
void advanceStep() {
  // manage gate length
  unsigned long timeNow = millis();
  if (timeNow - state.lastAdvReceived > 0) {
    state.gateMillis = static_cast<unsigned long>((timeNow - state.lastAdvReceived) / 2);
  }
  state.lastAdvReceived = timeNow;

  if (state.currentStep == 15) {
    changeStep(0);
  }
  else {
    changeStep(state.currentStep + 1);
  }
}

////////////////////////////////////// SETUP AND LOOP  /////////////////////////////////////////////

/**
 * @brief Initializes and begins communication with the SD card.
 *
 * @return true
 * @return false
 */
bool setupSDCard() {
  delay(200); // Seems to work better, not sure why
  Serial.println("Attempting to open SD card");
  if (!SD.begin(SD_CS_PIN)) { // update if not using a built-in SD card on a Teensy
    Serial.println("SD card failed, or not present");
    return false;
  } else {
    Serial.println("SD card initialized.");
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
  state.config.randomOutputOverwritesSteps = 1;

  // overwrite defaults if anything is in the Config.txt file
  state.config = Config::readConfigFromSDCard(state.config);
  return true;
}

/**
 * @brief Initializes and begins communication with the DACs and the NeoTrellis.
 *
 * @return true
 * @return false
 */
bool setupPeripheralHardware() {
  // Make sure peripherals are powered up fully before attempting to talk to them.
  delay(100);

  // DACs
  Serial.println("set up hardware");
  if (state.config.dac1.begin(MCP4728_I2CADDR_DEFAULT)) { // 0x60
    Serial.println("dac1 began successfully");
  } else {
    Serial.println("dac1 did not begin successfully");
    return false;
  }
  if (state.config.dac2.begin(0x61)) {
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
  Serial.println("set up state");

  // ephemeral state
  if (state.screen != SCREEN.ERROR) {
    state.screen = SCREEN.STEP_SELECT;
  }
  state.flash = 1;
  state.flashesSinceRandomColorChange = 0;
  state.initialKeyPressedDuringModHold = -1;
  state.keyPressesSinceModHold = 0;
  state.lastFlashToggle = 0;
  state.navHistoryIndex = 0;
  state.randomColorShouldChange = 1;
  state.readyForAdvInput = 1;
  state.readyForKeyPress = 1;
  state.readyForModPress = 1;
  state.readyForRecInput = 1;
  state.selectedKeyForCopying = -1;
  state.selectedKeyForRecording = -1;
  for (uint8_t i = 0; i < 16; i++) {
    if (i < 4) {
      state.navHistory[i] = SCREEN.STEP_SELECT;
    }
    state.pasteTargetKeys[i] = 0;
  }
  Serial.println("Successfully set up transient state");

  // persisted state
  state = State::readModuleFromSDCard(state);
  Serial.println("Successfully set up persisted state");

  return true;
}

/**
 * @brief Runs on power up and prior to loop()
 */
void setup() {
  Serial.begin(9600);
  // while (!Serial);

  Entropy.Initialize();

  pinMode(ADV_INPUT, INPUT);
  pinMode(MOD_INPUT, INPUT);
  pinMode(REC_INPUT, INPUT);
  pinMode(TRELLIS_INTERRUPT_INPUT, INPUT);
  pinMode(TEENSY_LED, OUTPUT);

  bool setUpSDCardSuccessfully = setupSDCard();
  if (!setUpSDCardSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  bool setUpConfigSuccessfully = setupConfig();
  if (!setUpConfigSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  bool setUpHardwareSuccessfully = setupPeripheralHardware();
  if (!setUpHardwareSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  bool setUpStateSuccessfully = setupState();
  if (!setUpStateSuccessfully) {
    state.screen = SCREEN.ERROR;
  }

  digitalWrite(TEENSY_LED, 1); // to indicate that the teensy is alive and well

  Serial.println("Completed set up");
}

/**
 * @brief Runs repeatedly; main execution loop of the entire program.
 *
 * Please note that the order of operations here is important.
 */
void loop() {
  uint8_t currentBank = state.currentBank;
  uint8_t currentStep = state.currentStep;
  uint8_t currentChannel = state.currentChannel;
  unsigned long ms = millis();

  //----------------------------- HANDLE TRELLIS KEY EVENTS ----------------------------------------
  if (!digitalRead(TRELLIS_INTERRUPT_INPUT)) {
    state.config.trellis.read(false);
  }

  //------------------------------------ FLASH TIMING ----------------------------------------------
  state.randomColorShouldChange = 0;
  if (
    ms - state.lastFlashToggle > FLASH_TIME
  ) {
    state.flashesSinceRandomColorChange += 1;
    if (state.flashesSinceRandomColorChange > 1) {
      state.flashesSinceRandomColorChange = 0;
      state.randomColorShouldChange = 1;
    }
    state.flash = !state.flash;
    state.lastFlashToggle = ms;
  }

  // ----------------------------- ERROR SCREEN RETURNS EARLY --------------------------------------
  if (state.screen == SCREEN.ERROR) {
    Hardware::reflectState(state);
    return;
  }

  //--------------------------------- HANDLE MOD BUTTON --------------------------------------------
  handleModButton();

  //--------------------------------- HANDLE ADV INPUT ---------------------------------------------
  if (state.readyForAdvInput && !digitalRead(ADV_INPUT)) {
    state.readyForAdvInput = 0;

    if (state.config.randomOutputOverwritesSteps) {
      // set random output voltages of next step before advancing
      for (uint8_t i = 0; i < 7; i++) {
        // random channels, random 32-bit converted to 10-bit
        if (state.randomOutputChannels[currentBank][i]) {
          state.voltages[currentBank][currentStep + 1][i] = Entropy.random(MAX_UNSIGNED_10_BIT);
        }

        if (state.randomSteps[currentBank][currentStep + 1][i]) {
          // random gate steps
          if (state.gateChannels[currentBank][i]) {
            state.voltages[currentBank][currentStep + 1][i] = Entropy.random(2)
              ? VOLTAGE_VALUE_MAX
              : 0;
          }
          // random CV steps, random 32-bit converted to 10-bit
          state.voltages[currentBank][currentStep + 1][i] = Entropy.random(MAX_UNSIGNED_10_BIT);
        }
      }
    }

    advanceStep();
  }
  else if (!state.readyForAdvInput && digitalRead(ADV_INPUT)) {
    state.readyForAdvInput = 1;
  }

  //--------------------------------- HANDLE REC INPUT ---------------------------------------------
  if (state.readyForRecInput && !digitalRead(REC_INPUT)) {
    state.readyForRecInput = 0;
    for (uint8_t i = 0; i < 7; i++) {
      if (
        state.autoRecordChannels[currentBank][i] &&
        !state.lockedVoltages[currentBank][currentStep][i]
      ) {
        state.voltages[currentBank][currentStep][i] = state.randomInputChannels[currentBank][i]
          ? Entropy.random(MAX_UNSIGNED_10_BIT)
          : analogRead(CV_INPUT);
      }
    }
  }
  else if (!state.readyForRecInput && digitalRead(REC_INPUT)) {
    state.readyForRecInput = 1;
  }

  //------------------------ EDITING OR RECORDING VOLTAGE CONTINUOUSLY -----------------------------
  if (state.selectedKeyForRecording >= 0) {
    if (state.screen == SCREEN.EDIT_CHANNEL_VOLTAGES || state.screen == SCREEN.STEP_SELECT) {
      state.voltages[currentBank][state.selectedKeyForRecording][currentChannel] = analogRead(CV_INPUT);
    }
    else if (
      state.screen == SCREEN.RECORD_CHANNEL_SELECT &&
      !state.lockedVoltages[currentBank][currentStep][state.selectedKeyForRecording] &&
      !state.autoRecordChannels[currentBank][currentStep] // automatic recording is sample and hold
    ) {
      state.voltages[currentBank][currentStep][state.selectedKeyForRecording] = analogRead(CV_INPUT);
    }
  }

  //------------------------------------- REFLECT STATE --------------------------------------------
  if (initialLoop) {
    Serial.println("attempting initial rendering of state");
  }
  if (!Hardware::reflectState(state)) {
    state.screen = SCREEN.ERROR;
  }

  //--------------------------------- INITIAL LOOP COMPLETED ---------------------------------------
  if (initialLoop) {
    Serial.println("initial loop completed");
  }
  initialLoop = 0;
}
