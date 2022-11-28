/**
 * Recollections: Config
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_MCP4728.h>
#include <Adafruit_NeoTrellis.h>
#include <SD.h>
#include <SPI.h>

#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_CONFIG_H_
#define VOLTAGE_MEMORY_CONFIG_H_

typedef struct Colors {
  uint8_t white[3];
  uint8_t red[3];
  uint8_t blue[3];
  uint8_t yellow[3];
  uint8_t green[3];
  uint8_t purple[3];
  uint8_t orange[3];
  uint8_t magenta[3];
} Colors;

/**
 * Config is slightly different from most members of State, in that these values should be updated
 * only very rarely, if at all. More probably, the user would only be able to change these by
 * directly editing the SD card, and the values will never change after populating them in setup().
 */
typedef struct Config {
  /**
   * The current module. A module consists of 16 banks.
   * We refer to this when loading the initial module's state at start up. This should be updated
   * whenever we load a new module to replace all banks, steps, and channels in State.
   *
   * TODO: Create a "Load Module" flow. This would allow the user to load modules 0-15 dynamically.
   *
   * Note that it will be possible to go beyond 0-15 by directly editing this value on the SD card.
   * That is, if a person changes this value to 16, we will save/load bank files to/from a folder
   * associated with the index 16. However, this module 16 will be inaccessible in the "Load Module"
   * flow as we will have only 16 keys to choose from. In the near future it will be possible to
   * load higher index modules via i2c or MIDI.
   */
  uint8_t currentModule;

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
   * Flag to determine whether we should overwrite steps when using randomized output set up in the
   * Edit Channel Selection or Edit Channel Voltages screens. It can be useful to do this overwrite,
   * as no external gate or trigger is required here to get random values into memory, which is the
   * case when using randomized input on the Recording screen.
   */
  bool randomOutputOverwritesSteps;

  /**
   * The default orientation of Recollections has the keys at the bottom and the jacks at the top.
   * We refer to this as the "controller layout", and assume that Recollections would be located in
   * the row of modules closest to the musician with no other modules immediately beneath it. This
   * makes the keys easy to access as a type of controller. However, many people prefer to have all
   * of their modules conform to the more "standard" Eurorack orientation, where jacks are at the
   * bottom and all the buttons and knobs are at the top. Or perhaps a person might want to put
   * Recollections in a different location, and the more "standard" orientation makes more sense for
   * them in this case. An inverted, non-controller panel for Recollections is available, so
   * rearranging it in this way is quite possible. However, in this case, the display of steps will
   * also need to be inverted, and this flag controls whether to display the steps in this inverted
   * way.
   */
  bool controllerOrientation;

  /** Get the config data from the config file */
  static Config readConfigFromSDCard(Config config);

  private:

  static Config readConfigFromFile(Config config, File configFile);

} Config;

#endif
