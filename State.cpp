/**
 * Copyright 2022 William Edward Fisher.
 */
#include "State.h"

#include <SD.h>
#include <SPI.h>
#include <StackString.hpp> // I have not yet understood how to use cstrings
using namespace Stack;

#include "Utils.h"

/**
 * @brief Capture voltage while automatically recording.
 *
 * @param state
 * @return State
 */
State State::autoRecord(State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentStep = state.currentStep;
  for (uint8_t i = 0; i < 7; i++) {
    if (
      state.autoRecordChannels[currentBank][i] &&
      !state.lockedVoltages[currentBank][currentStep][i] &&
      !state.randomInputChannels[currentBank][i]
    ) {
      state.voltages[currentBank][currentStep][i] = analogRead(CV_INPUT);
    }
  }
  return state;
}

/**
 * @brief Capture voltage in the current loop for a user flow within Editing or Step Selection. This
 * function will record voltage on the selected STEP. Note this could be continuous recording or a
 * sample -- this function is agnostic to whether it is continuous.
 *
 * @param state
 * @return State
 */
State State::editVoltageOnSelectedStep(State state) {
  if (state.screen == SCREEN.EDIT_CHANNEL_VOLTAGES || state.screen == SCREEN.STEP_SELECT) {
    state.voltages[state.currentBank][state.selectedKeyForRecording][state.currentChannel] =
      analogRead(CV_INPUT);
  }
  return state;
}

/**
 * @brief Capture voltage in the current loop for a user flow within Recording. This function will
 * record voltage on the selected CHANNEL. Note this could be continuous recording or a sample --
 * this function is agnostic to whether it is continuous.
 *
 * @param state
 * @return State
 */
State State::recordVoltageOnSelectedChannel(State state) {
  if (
    state.screen == SCREEN.RECORD_CHANNEL_SELECT &&
    !state.lockedVoltages[state.currentBank][state.currentStep][state.selectedKeyForRecording]
  ) {
    state.voltages[state.currentBank][state.currentStep][state.selectedKeyForRecording] =
      analogRead(CV_INPUT);
  }
  return state;
}

