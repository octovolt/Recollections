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
  uint8_t currentPreset = state.currentPreset;
  for (uint8_t i = 0; i < 7; i++) {
    if (
      state.autoRecordChannels[currentBank][i] &&
      !state.lockedVoltages[currentBank][currentPreset][i] &&
      !state.randomInputChannels[currentBank][i]
    ) {
      state.voltages[currentBank][currentPreset][i] = analogRead(CV_INPUT);
    }
  }
  return state;
}

/**
 * @brief Capture voltage in the current loop for a user flow within Editing or Preset Selection. This
 * function will record voltage on the selected PRESET for the current channel. Note this could be
 * continuous recording or a sample -- this function is agnostic to whether it is continuous.
 *
 * @param state
 * @return State
 */
State State::editVoltageOnSelectedPreset(State state) {
  if (state.screen == SCREEN.EDIT_CHANNEL_VOLTAGES || state.screen == SCREEN.PRESET_SELECT) {
    state.voltages[state.currentBank][state.selectedKeyForRecording][state.currentChannel] =
      analogRead(CV_INPUT);
  }
  return state;
}

/**
 * @brief Capture voltage in the current loop for a user flow within Recording. This function will
 * record voltage on the selected CHANNEL for the current preset. Note this could be continuous
 * recording or a sample -- this function is agnostic to whether it is continuous.
 *
 * @param state
 * @return State
 */
State State::recordVoltageOnSelectedChannel(State state) {
  if (
    state.screen == SCREEN.RECORD_CHANNEL_SELECT &&
    !state.lockedVoltages[state.currentBank][state.currentPreset][state.selectedKeyForRecording]
  ) {
    state.voltages[state.currentBank][state.currentPreset][state.selectedKeyForRecording] =
      analogRead(CV_INPUT);
  }
  return state;
}

State State::pasteBanks(State state) {
  uint8_t selectedKeyForCopying = state.selectedKeyForCopying;
  // [banks][presets][channels]
  for (uint8_t i = 0; i < 16; i++) {
    if (state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 16; j++) {
        for (uint8_t k = 0; k < 8; k++) {
          state.activeVoltages[i][j][k] = state.activeVoltages[selectedKeyForCopying][j][k];
          state.autoRecordChannels[i][k] = state.autoRecordChannels[selectedKeyForCopying][k];
          state.gateChannels[i][k] = state.gateChannels[selectedKeyForCopying][k];
          state.gateVoltages[i][j][k] = state.gateVoltages[selectedKeyForCopying][j][k];
          state.lockedVoltages[i][j][k] = state.lockedVoltages[selectedKeyForCopying][j][k];
          state.randomInputChannels[i][k] = state.randomInputChannels[selectedKeyForCopying][k];
          state.randomOutputChannels[i][k] = state.randomOutputChannels[selectedKeyForCopying][k];
          state.randomVoltages[i][j][k] = state.randomVoltages[selectedKeyForCopying][j][k];
          state.voltages[i][j][k] = state.voltages[selectedKeyForCopying][j][k];
        }
      }
      state.pasteTargetKeys[i] = false;
    }
  }
  state.selectedKeyForCopying = -1;
  return state;
}

/**
 * @brief Paste all 16 preset voltage values from one channel to the set of target channels.
 *
 * @param state
 * @return State
 */
