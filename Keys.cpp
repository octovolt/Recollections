/**
 * Copyright 2022 William Edward Fisher.
 *
 * This file should be about Key events and their effect on state, and nothing else. Other drivers
 * of state changes or code for key color display should be elsewhere.
 */
#include "Keys.h"

#include <Adafruit_NeoTrellis.h>

#include "Hardware.h"
#include "Nav.h"
#include "SDCard.h"
#include "Utils.h"
#include "constants.h"

State Keys::handleKeyEvent(keyEvent evt, State state) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && state.readyForKeyPress) {
    uint8_t key = state.config.controllerOrientation
      ? evt.bit.NUM
      : 15 - evt.bit.NUM;
    state.readyForKeyPress = false;
    switch (state.screen) {
      case SCREEN.BANK_SELECT:
        state = Keys::handleBankSelectKeyEvent(key, state);
        break;
      case SCREEN.EDIT_CHANNEL_SELECT:
        state = Keys::handleEditChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.EDIT_CHANNEL_VOLTAGES:
        state = Keys::handleEditChannelVoltagesKeyEvent(key, state);
        break;
      case SCREEN.ERROR:
        #ifdef CORE_TEENSY
          SCB_AIRCR = 0x05FA0004; // Do a soft reboot of Teensy
        #else
          rp2040.reboot();
        #endif
        break;
      case SCREEN.GLOBAL_EDIT:
        state = Keys::handleGlobalEditKeyEvent(key, state);
        break;
      case SCREEN.MODULE_SELECT:
        state = Keys::handleModuleSelectKeyEvent(key, state);
        break;
      case SCREEN.PRESET_CHANNEL_SELECT:
        state = Keys::handlePresetChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.PRESET_SELECT:
        state = Keys::handlePresetSelectKeyEvent(key, state);
        break;
      case SCREEN.RECORD_CHANNEL_SELECT:
        state = Keys::handleRecordChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.SECTION_SELECT:
        state = Keys::handleSectionSelectKeyEvent(key, state);
        break;
    }
  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING && !state.readyForKeyPress) {
    state.readyForKeyPress = true;
    state.selectedKeyForRecording = -1;
  }

  return state;
}

//--------------------------------------- PRIVATE --------------------------------------------------

State Keys::addKeyToCopyPasteData(uint8_t key, State state) {
  if (state.selectedKeyForCopying == key) {
    Serial.println("Somehow began copy/paste incorrectly. This should never happen.");
    return state;
  }
  if (state.selectedKeyForCopying < 0) { // No key selected yet, initiate copy of the pressed key.
    state.selectedKeyForCopying = key;
    state.pasteTargetKeys[key] = true;
  }
  else { // Pressed key should be added or removed from the set of paste target keys.
    state.pasteTargetKeys[key] = !state.pasteTargetKeys[key];
  }
  return state;
}

State Keys::carryRestsToInactiveVoltages(uint8_t key, State state) {
  for (uint8_t i = 0; i < 15; i++) {
    if (!state.gateVoltages[state.currentBank][i][key]) {
      state.activeVoltages[state.currentBank][i][key] = false;
    }
  }
  return state;
}

State Keys::handleBankSelectKeyEvent(uint8_t key, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state = Keys::updateModKeyCombinationTracking(key, state);
    if (state.selectedKeyForCopying != key) {
      state = Keys::addKeyToCopyPasteData(key, state);
    }
    else { // Pressed the original bank again, quit copy-paste and clear the paste banks.
      state = State::quitCopyPasteFlowPriorToPaste(state);
    }
  }
  else if (key != state.currentBank) {
    state.currentBank = key;
  }
  return state;
}

State Keys::handleEditChannelSelectKeyEvent(uint8_t key, State state) {
  // Invalid key
  if (key > 7) {
    return state;
  }

  state.currentChannel = key;

  // MOD button is not being held, select channel and navigate
  if (state.readyForModPress) {
    state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_VOLTAGES);
    return state;
  }

  // MOD button is being held
  uint8_t currentBank = state.currentBank;
  if (state.initialModHoldKey < 0) {
    state.initialModHoldKey = key;
  }

  // If we changed this key previously, reset the state.
  // Otherwise, update the mod + key tracking to enter the cycle of functionality.
  if (
    state.keyPressesSinceModHold == 0 &&
    (state.randomOutputChannels[currentBank][key] || state.gateChannels[currentBank][key])
  ) {
    state.randomOutputChannels[currentBank][key] = false;
    if (state.gateChannels[currentBank][key]) {
      state = Keys::carryRestsToInactiveVoltages(key, state);
      state.gateChannels[currentBank][key] = false;
    }
  } else {
    state = Keys::updateModKeyCombinationTracking(key, state);
  }

  // copy-paste
  if (state.keyPressesSinceModHold == 1) {
    state = Keys::addKeyToCopyPasteData(key, state);
  }

  // set as gate channel
  else if (state.keyPressesSinceModHold == 2) {
    state = State::quitCopyPasteFlowPriorToPaste(state);
    state.gateChannels[currentBank][key] = true;
  }

  // set as random CV channel
  else if (state.keyPressesSinceModHold == 3) {
    state.gateChannels[currentBank][key] = false;
    state.randomOutputChannels[currentBank][key] = true;
  }

  // Return to beginning
  else if (state.keyPressesSinceModHold == 4) {
    state.randomOutputChannels[currentBank][key] = false;
    state.keyPressesSinceModHold = 0;
  }

  return state;
}

State Keys::handleEditChannelVoltagesKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentChannel = state.currentChannel;

  // Alternate preset selection flow
  if (state.readyForModPress && state.readyForPresetSelection) {
    state.currentPreset = key;
    state.readyForPresetSelection = false;
    return state;
  }

  // Gate channel
  if (state.gateChannels[currentBank][state.currentChannel]) {
    // MOD button is not being held, so toggle gate on or off
    if (state.readyForModPress) {
      state.gateVoltages[currentBank][key][currentChannel] =
        !state.gateVoltages[currentBank][key][currentChannel];
    }
    // MOD button is being held
    else {
      if (state.initialModHoldKey < 0) {
        state.initialModHoldKey = key;
      }

      // If we changed this key previously, reset the state.
      // Otherwise, update the mod + key tracking to enter the cycle of functionality.
      if (
        state.keyPressesSinceModHold == 0 &&
        state.randomVoltages[currentBank][key][currentChannel]
      ) {
        state.randomVoltages[currentBank][key][currentChannel] = false;
      } else {
        state = Keys::updateModKeyCombinationTracking(key, state);
      }

      // Voltage is a random coin-flip between gate on or gate off
      if (state.keyPressesSinceModHold == 1) {
        state.randomVoltages[currentBank][key][currentChannel] = true;
      }
      // Return to beginning
      else if (state.keyPressesSinceModHold == 2) {
        state.randomVoltages[currentBank][key][currentChannel] = false;
        state.keyPressesSinceModHold = 0;
      }
    }
  }

  // CV channel
  else {
    // MOD button is not being held, so edit voltage
    if (state.readyForModPress) {
      state.selectedKeyForRecording = key;
      // See also continual recording in loop().
      #ifdef CORE_TEENSY
        state.voltages[currentBank][key][currentChannel] =
          Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
      #else
        state.voltages[currentBank][key][currentChannel] = analogRead(CV_INPUT);
      #endif
    }
    // MOD button is being held
    else {
      if (state.initialModHoldKey < 0) {
        state.initialModHoldKey = key;
      }

      // If we changed this key previously, reset the state.
      // Otherwise, update the mod + key tracking to enter the cycle of functionality.
      if (
        state.keyPressesSinceModHold == 0 &&
        (
          state.lockedVoltages[currentBank][key][currentChannel] ||
          !state.activeVoltages[currentBank][key][currentChannel] ||
          state.randomVoltages[currentBank][key][currentChannel]
        )
      ) {
        state.lockedVoltages[currentBank][key][currentChannel] = false;
        state.activeVoltages[currentBank][key][currentChannel] = true;
        state.randomVoltages[currentBank][key][currentChannel] = false;
      } else {
        state = Keys::updateModKeyCombinationTracking(key, state);
      }

      // Copy-paste voltage value
      if (state.keyPressesSinceModHold == 1) {
        state = Keys::addKeyToCopyPasteData(key, state);
      }
      // Voltage is locked
      else if (state.keyPressesSinceModHold == 2) {
        state = State::quitCopyPasteFlowPriorToPaste(state);
        state.lockedVoltages[currentBank][key][currentChannel] = true;
      }
      // Voltage is inactive
      else if (state.keyPressesSinceModHold == 3) {
        state.lockedVoltages[currentBank][key][currentChannel] = false;
        state.activeVoltages[currentBank][key][currentChannel] = false;
      }
      // Voltage is random
      else if (state.keyPressesSinceModHold == 4) {
        state.activeVoltages[currentBank][key][currentChannel] = true;
        state.randomVoltages[currentBank][key][currentChannel] = true;
      }
      // Return to beginning
      else if (state.keyPressesSinceModHold == 5) {
        state.randomVoltages[currentBank][key][currentChannel] = false;
        state.keyPressesSinceModHold = 0;
      }
    }
  }

  return state;
}

