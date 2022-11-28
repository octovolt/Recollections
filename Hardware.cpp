/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Hardware.h"

#include <Adafruit_MCP4728.h>
#include <Entropy.h>
#include <string.h> // for memcpy() -- would prefer to remove this if possible

#include "Utils.h"
#include "constants.h"


/**
 * @brief This is the entry point for side effects reflected in the hardware, based on the current
 * state: the display of colors in the grid of keys and the production of voltage in the DACs.
 *
 * @param state
 * @return true
 * @return false
 */
bool Hardware::reflectState(State state) {
  // voltage output
  bool result = Hardware::setOutputsAll(state);
  if (!result) {
    Serial.println("could not set outputs");
    return result;
  }

  // rendering of color and brightness in the 16 keys
  switch (state.screen) {
    case SCREEN.BANK_SELECT:
      result = Hardware::renderBankSelect(state);
      break;
    case SCREEN.EDIT_CHANNEL_SELECT:
      result = Hardware::renderEditChannelSelect(state);
      break;
    case SCREEN.EDIT_CHANNEL_VOLTAGES:
      result = Hardware::renderEditChannelVoltages(state);
      break;
    case SCREEN.ERROR:
      result = Hardware::renderError(state);
      break;
    case SCREEN.GLOBAL_EDIT:
      result = Hardware::renderGlobalEdit(state);
      break;
    case SCREEN.MODULE_SELECT:
      result = Hardware::renderModuleSelect(state);
      break;
    case SCREEN.RECORD_CHANNEL_SELECT:
      result = Hardware::renderRecordChannelSelect(state);
      break;
    case SCREEN.SECTION_SELECT:
      result = Hardware::renderSectionSelect(state);
      break;
    case SCREEN.STEP_CHANNEL_SELECT:
      result = Hardware::renderStepChannelSelect(state);
      break;
    case SCREEN.STEP_SELECT:
      result = Hardware::renderStepSelect(state);
      break;
  }

  return result;
}

//--------------------------------------- PRIVATE --------------------------------------------------

/**
 * @brief Set color values for a NeoTrellis key as either on or off. Note that this only *prepares*
 * a key to display the correct color. After the key is prepared, trellis.pixels.show() must be
 * called afterward.
 *
 * @param state Global state object.
 * @param step Which of the 16 steps/keys is targeted for changing.
 */
bool Hardware::prepareRenderingOfChannelEditGateStep(State state, uint8_t step) {
  if (state.randomSteps[state.currentBank][step][state.currentChannel]) {
    return Hardware::prepareRenderingOfRandomizedKey(state, step);
  }

  uint8_t color[3] = {};
  if (state.currentStep == step) {
    memcpy(color, state.config.colors.white, 3);
  } else if (state.gateSteps[state.currentBank][step][state.currentChannel]) {
    memcpy(color, state.config.colors.yellow, 3);
  } else {
    memcpy(color, state.config.colors.purple, 3);
  }

  return Hardware::prepareRenderingOfKey(state, step, color);
}

/**
 * @brief Set color values for a NeoTrellis key. Note that this only *prepares* a key to display the
 * correct color. After the key is prepared, trellis.pixels.show() must be called afterward.
 *
 * @param state Global state object.
 * @param step Which of the 16 steps/keys is targeted for changing.
 */
bool Hardware::prepareRenderingOfChannelEditVoltageStep(State state, uint8_t step) {
  uint8_t color[3] = {};
  if (
    state.selectedKeyForCopying >= 0 &&
    state.flash == 0 &&
    (step == state.selectedKeyForCopying ||
     state.pasteTargetKeys[step])
  ) {
    return Hardware::prepareRenderingOfKey(state, step, color);
  }

  if (state.randomSteps[state.currentBank][step][state.currentChannel]) {
    return Hardware::prepareRenderingOfRandomizedKey(state, step);
  }

  if (state.lockedVoltages[state.currentBank][step][state.currentChannel]) {
    memcpy(color, state.config.colors.orange, 3);
  }
  else if (!state.activeSteps[state.currentBank][step][state.currentChannel]) {
    memcpy(color, state.config.colors.purple, 3);
  }
  else if (state.currentStep == step) {
    memcpy(color, state.config.colors.white, 3);
  }
  else { // yellow
    int16_t voltage = state.voltages[state.currentBank][step][state.currentChannel];
    uint8_t colorValue = static_cast<int>(
      COLOR_VALUE_MAX * voltage * PERCENTAGE_MULTIPLIER_10_BIT
    );
    uint8_t yellowShade[3] = {colorValue, colorValue, 0};
    memcpy(color, yellowShade, 3);
  }
  return Hardware::prepareRenderingOfKey(state, step, color);
}

