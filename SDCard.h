/**
 * Recollections: Utils
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"

// Please note that including StreamUtils will break on RP2040 compiled using arduino-pico unless
// changes are made to the following files:
//
// /Arduino/libraries/StreamUtils/src/StreamUtils/Streams/StringStream.hpp
// /Arduino/libraries/StreamUtils/src/StreamUtils/Prints/StringPrint.hpp
//
// Required change, in both files, to allow them to compile across platforms:
//
// #ifdef CORE_TEENSY
//   #include <WString.h>
// #else
//   #include <api/String.h>
// #endif
#include <StreamUtils.h>
#include <string>

#ifdef CORE_TEENSY
  // This needs to be the Teensy-specific version of this. Rename others to disambiguate.
  #include <SD.h>
#else
  #include <SDFS.h>
#endif

#ifndef RECOLLECTIONS_SDCARD_H_
#define RECOLLECTIONS_SDCARD_H_

typedef struct SDCard {
  /**
   * @brief Get the config data from the config file, or create the file if it does not exist.
   *
   * @param config
   * @return Config
   */
  static Config readConfigFile(Config config);

  /**
   * @brief Read an entirely new module from the SD card, reading from both Module.txt and all the
   * Bank_n.txt files within a new Module_n directory, so an entirely new set of 16 banks becomes
   * available. Creat the directory structure and files if they do not yet exist.
   *
   * @param state
   * @return State
   */
  static State readModuleDirectory(State state);

  /**
   * @brief Read the persisted state values from the Module.txt file on the SD card. Create the
   * file if it does not yet exist.
   *
   * @param state
   * @return State
   */
  static State readModuleFile(State state);

  /**
   * @brief Read the persisted state values from one of the Bank_n.txt fils the SD card. Create the
   * file if it does not yet exist.
   *
   * @param state
   * @param bank
   * @return State
   */
  static State readBankFile(State state, uint8_t bank);

  /**
   * @brief Get the persisted state values from the state struct and write them to the SD card.
   * Returns a bool value denoting whether the write was successful.
   *
   * @param state
   * @return true
   * @return false
   */
  static bool writeCurrentModuleAndBank(State state);

  private:
  /**
   * @brief Make sure we have the correct path of directories set up on the SD card, or else create
   * these directories. This is required to create a file.
   *
   * @param state
   */
  static void confirmOrCreatePath(State state);
} SDCard;

#endif