State State::pasteChannels(State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t selectedKeyForCopying = state.selectedKeyForCopying;
  for (uint8_t i = 0; i < 8; i++) { // channels
    if (state.pasteTargetKeys[i]) {
      if (state.gateChannels[currentBank][selectedKeyForCopying]) {
        state.gateChannels[state.currentBank][i] = true;
        for (uint8_t j = 0; j < 16; j++) {
          state.gateVoltages[currentBank][j][i] =
            state.gateVoltages[currentBank][j][selectedKeyForCopying];
        }
      }
      else {
        for (uint8_t j = 0; j < 16; j++) { // presets
          state.activeVoltages[currentBank][j][i] =
            state.activeVoltages[currentBank][j][state.selectedKeyForCopying];
          state.voltages[currentBank][j][i] =
            state.voltages[currentBank][j][state.selectedKeyForCopying];
        }
      }
    }
    state.pasteTargetKeys[i] = false;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

/**
 * @brief Within the current channel, paste voltage values from one preset to the set of target
 * presets.
 *
 * @param state
 * @return State
 */
State State::pasteVoltages(State state) {
  for (uint8_t i = 0; i < 16; i++) { // presets
    if (state.pasteTargetKeys[i]) {
      state.voltages[state.currentBank][i][state.currentChannel] =
        state.voltages[state.currentBank][state.selectedKeyForCopying][state.currentChannel];
    }
    state.pasteTargetKeys[i] = false;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

/**
 * @brief Paste all 8 channel voltage values from one preset to the set of target presets.
 *
 * @param state
 * @return State
 */
State State::pastePresets(State state) {
  for (uint8_t i = 0; i < 16; i++) { // presets
    if (state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 8; j++) { // channels
        state.voltages[state.currentBank][i][j] =
          state.voltages[state.currentBank][state.selectedKeyForCopying][j];
      }
    }
    state.pasteTargetKeys[i] = false;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::quitCopyPasteFlowPriorToPaste(State state) {
  state.selectedKeyForCopying = -1;
  for (uint8_t i = 0; i < 16; i++) {
    state.pasteTargetKeys[i] = false;
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
      state.currentPreset = doc["currentPreset"];
      state.currentBank = doc["currentBank"];
      state.currentChannel = doc["currentChannel"];
      copyArray(doc["removedPresets"], state.removedPresets);
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
      copyArray(doc["activeVoltages"], state.activeVoltages[bank]);
      copyArray(doc["autoRecordChannels"], state.autoRecordChannels[bank]);
      copyArray(doc["gateChannels"], state.gateChannels[bank]);
      copyArray(doc["gateVoltages"], state.gateVoltages[bank]);
      copyArray(doc["lockedVoltages"], state.lockedVoltages[bank]);
      copyArray(doc["randomInputChannels"], state.randomInputChannels[bank]);
      copyArray(doc["randomOutputChannels"], state.randomOutputChannels[bank]);
      copyArray(doc["randomVoltages"], state.randomVoltages[bank]);
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
  moduleDoc["currentPreset"] = state.currentPreset;
  for (uint8_t i = 0; i < 16; i++) {
    moduleDoc["removedPresets"][i] = state.removedPresets[i];
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
  JsonArray activeVoltages = bankRoot.createNestedArray("activeVoltages");
  JsonArray gateVoltages = bankRoot.createNestedArray("gateVoltages");
  JsonArray lockedVoltages = bankRoot.createNestedArray("lockedVoltages");
  JsonArray randomVoltages = bankRoot.createNestedArray("randomVoltages");
  JsonArray voltages = bankRoot.createNestedArray("voltages");
  for (uint8_t i = 0; i < 16; i++) {
    JsonArray activeVoltagesChannelArray = activeVoltages.createNestedArray();
    JsonArray gateVoltagesChannelArray = gateVoltages.createNestedArray();
    JsonArray lockedVoltagesChannelArray = lockedVoltages.createNestedArray();
    JsonArray randomVoltagesChannelArray = randomVoltages.createNestedArray();
    JsonArray voltagesChannelArray = voltages.createNestedArray();
    for (uint8_t j = 0; j < 8; j++) {
      activeVoltagesChannelArray.add(state.activeVoltages[bank][i][j]);
      gateVoltagesChannelArray.add(state.gateVoltages[bank][i][j]);
      lockedVoltagesChannelArray.add(state.lockedVoltages[bank][i][j]);
      randomVoltagesChannelArray.add(state.randomVoltages[bank][i][j]);
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
