/**
 * Copyright 2022 William Edward Fisher.
 */

#include "Advance.h"

#include <Entropy.h>

/**
 * @brief Change the current preset to the next preset and calculate the expected gate length.
 *
 * @param loopStartTime
 * @return State
 */
State Advance::advancePreset(unsigned long loopStartTime, State state) {
  state.isAdvancingPresets = true;

  uint16_t avgInterval =
    ((state.lastAdvReceivedTime[0] - state.lastAdvReceivedTime[1]) +
    (state.lastAdvReceivedTime[1] - state.lastAdvReceivedTime[2])) * 0.5;
  uint16_t lastInterval = state.lastAdvReceivedTime[0] - loopStartTime;
  // If our most recent interval is below the isClockedTolerance, we are no longer being clocked.
  // See the handling of the ADV input for the upper bound of the tolerance.
  state.isClocked = lastInterval < avgInterval * (1 - state.config.isClockedTolerance)
    ? false
    : true;

  if (-15 > state.advancePresetAddend || state.advancePresetAddend > 15) {
    Serial.println("advancePresetAddend out of range, resetting it to 1");
    state.advancePresetAddend = 1;
  }
  int8_t advancedPreset = state.currentPreset + state.advancePresetAddend;
  state.currentPreset =
    advancedPreset > 15
      ? advancedPreset - 16
      : advancedPreset < 0
        ? advancedPreset + 16
        : advancedPreset;

  return Advance::skipRemovedPreset(loopStartTime, state);
}

/**
 * @brief This function assumes it is being called when state.isAdvancingPresets is true.
 *
 * @param loopStartTime
 * @return State
 */
State Advance::updateStateAfterAdvancing(unsigned long loopStartTime, State state) {
  // Press record key while advancing: sample new voltage
  if (state.screen == SCREEN.RECORD_CHANNEL_SELECT && state.selectedKeyForRecording >= 0) {
    state = State::recordVoltageOnSelectedChannel(state);
  }
  // Autorecord while advancing: sample new voltage
  else if (!state.readyForRecInput) {
    for (uint8_t i = 0; i < 8; i++) {
      if (state.autoRecordChannels[state.currentPreset][i]) {
        state.voltages[state.currentBank][state.currentPreset][i] =
          state.randomInputChannels[state.currentPreset][i]
          ? Entropy.random(MAX_UNSIGNED_10_BIT)
          : analogRead(CV_INPUT);
      }
    }
  }

  // manage gate length
  if (state.isClocked) {
    if (loopStartTime - state.lastAdvReceivedTime[0] > 0) {
      state.gateMillis = static_cast<unsigned long>((loopStartTime - state.lastAdvReceivedTime[0]) / 2);
    }
  } else {
    state.gateMillis = DEFAULT_TRIGGER_LENGTH;
  }

  // update tracking of last ADV pulse received
  state.lastAdvReceivedTime[2] = state.lastAdvReceivedTime[1];
  state.lastAdvReceivedTime[1] = state.lastAdvReceivedTime[0];
  state.lastAdvReceivedTime[0] = loopStartTime;

  return state;
}

// ---------------------------------------- PRIVATE ------------------------------------------------

/**
 * @brief Skip the current preset if it has been removed.
 *
 * @param loopStartTime
 * @return State
 */
State Advance::skipRemovedPreset(unsigned long loopStartTime, State state) {
  // WARNING! ACHTUNG! PELIGRO!
  // Make sure to prevent an infinite loop before calling advancePreset()! We cannot allow all
  // presets to be removed, but if somehow they are, do not call advancePreset().
  bool allPresetsRemoved = true;
  for (u_int8_t i = 0; i < 16; i++) {
    if (!state.removedPresets[i]) {
      allPresetsRemoved = false;
      break;
    }
  }
  if (!allPresetsRemoved && state.removedPresets[state.currentPreset]) {
    state = Advance::advancePreset(loopStartTime, state);
  }
  return state;
}