/**
 * @brief Set the pixel color of a single key. This method should be used in all cases to ensure
 * that the inverted orientation renders correctly. This method only *prepares* a key to display the
 * correct color. After the key is prepared, trellis.pixels.show() must be called afterward.
 * NOTE: No other method should call trellis.pixels.setPixelColor().
 *
 * @param state
 * @param key
 * @param rgbColor
 * @return true
 * @return false
 */
bool Hardware::prepareRenderingOfKey(State state, uint8_t key, uint8_t rgbColor[]) {
  // uint8_t rgbColorSize = sizeof(rgbColor)/sizeof(rgbColor[0]);
  // if (rgbColorSize != 3) {
  //   Serial.printf("%s %u %s \n", "RGB color provided with", rgbColorSize, "values instead of 3.");
  //   return 0;
  // }
  uint8_t displayKey = state.config.controllerOrientation
    ? key
    : 15 - key;
  state.config.trellis.pixels.setPixelColor(displayKey, rgbColor[0], rgbColor[1], rgbColor[2]);
  return 1;
}

/**
 * @brief Set the pixel color of a key to a random color. Note that this only *prepares* the key
 * to display a random color. After the key is prepared, trellis.pixels.show() must be called
 * afterward.
 *
 * @param state
 * @param key
 * @return true
 * @return false
 */
bool Hardware::prepareRenderingOfRandomizedKey(State state, uint8_t key) {
  if (state.randomColorShouldChange) {
    uint8_t red = Entropy.random(MAX_UNSIGNED_8_BIT);
    uint8_t green = Entropy.random(MAX_UNSIGNED_8_BIT);
    uint8_t blue = Entropy.random(MAX_UNSIGNED_8_BIT);
    uint8_t color[3] = {red, green, blue};
    return Hardware::prepareRenderingOfKey(state, key, color);
  }
  // else no op
  return 1;
}