State State::pasteBanks(State state) {
  uint8_t selectedKeyForCopying = state.selectedKeyForCopying;
  // [banks][steps][channels]
  for (uint8_t i = 0; i < 16; i++) {
    if (state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 16; j++) {
        for (uint8_t k = 0; k < 8; k++) {
          state.activeSteps[i][j][k] = state.activeSteps[selectedKeyForCopying][j][k];
          state.autoRecordChannels[i][k] = state.autoRecordChannels[selectedKeyForCopying][k];
          state.gateChannels[i][k] = state.gateChannels[selectedKeyForCopying][k];
          state.gateSteps[i][j][k] = state.gateSteps[selectedKeyForCopying][j][k];
          state.lockedVoltages[i][j][k] = state.lockedVoltages[selectedKeyForCopying][j][k];
          state.randomInputChannels[i][k] = state.randomInputChannels[selectedKeyForCopying][k];
          state.randomOutputChannels[i][k] = state.randomOutputChannels[selectedKeyForCopying][k];
          state.randomSteps[i][j][k] = state.randomSteps[selectedKeyForCopying][j][k];
          state.voltages[i][j][k] = state.voltages[selectedKeyForCopying][j][k];
        }
      }
      state.pasteTargetKeys[i] = 0;
    }
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::pasteChannels(State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t selectedKeyForCopying = state.selectedKeyForCopying;
  for (uint8_t i = 0; i < 8; i++) { // channels
    if (state.pasteTargetKeys[i]) {
      if (state.gateChannels[currentBank][selectedKeyForCopying]) {
        state.gateChannels[state.currentBank][i] = 1;
        for (uint8_t j = 0; j < 16; j++) {
          state.gateSteps[currentBank][j][i] =
            state.gateSteps[currentBank][j][selectedKeyForCopying];
        }
      }
      else {
        for (uint8_t j = 0; j < 16; j++) { // steps
          state.activeSteps[currentBank][j][i] =
            state.activeSteps[currentBank][j][state.selectedKeyForCopying];
          state.voltages[currentBank][j][i] =
            state.voltages[currentBank][j][state.selectedKeyForCopying];
        }
      }
    }
    state.pasteTargetKeys[i] = 0;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::pasteChannelStepVoltages(State state) {
  for (uint8_t i = 0; i < 16; i++) { // steps
    if (state.pasteTargetKeys[i]) {
      state.voltages[state.currentBank][i][state.currentChannel] =
        state.voltages[state.currentBank][state.selectedKeyForCopying][state.currentChannel];
    }
    state.pasteTargetKeys[i] = 0;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::pasteGlobalSteps(State state) {
  for (uint8_t i = 0; i < 16; i++) { // steps
    if (state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 8; j++) { // channels
        state.voltages[state.currentBank][i][j] =
          state.voltages[state.currentBank][state.selectedKeyForCopying][j];
      }
    }
    state.pasteTargetKeys[i] = 0;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::quitCopyPasteFlowPriorToPaste(State state) {
  state.selectedKeyForCopying = -1;
  for (uint8_t i = 0; i < 16; i++) {
    state.pasteTargetKeys[i] = 0;
  }
  return state;
}

void State::confirmOrCreatePathOnSDCard(State state) {
  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);
  StackString<100> modulePath = StackString<100>(MODULE_SD_PATH_PREFIX);
  modulePath.append(currentModuleString);

  if (!SD.exists(modulePath.c_str())) {
    if (!SD.exists("Recollections")) {
      SD.mkdir("Recollections");
    }
    SD.mkdir(modulePath.c_str());
  }
}

State State::readModuleFromSDCard(State state) {
  // First we establish defaults to make sure the data is populated, then we attempt to get data
  // from the SD card.

  // Core data -- preserved in Module.txt
  // Keep this in sync with State::readModuleFileFromSDCard().
  // If adding or removing anything here, please recalculate the size constants for the JSON
  // documents required for storing the data on the SD card. See constants.h.
  state.currentStep = 0;
  state.currentBank = 0;
  state.currentChannel = 0;
  for (uint8_t i = 0; i < 16; i++) {
    state.removedSteps[i] = 0;
  }

  // Bank data -- preserved in Bank_<bank-index>.txt
  //
  // Keep this in sync with State::readBankFromSDCard().
  // If adding or removing anything here, please recalculate the size constants for the JSON
  // documents required for storing the data on the SD card. See constants.h.
  //
  // Also keep this in sync with State::pasteBanks().
  //
  // Indices are bank, step, channel.
  for (uint8_t i = 0; i < 16; i++) {
    for (uint8_t j = 0; j < 16; j++) {
      for (uint8_t k = 0; k < 8; k++) {
        state.activeSteps[i][j][k] = 1;
        state.autoRecordChannels[i][k] = 0;
        state.gateChannels[i][k] = 0;
        state.gateSteps[i][j][k] = 0;
        state.lockedVoltages[i][j][k] = 0;
        state.randomInputChannels[i][k] = 0;
        state.randomOutputChannels[i][k] = 0;
        state.randomSteps[i][j][k] = 0;
        state.voltages[i][j][k] = VOLTAGE_VALUE_MID;
      }
    }
  }

  state = State::readModuleFileFromSDCard(state);
  for (uint8_t bank = 0; bank < 16; bank++) {
    state = State::readBankFileFromSDCard(state, bank);
  }
  return state;
}

State State::readModuleFileFromSDCard(State state) {
  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);

  // Recollections/Module_15/Module.txt
  StackString<100> modulePath = StackString<100>(MODULE_SD_PATH_PREFIX);
  modulePath.append(currentModuleString);
  modulePath.append("/Module.txt");

  File moduleFile = SD.open(modulePath.c_str(), SD_READ_CREATE);
  if (!moduleFile) {
    Serial.println("Could not open Module.txt");
    return state;
  } else {
    Serial.println("Successfully opened Module.txt");
  }

  if (moduleFile.available()) {
    Serial.println("Module file is available");
    StaticJsonDocument<MODULE_JSON_DOC_DESERIALIZATION_SIZE> doc;
    DeserializationError error = deserializeJson(doc, moduleFile);
    if (error == DeserializationError::EmptyInput) {
      Serial.println("Module.txt is an empty file");
    }
    else if (error) {
      Serial.printf("%s %s \n", "deserializeJson() failed during read operation: ", error.c_str());
    }
    else {
      state.currentStep = doc["currentStep"];
      state.currentBank = doc["currentBank"];
      state.currentChannel = doc["currentChannel"];
      copyArray(doc["removedSteps"], state.removedSteps);
    }
    moduleFile.close();
  }

  return state;
}

State State::readBankFileFromSDCard(State state, uint8_t bank) {
  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);

  int bankLength = snprintf(NULL, 0, "%d", bank) + 1;
  char bankString[bankLength];
  sprintf(bankString, "%d", bank);

  // Recollections/Module_15/Bank_0.txt
  StackString<100> bankPath = StackString<100>(MODULE_SD_PATH_PREFIX);
  bankPath.append(currentModuleString);
  bankPath.append("/Bank_");
  bankPath.append(bankString);
  bankPath.append(".txt");

  File bankFile = SD.open(bankPath.c_str(), SD_READ_CREATE);

  if (!bankFile) {
    Serial.printf("%s%s%s\n", "Could not open Bank_", bankString, ".txt");
    return state;
  } else {
    Serial.printf("%s%s%s\n", "Successfully opened Bank_", bankString, ".txt");
  }

  if (bankFile.available()) {
    Serial.println("Bank file is available");
    StaticJsonDocument<BANK_JSON_DOC_DESERIALIZATION_SIZE> doc;
    DeserializationError error = deserializeJson(doc, bankFile);
    if (error == DeserializationError::EmptyInput) {
      Serial.println("Module.txt is an empty file");
    }
    else if (error) {
      Serial.printf("%s %s \n", "deserializeJson() failed during read operation: ", error.c_str());
    }
    else {
      copyArray(doc["activeSteps"], state.activeSteps[bank]);
      copyArray(doc["autoRecordChannels"], state.autoRecordChannels[bank]);
      copyArray(doc["gateChannels"], state.gateChannels[bank]);
      copyArray(doc["gateSteps"], state.gateSteps[bank]);
      copyArray(doc["lockedVoltages"], state.lockedVoltages[bank]);
      copyArray(doc["randomInputChannels"], state.randomInputChannels[bank]);
      copyArray(doc["randomOutputChannels"], state.randomOutputChannels[bank]);
      copyArray(doc["randomSteps"], state.randomSteps[bank]);
      copyArray(doc["voltages"], state.voltages[bank]);
    }
  }
  bankFile.close();

  return state;
}

bool State::writeCurrentModuleAndBankToSDCard(State state) {
  Serial.println("writing to SD card");

  State::confirmOrCreatePathOnSDCard(state);

  // TODO: I would like to abstract a lot of this into a function, since a lot of lines are repeated
  // twice here and in the other SD card functions, but I am having trouble figuring out how to do
  // this with cstrings in a way that makes sense and actually saves lines of code. Something to
  // improve upon later.

  // --------------------------- Module file ---------------------------------

  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);

  // Recollections/Module_15/Module.txt
  StackString<100> modulePath = StackString<100>(MODULE_SD_PATH_PREFIX);
  modulePath.append(currentModuleString);
  modulePath.append("/Module.txt");

  File moduleFile = SD.open(modulePath.c_str(), FILE_WRITE_BEGIN);

  if (!moduleFile) {
    Serial.println("Could not open Module.txt");
    return false;
  } else {
    Serial.println("Successfully opened Module.txt");
  }

  moduleFile.truncate();

  StaticJsonDocument<MODULE_JSON_DOC_SERIALIZATION_SIZE> moduleDoc;
  moduleDoc["currentBank"] = state.currentBank;
  moduleDoc["currentChannel"] = state.currentChannel;
  moduleDoc["currentStep"] = state.currentStep;
  for (uint8_t i = 0; i < 16; i++) {
    moduleDoc["removedSteps"][i] = state.removedSteps[i];
  }

  WriteBufferingStream writeBufferingStream(moduleFile, 64);
  size_t charsWritten = serializeJson(moduleDoc, writeBufferingStream);
  writeBufferingStream.flush();
  moduleFile.close();
  if (charsWritten == 0) {
    Serial.println("Failed to write any chars to SD card");
    return false;
  } else {
    Serial.printf("%s %u \n", "chars written: ", charsWritten);
  }

  // --------------------------- Bank file -------------------------------------

  uint8_t bank = state.currentBank;

  int bankLength = snprintf(NULL, 0, "%d", bank) + 1;
  char bankString[bankLength];
  sprintf(bankString, "%d", bank);

  // Recollections/Module_15/Bank_0.txt
  StackString<100> bankPath = StackString<100>(MODULE_SD_PATH_PREFIX);
  bankPath.append(currentModuleString);
  bankPath.append("/Bank_");
  bankPath.append(bankString);
  bankPath.append(".txt");

  File bankFile = SD.open(bankPath.c_str(), FILE_WRITE_BEGIN);
  if (!bankFile) {
    Serial.printf("%s%s%s\n", "Could not open Bank_", bankString, ".txt");
    return false;
  } else {
    Serial.printf("%s%s%s\n", "Successfully opened Bank_", bankString, ".txt");
  }

  bankFile.truncate();

  StaticJsonDocument<BANK_JSON_DOC_SERIALIZATION_SIZE> bankDoc;
  JsonObject bankRoot = bankDoc.to<JsonObject>();
  JsonArray autoRecordChannels = bankRoot.createNestedArray("autoRecordChannels");
  JsonArray gateChannels = bankRoot.createNestedArray("gateChannels");
  JsonArray randomInputChannels = bankRoot.createNestedArray("randomInputChannels");
  JsonArray randomOutputChannels = bankRoot.createNestedArray("randomOutputChannels");
  for (uint8_t i = 0; i < 8; i++) {
    autoRecordChannels.add(state.autoRecordChannels[bank][i]);
    gateChannels.add(state.gateChannels[bank][i]);
    randomInputChannels.add(state.randomInputChannels[bank][i]);
    randomOutputChannels.add(state.randomOutputChannels[bank][i]);
  }
  JsonArray activeSteps = bankRoot.createNestedArray("activeSteps");
  JsonArray gateSteps = bankRoot.createNestedArray("gateSteps");
  JsonArray lockedVoltages = bankRoot.createNestedArray("lockedVoltages");
  JsonArray randomSteps = bankRoot.createNestedArray("randomSteps");
  JsonArray voltages = bankRoot.createNestedArray("voltages");
  for (uint8_t i = 0; i < 16; i++) {
    JsonArray activeStepsChannelArray = activeSteps.createNestedArray();
    JsonArray gateStepsChannelArray = gateSteps.createNestedArray();
    JsonArray lockedVoltagesChannelArray = lockedVoltages.createNestedArray();
    JsonArray randomStepsChannelArray = randomSteps.createNestedArray();
    JsonArray voltagesChannelArray = voltages.createNestedArray();
    for (uint8_t j = 0; j < 8; j++) {
      activeStepsChannelArray.add(state.activeSteps[bank][i][j]);
      gateStepsChannelArray.add(state.gateSteps[bank][i][j]);
      lockedVoltagesChannelArray.add(state.lockedVoltages[bank][i][j]);
      randomStepsChannelArray.add(state.randomSteps[bank][i][j]);
      voltagesChannelArray.add(state.voltages[bank][i][j]);
    }
  }

  WriteBufferingStream writeBankBufferingStream(bankFile, 64);
  size_t bankCharsWritten = serializeJson(bankDoc, writeBankBufferingStream);
  writeBankBufferingStream.flush();
  bankFile.close();
  if (bankCharsWritten == 0) {
    Serial.println("Failed to write any chars to SD card");
    return false;
  } else {
    Serial.printf("%s %u \n", "chars written: ", bankCharsWritten);
  }

  return true;
}