State Keys::handleGlobalEditKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;

  if (state.readyForModPress) { // MOD button is not being held
    // Alternate preset selection flow
    if (state.readyForPresetSelection) {
      state.currentPreset = key;
      state.readyForPresetSelection = false;
      return state;
    }

    // Toggle removed presets
    if (state.removedPresets[key]) {
      state.removedPresets[key] = false;
    }
    else {
      uint8_t totalRemovedPresets = 0;
      for (uint8_t i = 0; i < 16; i++) {
        if (state.removedPresets[i]) {
          totalRemovedPresets = totalRemovedPresets + 1;
        }
      }
      // NOTE: it is important to always have at least one preset, so we need to prevent the removal
      // if it would be the 16th removed preset.
      state.removedPresets[key] = totalRemovedPresets < 15 ? true : false;
    }
  }

  // MOD button is being held
  else {
    if (state.initialModHoldKey < 0) {
      state.initialModHoldKey = key;
    }

    // If we changed this key previously, reset the state.
    // Otherwise, update the mod + key tracking to enter the cycle of functionality.
    if (state.keyPressesSinceModHold == 0) {
      bool allChannelVoltagesLocked = true;
      bool allChannelVoltagesInactive = true;
      for (uint8_t i = 0; i < 8; i++) {
        if (!state.lockedVoltages[state.currentBank][key][i]) {
          allChannelVoltagesLocked = false;
        }
        if (state.activeVoltages[state.currentBank][key][i]) {
          allChannelVoltagesInactive = false;
        }
      }
      if (allChannelVoltagesLocked || allChannelVoltagesInactive) {
        for (uint8_t i = 0; i < 8; i++) {
          state.lockedVoltages[currentBank][key][i] = false;
          state.activeVoltages[currentBank][key][i] = true;
        }
        return state;
      }
    }

    state = Keys::updateModKeyCombinationTracking(key, state);

    // Copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Keys::addKeyToCopyPasteData(key, state);
    }
    // Toggle locked voltages
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = true;
      }
    }
    // Toggle active/inactive voltages
    else if (state.keyPressesSinceModHold == 3) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = false;
        state.activeVoltages[currentBank][key][i] = false;
      }
    }
    // Return to beginning
    else if (state.keyPressesSinceModHold == 4) {
      for (uint8_t i = 0; i < 8; i++) {
        state.activeVoltages[currentBank][key][i] = true;
      }
      state.keyPressesSinceModHold = 0;
    }
  }
  return state;
}

State Keys::handleModuleSelectKeyEvent(uint8_t key, State state) {
  state.config.currentModule = key;
  state = SDCard::readModuleDirectory(state);
  return state;
}

State Keys::handlePresetChannelSelectKeyEvent(uint8_t key, State state) {
  if (key > 7) {
    return state;
  }
  state.currentChannel = key;
  state = Nav::goBack(state);
  return state;
}

State Keys::handlePresetSelectKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentChannel = state.currentChannel;

  if (!state.readyForModPress) { // MOD button is being held
    state.initialModHoldKey = key;
    state.selectedKeyForRecording = key;
    if (
      state.randomInputChannels[currentBank][currentChannel] ||
      (state.randomVoltages[currentBank][state.currentPreset][currentBank] &&
        state.config.randomOutputOverwrites)
    ) {
      state.voltages[currentBank][key][currentChannel] = Utils::random(MAX_UNSIGNED_12_BIT);
    }
    else {
      #ifdef CORE_TEENSY
        state.voltages[currentBank][key][currentChannel] =
          Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
      #else
        state.voltages[currentBank][key][currentChannel] = analogRead(CV_INPUT);
      #endif
    }
  }
  else {
    state.currentPreset = key;
  }
  return state;
}

