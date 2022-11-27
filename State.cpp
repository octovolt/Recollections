/**
 * Copyright 2022 William Edward Fisher.
 */
#include "State.h"

#include <SD.h>
#include <SPI.h>
#include <StackString.hpp> // I have not yet understood how to use cstrings
using namespace Stack;

#include "Utils.h"

State State::pasteBanks(State state) {
  Serial.println("pasteBanks()");
  for (uint8_t i = 0; i < 16; i++) { // banks
    if (state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 16; j++) { // steps
        for (uint8_t k = 0; k < 8; k++) { // channels
           state.voltages[i][j][k] = state.voltages[state.selectedKeyForCopying][j][k];
        }
      }
      state.pasteTargetKeys[i] = 0;
    }
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::pasteChannels(State state) {
  // paranoia: go through all 16 keys to make sure any extra keys are cleared. change to 8 keys?
  for (uint8_t i = 0; i < 16; i++) { // channels
    if (i < 8 && state.pasteTargetKeys[i]) {
      for (uint8_t j = 0; j < 16; j++) { // steps
        state.voltages[state.currentBank][j][i] = 
          state.voltages[state.currentBank][j][state.selectedKeyForCopying];
      }
    }
    state.pasteTargetKeys[i] = 0;
  }
  state.selectedKeyForCopying = -1;
  return state;
}

State State::pasteChannelSteps(State state) {
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

State State::readModuleFromSDCard(State state) {
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
    StaticJsonDocument<MODULE_JSON_DOC_DESERIALIZATION_SIZE> doc;
    DeserializationError error = deserializeJson(doc, moduleFile);
    if (error == DeserializationError::EmptyInput) {
      Serial.println("Module.txt is an empty file");
    }
    else if (error) {
      Serial.printf("%s %s \n", "deserializeJson() failed during read operation: ", error.c_str());
    }
    else {
      state.autoRecordEnabled = doc["autoRecordEnabled"];
      state.currentStep = doc["currentStep"];
      state.currentBank = doc["currentBank"];
      state.currentChannel = doc["currentChannel"];
      copyArray(doc["removedSteps"], state.removedSteps);
    }
    moduleFile.close();
  }
  
  return state;
}

State State::readBankFromSDCard(State state, uint8_t bank) {
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
      copyArray(doc["gateChannels"], state.gateChannels[bank]);
      copyArray(doc["gateLengths"], state.gateLengths[bank]);
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

bool State::writeModuleAndBankToSDCard(State state) {
  Serial.println("writing to SD card");

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
  moduleDoc["autoRecordEnabled"] = state.autoRecordEnabled;
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
  JsonArray gateChannels = bankRoot.createNestedArray("gateChannels");
  JsonArray randomInputChannels = bankRoot.createNestedArray("randomInputChannels");
  JsonArray randomOutputChannels = bankRoot.createNestedArray("randomOutputChannels");
  for (uint8_t i = 0; i < 8; i++) {
    gateChannels.add(state.gateChannels[bank][i]);
    randomInputChannels.add(state.randomInputChannels[bank][i]);
    randomOutputChannels.add(state.randomOutputChannels[bank][i]);
  }
  JsonArray activeSteps = bankRoot.createNestedArray("activeSteps");
  JsonArray gateLengths = bankRoot.createNestedArray("gateLengths");
  JsonArray gateSteps = bankRoot.createNestedArray("gateSteps");
  JsonArray lockedVoltages = bankRoot.createNestedArray("lockedVoltages");
  JsonArray randomSteps = bankRoot.createNestedArray("randomSteps");
  JsonArray voltages = bankRoot.createNestedArray("voltages");
  for (uint8_t i = 0; i < 16; i++) {
    JsonArray activeStepsChannelArray = activeSteps.createNestedArray();
    JsonArray gateLengthsChannelArray = gateLengths.createNestedArray();
    JsonArray gateStepsChannelArray = gateSteps.createNestedArray();
    JsonArray lockedVoltagesChannelArray = lockedVoltages.createNestedArray();
    JsonArray randomStepsChannelArray = randomSteps.createNestedArray();
    JsonArray voltagesChannelArray = voltages.createNestedArray();
    for (uint8_t j = 0; j < 8; j++) {
      activeStepsChannelArray.add(state.activeSteps[bank][i][j]);
      gateLengthsChannelArray.add(state.gateLengths[bank][i][j]);
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
