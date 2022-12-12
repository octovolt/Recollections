/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Config.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
// #include <string.h> // for memcpy() -- would prefer to remove this if possible

#include "constants.h"

Config Config::readConfigFromSDCard(Config config) {
  // Constants from SdFat.h
  // open file for reading, create it if it doesn't exist
  File configFile = SD.open(CONFIG_SD_PATH, SD_READ_CREATE);

  if (!configFile) {
    Serial.println("Could not open Config.txt");
    return config;
  } else {
    Serial.println("Successfully opened Config.txt");
  }

  config = Config::readConfigFromFile(config, configFile);

  configFile.close();

  return config;
}

//--------------------------------------- PRIVATE --------------------------------------------------

Config Config::readConfigFromFile(Config config, File configFile) {
  // If the file is not available, recurse until it is.
  if (!configFile.available()) {
    Serial.println("Config.txt was opened but is not yet available.");
    delay(500);
    // return Config::readConfigFromFile(config, configFile);
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
      if (doc["isAdvancingMaxInterval"] != nullptr) {
        config.isAdvancingMaxInterval = doc["isAdvancingMaxInterval"];
      }
      if (doc["isClockedTolerance"] != nullptr) {
        config.isClockedTolerance = doc["isClockedTolerance"];
      }
      if (doc["randomOutputOverwrites"] != nullptr) {
        config.randomOutputOverwrites = doc["randomOutputOverwrites"];
      }
    }
  }

  return config;
}

