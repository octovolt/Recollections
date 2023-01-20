/**
 * Recollections: Config
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_MCP4728.h>
#include <Adafruit_NeoTrellis.h>
#include <SPI.h>

// Included for File
#ifdef CORE_TEENSY
  // This needs to be the Teensy-specific version of this. Rename others to disambiguate.
  #include <SD.h>
#else
  #include <SDFS.h>
#endif

#include "typedefs.h"

#ifndef RECOLLECTIONS_CONFIG_H_
#define RECOLLECTIONS_CONFIG_H_

typedef struct Colors {
  RGBColorArray_t white;
  RGBColorArray_t red;
  RGBColorArray_t blue;
  RGBColorArray_t yellow;
  RGBColorArray_t green;
  RGBColorArray_t purple;
  RGBColorArray_t orange;
  RGBColorArray_t magenta;
  RGBColorArray_t black;
} Colors;

/**
 * Config is slightly different from most members of State, in that these values should be updated
 * only very rarely, if at all. More probably, the user would only be able to change these by
 * directly editing the SD card, and the values will never change after populating them in setup().
 */
typedef struct Config {
  /**
   * Hardware. These will not change after being instantiated in setup().
   */
  Adafruit_MCP4728 dac1;
  Adafruit_MCP4728 dac2;
  Adafruit_NeoTrellis trellis;

  /**
   * Overall brightness level, up to 255. Brightness above 120 may consume too much power. Default
   * in setup() is 100.
   */
  uint8_t brightness;

  /**
   * Colors may be changed by directly editing the SD card. The intent here is to provide an
   * affordance for accessibility, as people may not see colors the same way.
   */
  Colors colors;

  /**
   * The default orientation of Recollections has the keys at the bottom and the jacks at the top.
   * We refer to this as the "controller layout", and assume that Recollections would be located in
   * the row of modules closest to the musician with no other modules immediately beneath it. This
   * makes the keys easy to access as a type of controller. However, many people prefer to have all
   * of their modules conform to the more "standard" Eurorack orientation, where jacks are at the
   * bottom and all the buttons and knobs are at the top. Or perhaps a person might want to put
   * Recollections in a different location, and the more "standard" orientation makes more sense for
   * them in this case. An inverted, non-controller panel for Recollections is available, so
   * rearranging it in this way is quite possible. However, in this case, the display of the keys
   * will also need to be inverted, and this flag controls whether to display the keys in this
   * inverted way.
   */
  bool controllerOrientation;

  /**
   * The current module. A module consists of 16 banks.
   * We refer to this when loading the initial module's state at start up. This should be updated
   * whenever we load a new module to replace all voltages across all banks, presets, and channels
   * in State in addition to any other data within the module's directory on the SD card.
   *
   * Note that it will be possible to go beyond modules 0-15 by directly editing this value on the
   * SD card. That is, if a person changes this value to 16, we will save/load bank files to/from a
   * folder associated with the index 16. However, this module 16 will be inaccessible in the "Load
   * Module" flow as we will have only 16 keys to choose from. In the near future it will be
   * possible to load higher index modules via i2c or MIDI.
   */
  uint8_t currentModule;

  /**
   * The number of milliseconds that can be measured between gates or triggers at the ADV input jack
   * before we say state.isAdvancingPresets is false.
   */
  uint16_t isAdvancingPresetsMaxInterval;

  /**
   * The permissible limit of variation, expressed as a percentage between 0 and 1, that gates or
   * triggers must be within to be considered "regular" clock pulses. That is, if the value of
   * isClockedTolerance is 0.1, gates spaced at 1000 and 900 milliseconds would be considered to be
   * regular, as would gates at 1000 and 1100 millseconds. That is, the second interval is +/- 10%
   * of the first interval.
   */
  float isClockedTolerance;

  /**
   * Flag to determine whether we should overwrite voltages when using randomized output set up in
   * the Edit Channel Selection or Edit Channel Voltages screens. It can be useful to do this
   * overwrite, as no external gate or trigger is required here to get random values into memory,
   * which is the case when using randomized input on the Recording screen.
   */
  bool randomOutputOverwrites;

} Config;

#endif
