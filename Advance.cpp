/**
 * Copyright 2022 William Edward Fisher.
 */

#include "Advance.h"

#include "Utils.h"

/**
 * @brief Change the current preset to the next preset and calculate the expected gate length.
 *
 * @param loopStartTime
 * @return State
 */
void Advance::advancePreset(unsigned long *loopStartTime, State *state) {
  // WARNING! ACHTUNG! PELIGRO! We need to prevent infinite recursion.
  // If all presets have been somehow removed, we need to prevent nextPreset() from recursing.
  bool allowRecursion = !Advance::allPresetsRemoved(state->removedPresets);
  state->currentPreset = Advance::nextPreset(
    state->currentPreset,
    state->advancePresetAddend,
    state->removedPresets,
    allowRecursion
  );
}

bool Advance::allPresetsRemoved(bool removedPresets[]) {
  bool allRemoved = true;
  for (u_int8_t i = 0; i < 16; i++) {
    if (!removedPresets[i]) {
      allRemoved = false;
      break;
    }
  }
  return allRemoved;
}

uint8_t Advance::nextPreset(uint8_t preset, uint8_t addend, bool removedPresets[], bool allowRecursion) {
  uint8_t addendedPreset = preset + addend;
  uint8_t nextPreset =
    addendedPreset > 15
      ? addendedPreset - 16
      : addendedPreset < 0
        ? addendedPreset + 16
        : addendedPreset;
  if (allowRecursion && removedPresets[nextPreset]) {
    nextPreset = Advance::nextPreset(nextPreset, addend, removedPresets, allowRecursion);
  }
  return nextPreset;
}

/**
 * @brief This function assumes it is being called when state.isAdvancingPresets is true.
 * TODO: break this up into multiple functions that do one thing instead of this grab bag.
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
    uint8_t currentBank = state.currentBank;
    uint8_t currentPreset = state.currentPreset;
    for (uint8_t i = 0; i < 8; i++) {
      if (state.autoRecordChannels[currentPreset][i]) {
        if (state.randomInputChannels[currentPreset][i]) {
          state.voltages[currentBank][currentPreset][i] = Utils::random(MAX_UNSIGNED_12_BIT);
        }
        else {
          #ifdef CORE_TEENSY
            state.voltages[currentBank][currentPreset][i] =
              Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
          #else
            state.voltages[currentBank][currentPreset][i] = analogRead(CV_INPUT);
          #endif
        }
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
