/**
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"

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
      #ifdef CORE_TEENSY
        state.voltages[currentBank][currentPreset][i] =
          Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
      #else
        state.voltages[currentBank][currentPreset][i] = analogRead(CV_INPUT);
      #endif
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
    #ifdef CORE_TEENSY
      state.voltages[state.currentBank][state.selectedKeyForRecording][state.currentChannel] =
        Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
    #else
      state.voltages[state.currentBank][state.selectedKeyForRecording][state.currentChannel] =
        analogRead(CV_INPUT);
    #endif
  }
  return state;
}

/**
 * @brief Entry point to continuous recording over time rather than a single sample. Called within
 * the main loop() function.
 *
 * @param state
 * @return State
 */
State State::recordContinuously(State state) {
  if (state.selectedKeyForRecording >= 0) {
    if (
      (state.screen == SCREEN.EDIT_CHANNEL_VOLTAGES || state.screen == SCREEN.PRESET_SELECT) &&
      !state.randomInputChannels[state.currentBank][state.currentChannel]
    ) {
      state = State::editVoltageOnSelectedPreset(state);
    }
    else if (state.screen == SCREEN.RECORD_CHANNEL_SELECT && !state.isAdvancingPresets) {
      state = State::recordVoltageOnSelectedChannel(state);
    }
  }
  else if (!state.readyForRecInput && !state.isAdvancingPresets) {
    Serial.println("should auto record");
    state = State::autoRecord(state);
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
  uint8_t currentBank = state.currentBank;
  uint8_t currentPreset = state.currentPreset;
  uint8_t channel = state.selectedKeyForRecording;
  if (
    state.screen == SCREEN.RECORD_CHANNEL_SELECT &&
    !state.lockedVoltages[currentBank][currentPreset][channel]
  ) {
    #ifdef CORE_TEENSY
      state.voltages[currentBank][currentPreset][channel] =
        Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
    #else
      state.voltages[currentBank][currentPreset][channel] = analogRead(CV_INPUT);
    #endif
  }
  return state;
}

State State::paste(State state) {
  if (state.selectedKeyForCopying < 0) {
    Serial.printf("%s %u \n", "selectedKeyForCopying is unexpectedly", state.selectedKeyForCopying);
    return state;
  }
  switch (state.screen) {
    case SCREEN.BANK_SELECT:
      state = State::pasteBanks(state);
      break;
    case SCREEN.EDIT_CHANNEL_SELECT:
      state = State::pasteChannels(state);
      break;
    case SCREEN.EDIT_CHANNEL_VOLTAGES:
      state = State::pasteVoltages(state);
      break;
    case SCREEN.GLOBAL_EDIT:
      state = State::pastePresets(state);
      break;
  }
  state.selectedKeyForCopying = -1;
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

State State::setRandomVoltagesForPreset(uint8_t preset, State state) {
  for (uint8_t i = 0; i < 8; i++) {
    // random channels, random 32-bit converted to 12-bit
    if (state.randomOutputChannels[state.currentBank][i]) {
      state.voltages[state.currentBank][preset][i] = Utils::random(MAX_UNSIGNED_12_BIT);
    }

    if (state.randomVoltages[state.currentBank][preset][i]) {
      // random gate presets
      if (state.gateChannels[state.currentBank][i]) {
        uint32_t coinToss = Utils::random(2);
        state.gateVoltages[state.currentBank][preset][i] = coinToss
          ? VOLTAGE_VALUE_MAX
          : 0;
      } else {
        // random CV presets, random 32-bit converted to 12-bit
        state.voltages[state.currentBank][preset][i] = Utils::random(MAX_UNSIGNED_12_BIT);
      }
    }
  }
  return state;
}
