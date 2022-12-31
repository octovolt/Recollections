/**
 * Copyright 2022 William Edward Fisher.
 */

#include "SDCard.h"

#include <ArduinoJson.h>

#include <SPI.h>
#include <StackString.hpp> // I have not yet understood how to use cstrings
using namespace Stack;

#include "Config.h"
#include "Utils.h"

/**
 * @brief Abstraction for different SD/FS libraries across platforms.
 *
 * I wanted to make this a sub-struct of SDCard, but had trouble calling into it. I tried to make it
 * a static member (static struct FS) and call it with SDCard::FS::exists(), but kept getting this
 * error: `cannot call member function 'bool SDCard::FS::exists(const char*)' without object`
 */
typedef struct RecollectionsFileSystem {
  static File open(const char *filepath, uint8_t mode);
  static bool exists(const char *filepath);
  static bool mkdir(const char *filepath);
} RecollectionsFileSystem;

File RecollectionsFileSystem::open(const char *filepath, uint8_t mode = FILE_READ) {
  #if defined(ARDUINO_TEENSY36) || defined(ARDUINO_TEENSY41)
    File file = SD.open(filepath, mode);
    if (mode == FILE_WRITE_BEGIN) {
      file.truncate();
    }
    return file;
  #else
    switch (mode) {
      case SD_READ_CREATE: {
        // no read + create option, so read + write + create
        return SDFS.open(filepath, "w+");
        break;
      }
      case FILE_WRITE_BEGIN: {
        return SDFS.open(filepath, "w");
        break;
      }
      default: {
        return SDFS.open(filepath, "r");
        break;
      }
    }
  #endif
}

bool RecollectionsFileSystem::exists(const char *filepath) {
  #if defined(ARDUINO_TEENSY36) || defined(ARDUINO_TEENSY41)
    return SD.exists(filepath);
  #else
    return SDFS.exists(filepath);
  #endif
}

bool RecollectionsFileSystem::mkdir(const char *filepath) {
  #if defined(ARDUINO_TEENSY36) || defined(ARDUINO_TEENSY41)
    return SD.mkdir(filepath);
  #else
    return SDFS.mkdir(filepath);
  #endif
}

void SDCard::confirmOrCreatePath(State state) {
  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);
  StackString<100> modulePath = StackString<100>(MODULE_SD_PATH_PREFIX);
  modulePath.append(currentModuleString);

  if (!RecollectionsFileSystem::exists(modulePath.c_str())) {
    if (!RecollectionsFileSystem::exists("Recollections")) {
      RecollectionsFileSystem::mkdir("Recollections");
    }
    RecollectionsFileSystem::mkdir(modulePath.c_str());
  }
}

Config SDCard::readConfigFile(Config config) {
  File configFile = RecollectionsFileSystem::open(CONFIG_SD_PATH, SD_READ_CREATE);

  if (!configFile) {
    Serial.println("Could not open Config.txt");
    return config;
  } else {
    Serial.println("Successfully opened Config.txt");
  }

  if (!configFile.available()) {
    Serial.println("Config.txt was opened but is not yet available.");
    delay(500);

    // not sure about this recursion strategy. the delay may be enough.
    // configFile.close();
    // return SDCard::readConfigFile();
  }

  if (configFile.available()) {
    Serial.println("config file available");
    StaticJsonDocument<CONFIG_JSON_DOC_DESERIALIZATION_SIZE> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error == DeserializationError::EmptyInput) {
      Serial.println("Config.txt is an empty file");
      return config;
    }
    else if (error) {
      Serial.printf("%s %s \n", "deserializeJson() failed during read operation: ", error.c_str());
      return config;
    }
    else {
      Serial.println("getting config data from file");

      if (doc["brightness"] != nullptr) {
        config.brightness = doc["brightness"];
      }
      if (doc["colors"] != nullptr) {
        copyArray(doc["colors"]["white"], config.colors.white);
        copyArray(doc["colors"]["red"], config.colors.red);
        copyArray(doc["colors"]["blue"], config.colors.blue);
        copyArray(doc["colors"]["yellow"], config.colors.yellow);
        copyArray(doc["colors"]["green"], config.colors.green);
        copyArray(doc["colors"]["purple"], config.colors.purple);
        copyArray(doc["colors"]["orange"], config.colors.orange);
        copyArray(doc["colors"]["magenta"], config.colors.magenta);
        copyArray(doc["colors"]["black"], config.colors.black);
      }
      if (doc["controllerOrientation"] != nullptr) {
        config.controllerOrientation = doc["controllerOrientation"];
      }
      if (doc["currentModule"] != nullptr) {
        config.currentModule = doc["currentModule"];
      }
      if (doc["isAdvancingPresetsMaxInterval"] != nullptr) {
        config.isAdvancingPresetsMaxInterval = doc["isAdvancingPresetsMaxInterval"];
      }
      if (doc["isClockedTolerance"] != nullptr) {
        config.isClockedTolerance = doc["isClockedTolerance"];
      }
      if (doc["randomOutputOverwrites"] != nullptr) {
        config.randomOutputOverwrites = doc["randomOutputOverwrites"];
      }
    }
  }
  configFile.close();

  return config;
}

State SDCard::readModuleDirectory(State state) {
  // First we establish defaults to make sure the data is populated, then we attempt to get data
  // from the SD card.

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

  state = SDCard::readModuleFile(state);
  for (uint8_t bank = 0; bank < 16; bank++) {
    state = SDCard::readBankFile(state, bank);
  }
  return state;
}

State SDCard::readModuleFile(State state) {
  int currentModuleLength = snprintf(NULL, 0, "%d", state.config.currentModule) + 1;
  char currentModuleString[currentModuleLength];
  sprintf(currentModuleString, "%d", state.config.currentModule);

  // Recollections/Module_15/Module.txt
  StackString<100> modulePath = StackString<100>(MODULE_SD_PATH_PREFIX);
  modulePath.append(currentModuleString);
  modulePath.append("/Module.txt");

  File moduleFile = RecollectionsFileSystem::open(modulePath.c_str(), SD_READ_CREATE);
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

State SDCard::readBankFile(State state, uint8_t bank) {
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

  File bankFile = RecollectionsFileSystem::open(bankPath.c_str(), SD_READ_CREATE);

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

bool SDCard::writeCurrentModuleAndBank(State state) {
  Serial.println("writing to SD card");

  SDCard::confirmOrCreatePath(state);

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

  File moduleFile = RecollectionsFileSystem::open(modulePath.c_str(), FILE_WRITE_BEGIN);

  if (!moduleFile) {
    Serial.println("Could not open Module.txt");
    return false;
  } else {
    Serial.println("Successfully opened Module.txt");
  }

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

  File bankFile = RecollectionsFileSystem::open(bankPath.c_str(), FILE_WRITE_BEGIN);
  if (!bankFile) {
    Serial.printf("%s%s%s\n", "Could not open Bank_", bankString, ".txt");
    return false;
  } else {
    Serial.printf("%s%s%s\n", "Successfully opened Bank_", bankString, ".txt");
  }

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