bool Hardware::renderBankSelect(State state) {
  uint8_t nonilluminated[3] = {};
  uint8_t blue[3];
  memcpy(blue, state.config.colors.blue, 3);
  if (state.selectedKeyForCopying < 0) {
    for (uint8_t i = 0; i < 16; i++) {
      if (i != state.currentBank) {
        Hardware::prepareRenderingOfKey(state, i, nonilluminated);
      }
    }
    Hardware::prepareRenderingOfKey(state, state.currentBank, blue);
  }
  else {
    for (int8_t step = 0; step < 16; step++) {
      Hardware::prepareRenderingOfKey(
        state,
        step,
        state.flash && (step == state.selectedKeyForCopying || state.pasteTargetKeys[step])
          ? blue
          : nonilluminated
      );
    }
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderEditChannelSelect(State state) {
  for (uint8_t i = 0; i < 16; i++) {
    // non-illuminated keys
    if (
      i > 7 || (state.flash == 0 && (i == state.selectedKeyForCopying || state.pasteTargetKeys[i]))
    ) {
      uint8_t nonilluminated[3] = {};
      Hardware::prepareRenderingOfKey(state, i, nonilluminated);
    }
    // illuminated keys
    else {
      if (state.randomOutputChannels[state.currentBank][i]) {
        Hardware::prepareRenderingOfRandomizedKey(state, i);
      }
      else {
        uint8_t color[3] = {};
        memcpy(
          color,
          state.gateChannels[state.currentBank][i]
            ? state.config.colors.purple
            : state.config.colors.yellow,
          3
        );
        Hardware::prepareRenderingOfKey(state, i, color);
      }
    }
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderEditChannelVoltages(State state) {
  seesaw_NeoPixel pixels = state.config.trellis.pixels;
  for (uint8_t i = 0; i < 16; i++) {
    if (state.gateChannels[state.currentBank][state.currentChannel]) {
      Hardware::prepareRenderingOfChannelEditGateStep(state, i);
    }
    else {
      Hardware::prepareRenderingOfChannelEditVoltageStep(state, i);
    }
  }
  pixels.show();
  return 1;
}

bool Hardware::renderError(State state) {
  for (uint8_t key = 0; key < 16; key++) {
    uint8_t color[3] = {};
    if (state.flash) {
      memcpy(color, state.config.colors.red, 3);
    }
    Hardware::prepareRenderingOfKey(state, key, color);
  }
  state.config.trellis.pixels.show();
  return 0; // stay in error screen
}

bool Hardware::renderGlobalEdit(State state) {
  uint8_t color[3] = {};
  for (uint8_t i = 0; i < 16; i++) {
    bool allChannelVoltagesLocked = 1;
    bool allChannelStepsInactive = 1;
    for (uint8_t j = 0; j < 8; j++) {
      if (!state.lockedVoltages[state.currentBank][i][j]) {
        allChannelVoltagesLocked = 0;
      }
      if (state.activeSteps[state.currentBank][i][j]) {
        allChannelStepsInactive = 0;
      }
    }
    if (
      state.removedSteps[i] ||
      (
        state.flash == 0 &&
        (state.selectedKeyForCopying == i || state.pasteTargetKeys[i])
      )
    ) {
      Hardware::prepareRenderingOfKey(state, i, color);
    }
    else if (allChannelVoltagesLocked) {
      memcpy(color, state.config.colors.orange, 3);
      Hardware::prepareRenderingOfKey(state, i, color);
    }
    else if (allChannelStepsInactive) {
      memcpy(color, state.config.colors.purple, 3);
      Hardware::prepareRenderingOfKey(state, i, color);
    }
    else {
      memcpy(
        color,
        state.currentStep == i
          ? state.config.colors.white
          : state.config.colors.green,
        3
      );
      Hardware::prepareRenderingOfKey(state, i, color);
    }
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderModuleSelect(State state) {
  uint8_t color[3] = {};
  uint8_t dimmedGreen[3] = {
    static_cast<uint8_t>(state.config.colors.green[0] * DIMMED_COLOR_MULTIPLIER),
    static_cast<uint8_t>(state.config.colors.green[1] * DIMMED_COLOR_MULTIPLIER),
    static_cast<uint8_t>(state.config.colors.green[2] * DIMMED_COLOR_MULTIPLIER),
  };
  for (uint8_t i = 0; i < 16; i++) {
    if (state.config.currentModule == i) {
      memcpy(color, state.config.colors.magenta, 3);
    }
    else {
      memcpy(color, dimmedGreen, 3);
    }
    Hardware::prepareRenderingOfKey(state, i, color);
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderSectionSelect(State state) {
  uint8_t color[3] = {};
  for (uint8_t i = 0; i < 16; i++) {
    switch (Utils::keyQuadrant(i)) {
      case QUADRANT.INVALID:
        return 0;
      case QUADRANT.NW: // EDIT_CHANNEL_SELECT
        memcpy(color, state.config.colors.yellow, 3);
        break;
      case QUADRANT.NE: // RECORD_CHANNEL_SELECT
        memcpy(color, state.config.colors.red, 3);
        break;
      case QUADRANT.SW: // GLOBAL_EDIT
        memcpy(color, state.config.colors.green, 3);
        break;
      case QUADRANT.SE: // BANK_SELECT
        memcpy(color, state.config.colors.blue, 3);
        break;
    }
    Hardware::prepareRenderingOfKey(state, i, color);
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderRecordChannelSelect(State state) {
  uint8_t color[3] = {};
  for (uint8_t key = 0; key < 16; key++) {
    if (
      key > 7 ||
      (!state.readyForRecInput &&
        state.flash == 0 &&
        (state.autoRecordChannels[state.currentBank][key] ||
        state.randomInputChannels[state.currentBank][key]))
    ) {
      Hardware::prepareRenderingOfKey(state, key, color); // not illuminated
    }
    else if (state.lockedVoltages[state.currentBank][state.currentStep][key]) {
      memcpy(color, state.config.colors.orange, 3);
      Hardware::prepareRenderingOfKey(state, key, color);
    }
    else if (state.randomInputChannels[state.currentBank][key]) {
      Hardware::prepareRenderingOfRandomizedKey(state, key);
    }
    else {
      uint16_t voltage = state.voltages[state.currentBank][state.currentStep][key];
      uint8_t colorValue = state.autoRecordChannels[state.currentBank][key]
        ? 255
        : static_cast<uint8_t>(COLOR_VALUE_MAX * voltage * PERCENTAGE_MULTIPLIER_10_BIT);
      uint8_t redShade[3] = {colorValue, 0, 0};
      Hardware::prepareRenderingOfKey(state, key, redShade);
    }
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderStepChannelSelect(State state) {
  uint8_t color[3];
  uint8_t dimmed[3] = {
    static_cast<uint8_t>(state.config.colors.white[0] * DIMMED_COLOR_MULTIPLIER),
    static_cast<uint8_t>(state.config.colors.white[1] * DIMMED_COLOR_MULTIPLIER),
    static_cast<uint8_t>(state.config.colors.white[2] * DIMMED_COLOR_MULTIPLIER),
  };
  uint8_t nonilluminated[3] = {};
  for (uint8_t i = 0; i < 16; i++) {
    if (i < 8) {
      if (state.currentChannel == i) {
        memcpy(color, state.config.colors.white, 3);
      }
      else {
        memcpy(color, dimmed, 3);
      }
    }
    else {
      memcpy(color, nonilluminated, 3);
    }
    Hardware::prepareRenderingOfKey(state, i, color);
  }
  state.config.trellis.pixels.show();
  return 1;
}

bool Hardware::renderStepSelect(State state) {
  uint8_t color[3] = {};
  for (uint8_t i = 0; i < 16; i++) {
    if (state.selectedKeyForRecording == i) {
      uint16_t voltage =
        state.voltages[state.currentBank][state.selectedKeyForRecording][state.currentChannel];
      uint8_t redShade[3] = {
        static_cast<uint8_t>(COLOR_VALUE_MAX * voltage * PERCENTAGE_MULTIPLIER_10_BIT),
        0,
        0
      };
      Hardware::prepareRenderingOfKey(state, state.selectedKeyForRecording, redShade);
    }
    else {
      uint8_t nonilluminated[3] = {};
      memcpy(color, state.currentStep == i ? state.config.colors.white : nonilluminated, 3);
      Hardware::prepareRenderingOfKey(state, i, color);
    }
  }
  state.config.trellis.pixels.show();
  return 1;
}

/**
 * @brief Set the output of a channel.
 * @param state The app-wide state struct. See State.h.
 * @param channel The channel to set, 0-7.
 * @param voltageValue The stored voltage value, a 10-bit integer.
 */
bool Hardware::setOutput(State state, const int8_t channel, const uint16_t voltageValue) {
  if (channel > 7) {
    Serial.printf("%s %u \n", "invalid channel", channel);
    return 0;
  }
  if (0 > voltageValue || voltageValue > MAX_UNSIGNED_10_BIT) {
    Serial.printf("%s %u \n", "invalid 10-bit voltage value", voltageValue);
    return 0;
  }
  Adafruit_MCP4728 dac = channel < 4 ? state.config.dac1 : state.config.dac2;
  int dacChannel = channel < 4 ? channel : channel - 4; // normalize to output indexes 0 to 3.
  bool writeSuccess = dac.setChannelValue(DAC_CHANNELS[dacChannel], Utils::tenBitToTwelveBit(voltageValue));
  if (!writeSuccess) {
    Serial.println("setOutput unsuccessful; setChannelValue error");
    return 0;
  }
  return 1;
}

/**
 * @brief Set the output of all channels.
 * @param state The app-wide state struct. See State.h.
 */
bool Hardware::setOutputsAll(State state) {
  // TODO: revise to use dac.fastWrite().
  // See https://adafruit.github.io/Adafruit_MCP4728/html/class_adafruit___m_c_p4728.html
  for (uint8_t channel = 0; channel < 8; channel++) {
    uint16_t voltageValue = Utils::voltageValueForStep(state, state.currentStep, channel);
    if(!Hardware::setOutput(state, channel, voltageValue)) {
      return 0;
    }
  }
  return 1;
}