State Keys::handleRecordChannelSelectKeyEvent(uint8_t key, State state) {
  if (key > 7) {
    return state;
  }

  state.currentChannel = key;
  uint8_t currentBank = state.currentBank;
  uint8_t currentPreset = state.currentPreset;

  // MOD button is not being held
  if (state.readyForModPress) {
    state.selectedKeyForRecording = key;
    if (!state.isAdvancingPresets) {
      // This is only the initial sample when pressing the key. When isAdvancingPresets is true, we
      // do not record immediately upon pressing the key here, but rather when the preset changes.
      // See Advance::updateStateAfterAdvancing().
      #ifdef CORE_TEENSY
        state.voltages[currentBank][currentPreset][key] =
          Utils::tenBitToTwelveBit(analogRead(CV_INPUT));
      #else
        state.voltages[currentBank][currentPreset][key] = analogRead(CV_INPUT);
      #endif
    }
    return state;
  }

  // MOD button is being held
  if (state.initialModHoldKey < 0) {
    state.initialModHoldKey = key;
  }

  // Allow auto recording only on one channel at a time
  if (state.initialModHoldKey != key) {
    return state;
  }

  // If we changed this key previously, reset the state.
  // Otherwise, update the mod + key tracking to enter the cycle of functionality.
  if (
    state.keyPressesSinceModHold == 0 &&
    (state.autoRecordChannels[currentBank][key] || state.randomInputChannels[currentBank][key])
  ) {
    state.autoRecordChannels[currentBank][key] = false;
    state.randomInputChannels[currentBank][key] = false;
  }
  else {
    state = Keys::updateModKeyCombinationTracking(key, state);
  }

  // Automatic recording
  if (state.keyPressesSinceModHold == 1) {
    state.autoRecordChannels[currentBank][key] = true;
  }

  // Randomly generated input.
  // Note: this does not turn off automatic recording, as we want to use random voltage as part of
  // automatic recording in this case.
  else if (state.keyPressesSinceModHold == 2) {
    state.randomInputChannels[currentBank][key] = true;
    // if not advancing, sample random voltage immediately
    if (!state.isAdvancingPresets) {
      state.cachedVoltage = state.voltages[currentBank][currentPreset][key];
      state.voltages[currentBank][currentPreset][key] =
        Utils::random(MAX_UNSIGNED_12_BIT);
    }
  }

  // Return to beginning
  else if (state.keyPressesSinceModHold == 3) {
    state.autoRecordChannels[currentBank][key] = false;
    state.randomInputChannels[currentBank][key] = false;
    if (!state.isAdvancingPresets) {
      state.voltages[currentBank][currentPreset][key] = state.cachedVoltage;
    }
    state.keyPressesSinceModHold = 0;
  }

  return state;
}

State Keys::handleSectionSelectKeyEvent(uint8_t key, State state) {
  bool const modButtonIsBeingHeld = !state.readyForModPress;
  Quadrant_t quadrant = Utils::keyQuadrant(key);

  // Cancel save by pressing any other quadrant
  if (state.readyToSave && quadrant != QUADRANT.SE) {
    state.readyToSave = false;
    return state;
  }

  switch (quadrant) {
    case QUADRANT.INVALID:
      state.screen = SCREEN.ERROR;
      break;
    case QUADRANT.NW: // yellow: navigate to channel editing
      if (modButtonIsBeingHeld) {
        // TODO: configure output voltage?
      } else {
        state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.NE: // red: navigate to recording
      if (modButtonIsBeingHeld) {
        // TODO: configure input voltage?
      } else {
        state = Nav::goForward(state, SCREEN.RECORD_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.SW: // green: navigate to global edit or load module
      if (modButtonIsBeingHeld) {
        state.initialModHoldKey = key;
        state = Nav::goForward(state, SCREEN.MODULE_SELECT);
      } else {
        state = Nav::goForward(state, SCREEN.GLOBAL_EDIT);
      }
      break;
    case QUADRANT.SE: // blue: navigate to bank select or save bank to SD
      if (modButtonIsBeingHeld || state.readyToSave) {
        if (!state.readyToSave) {
          state.initialModHoldKey = key;
          state.readyToSave = true;
        }
        else {
          bool const writeSuccess = SDCard::writeCurrentModuleAndBank(state);
          if (writeSuccess) {
            state.readyToSave = false;
            state.confirmingSave = true;
            state.flashesSinceSave = 0;
          } else {
            state = Nav::goForward(state, SCREEN.ERROR);
          }
        }
      } else {
        state = Nav::goForward(state, SCREEN.BANK_SELECT);
      }
      break;
  }
  return state;
}

/**
 * @brief This function updates the keyPressesSinceModHold count only if this is the first key
 * pressed or the same key as the first is pressed. If another key other than the first is pressed,
 * no update of keyPressesSinceModHold occurs.
 *
 * @param key
 * @param state
 * @return State
 */
State Keys::updateModKeyCombinationTracking(uint8_t key, State state) {
  // MOD button is being held
  if (!state.readyForModPress) {
    // this is the first key to be pressed
    if (state.initialModHoldKey < 0) {
      state.initialModHoldKey = key;
      state.keyPressesSinceModHold = 1;
    }
    // initial key is pressed repeatedly
    else if (state.initialModHoldKey == key) {
      state.keyPressesSinceModHold = state.keyPressesSinceModHold + 1;
    }
  }
  return state;
}
