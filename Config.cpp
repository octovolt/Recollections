/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Config.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <string.h> // for memcpy() -- would prefer to remove this if possible

#include "constants.h"

Config Config::readConfigFromSDCard() {
  Config configData;

  // Constants from SdFat.h
  // open file for reading, create it if it doesn't exist
  File configFile = SD.open(CONFIG_SD_PATH, SD_READ_CREATE);

  if (!configFile) {
    Serial.println("Could not open Config.txt");
    return configData;
  } else {
    Serial.println("Successfully opened Config.txt");
  }

  if (configFile.available()) {
    StaticJsonDocument<CONFIG_JSON_DOC_DESERIALIZATION_SIZE> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error == DeserializationError::EmptyInput) {
      Serial.println("Config.txt is an empty file");
    }
    else if (error) {
      Serial.printf("%s %s \n", "deserializeJson() failed during read operation: ", error.c_str());
    }
    else {
      configData.currentModule = doc["currentModule"] | 0;
      configData.brightness = doc["brightness"] | DEFAULT_BRIGHTNESS;

      Colors defaultColors = (Colors){
        .white = {255,255,255},
        .red = {85,0,0},
        .blue = {0,0,119},
        .yellow = {119,119,0},
        .green = {0,85,0},
        .purple = {51,0,255},
        .orange = {119,51,0},
      };

      // How can I not repeat myself here? It would be nice if I could use the pipe operator,
      // similar to how it used above for currentModule and brightness. Or if C++ would allow
      // struct member access with the [] operator.  TODO: come back to this and figure it out.
      // Also TODO: use these colors, particularly in Hardware.cpp.

      if (doc["colors"]["white"] == nullptr) {
        memcpy(configData.colors.white, defaultColors.white, 3);
      } else {
        copyArray(doc["colors"]["white"], configData.colors.white);
      }

      if (doc["colors"]["red"] == nullptr) {
        memcpy(configData.colors.red, defaultColors.red, 3);
      } else {
        copyArray(doc["colors"]["red"], configData.colors.red);
      }

      if (doc["colors"]["blue"] == nullptr) {
        memcpy(configData.colors.blue, defaultColors.blue, 3);
      } else {
        copyArray(doc["colors"]["blue"], configData.colors.blue);
      }

      if (doc["colors"]["yellow"] == nullptr) {
        memcpy(configData.colors.yellow, defaultColors.yellow, 3);
      } else {
        copyArray(doc["colors"]["yellow"], configData.colors.yellow);
      }

      if (doc["colors"]["green"] == nullptr) {
        memcpy(configData.colors.green, defaultColors.green, 3);
      } else {
        copyArray(doc["colors"]["green"], configData.colors.green);
      }

      if (doc["colors"]["purple"] == nullptr) {
        memcpy(configData.colors.purple, defaultColors.purple, 3);
      } else {
        copyArray(doc["colors"]["purple"], configData.colors.purple);
      }

      if (doc["colors"]["orange"] == nullptr) {
        memcpy(configData.colors.orange, defaultColors.orange, 3);
      } else {
        copyArray(doc["colors"]["orange"], configData.colors.orange);
      }
    }
  }
  configFile.close();

  return configData;
}
