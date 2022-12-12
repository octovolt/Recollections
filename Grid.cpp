/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Grid.h"

#include <Adafruit_NeoTrellis.h>
#include <Entropy.h>

#include "Hardware.h"
#include "Nav.h"
#include "Utils.h"
#include "constants.h"

State Grid::handleKeyEvent(keyEvent evt, State state) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && state.readyForKeyPress) {
    uint8_t key = state.config.controllerOrientation
      ? evt.bit.NUM
      : 15 - evt.bit.NUM;
    state.readyForKeyPress = 0;
    switch (state.screen) {
      case SCREEN.BANK_SELECT:
        state = Grid::handleBankSelectKeyEvent(key, state);
        break;
      case SCREEN.EDIT_CHANNEL_SELECT:
        state = Grid::handleEditChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.EDIT_CHANNEL_VOLTAGES:
        state = Grid::handleEditChannelVoltagesKeyEvent(key, state);
        break;
      case SCREEN.ERROR:
        SCB_AIRCR = 0x05FA0004; // Do a soft reboot of Teensy. Does not work (???)
        break;
      case SCREEN.GLOBAL_EDIT:
        state = Grid::handleGlobalEditKeyEvent(key, state);
        break;
      case SCREEN.MODULE_SELECT:
        state = Grid::handleModuleSelectKeyEvent(key, state);
        break;
      case SCREEN.PRESET_CHANNEL_SELECT:
        state = Grid::handlePresetChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.PRESET_SELECT:
        state = Grid::handlePresetSelectKeyEvent(key, state);
        break;
      case SCREEN.RECORD_CHANNEL_SELECT:
        state = Grid::handleRecordChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.SECTION_SELECT:
        state = Grid::handleSectionSelectKeyEvent(key, state);
        break;
    }
  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING && !state.readyForKeyPress) {
    state.readyForKeyPress = 1;
    state.selectedKeyForRecording = -1;
  }

  return state;
}

//--------------------------------------- PRIVATE --------------------------------------------------

State Grid::addKeyToCopyPasteData(uint8_t key, State state) {
  if (state.selectedKeyForCopying == key) {
    Serial.println("Somehow began copy/paste incorrectly. This should never happen.");
  }
  if (state.selectedKeyForCopying < 0) { // No key selected yet, initiate copy of the pressed key.
    state.selectedKeyForCopying = key;
    state.pasteTargetKeys[key] = 1;
  }
  else { // Pressed key should be added or removed from the set of paste target keys.
    state.pasteTargetKeys[key] = !state.pasteTargetKeys[key];
  }
  return state;
}

State Grid::handleBankSelectKeyEvent(uint8_t key, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state = Grid::updateModKeyCombinationTracking(key, state);
    if (state.selectedKeyForCopying != key) {
      state = Grid::addKeyToCopyPasteData(key, state);
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

State Grid::handleEditChannelSelectKeyEvent(uint8_t key, State state) {
  uint8_t bank = state.currentBank;
  if (state.readyForModPress) { // MOD button is not being held, select channel and navigate
    state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_VOLTAGES);
    if (key > 7) {
      return state;
    }
    state.currentChannel = key;
  }
  else { // MOD button is being held
    state.randomOutputChannels[state.currentBank][key] = 0;

    // Update mod + key tracking.
    if (state.initialModHoldKey < 0) {
      state.initialModHoldKey = key;
    }
    state = Grid::updateModKeyCombinationTracking(key, state);

    // copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Grid::addKeyToCopyPasteData(key, state);
    }

    // toggle as gate channel
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      state.gateChannels[bank][key] = !state.gateChannels[bank][key];
    }

    // set as random channel
    else if (state.keyPressesSinceModHold == 3) {
      state.gateChannels[bank][key] = !state.gateChannels[bank][key];
      state.randomOutputChannels[state.currentBank][key] = 1;
    }

    // recurse
    else if (state.keyPressesSinceModHold == 4) {
      state.randomOutputChannels[state.currentBank][key] = 0;
      state.keyPressesSinceModHold = 0;
      return Grid::handleEditChannelSelectKeyEvent(key, state);
    }
  }

  return state;
}

State Grid::handleEditChannelVoltagesKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentChannel = state.currentChannel;

  // Alternate preset selection flow
  if (state.readyForModPress && state.readyForPresetSelection) {
    state.currentPreset = key;
    state.readyForPresetSelection = 0;
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
      // Update mod + key tracking.
      if (state.initialModHoldKey < 0) {
        state.initialModHoldKey = key;
      }
      state = Grid::updateModKeyCombinationTracking(key, state);

      // Voltage is a random coin-flip between gate on or gate off
      if (state.keyPressesSinceModHold == 1) {
        state.randomVoltages[currentBank][key][currentChannel] =
          !state.randomVoltages[currentBank][key][currentChannel];
      }
      // Recurse
      else if (state.keyPressesSinceModHold == 2) {
        // TODO: restore to previous gate length value?
        state.keyPressesSinceModHold = 0;
        return Grid::handleEditChannelVoltagesKeyEvent(key, state);
      }
    }
  }

  // CV channel
  else {
    // MOD button is not being held, so edit voltage
    if (state.readyForModPress) {
      state.selectedKeyForRecording = key;
      // See also continual recording in loop().
      state.voltages[currentBank][key][currentChannel] = analogRead(CV_INPUT);
    }
    // MOD button is being held
    else {
      // TODO: this can be improved to avoid going into copy-paste when we actually want to clear
      // the alternate state (locked, inactive, random). I think the solution here is to clear
      // states if they have been modified, and to only update keyPressesSinceModHold if we are both
      // at zero for keyPressesSinceModHold and no state needs to be cleared.
      if (state.keyPressesSinceModHold == 0) {
        state.lockedVoltages[currentBank][key][currentChannel] = 0;
        state.activeVoltages[currentBank][key][currentChannel] = 1;
        state.randomVoltages[currentBank][key][currentChannel] = 0;
      }

      // Update mod + key tracking.
      if (state.initialModHoldKey < 0) {
        state.initialModHoldKey = key;
      }
      state = Grid::updateModKeyCombinationTracking(key, state);

      // Copy-paste voltage value
      if (state.keyPressesSinceModHold == 1) {
        state = Grid::addKeyToCopyPasteData(key, state);
      }
      // Voltage is locked
      else if (state.keyPressesSinceModHold == 2) {
        state = State::quitCopyPasteFlowPriorToPaste(state);
        state.lockedVoltages[currentBank][key][currentChannel] = 1;
      }
      // Voltage is inactive
      else if (state.keyPressesSinceModHold == 3) {
        state.lockedVoltages[currentBank][key][currentChannel] = 0;
        state.activeVoltages[currentBank][key][currentChannel] = 0;
      }
      // Voltage is random
      else if (state.keyPressesSinceModHold == 4) {
        state.activeVoltages[currentBank][key][currentChannel] = 1;
        state.randomVoltages[currentBank][key][currentChannel] = 1;
      }
      // Recurse
      else if (state.keyPressesSinceModHold == 5) {
        state.randomVoltages[currentBank][key][currentChannel] = 0;
        state.keyPressesSinceModHold = 0;
        return Grid::handleEditChannelVoltagesKeyEvent(key, state);
      }
    }
  }

  return state;
}

State Grid::handleGlobalEditKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;

  if (state.readyForModPress) { // MOD button is not being held
    // Alternate preset selection flow
    if (state.readyForPresetSelection) {
      state.currentPreset = key;
      state.readyForPresetSelection = 0;
      return state;
    }

    // Toggle removed presets
    if (state.removedPresets[key]) {
      state.removedPresets[key] = 0;
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
      state.removedPresets[key] = totalRemovedPresets < 15 ? 1 : 0;
    }
  }

  // MOD button is being held
  else {
    // Clear states for faster work flow -- is this actually needed?
    if (state.keyPressesSinceModHold == 0) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 0;
        state.activeVoltages[currentBank][key][i] = 1;
      }
    }

    // Update mod + key tracking.
    if (state.initialModHoldKey < 0) {
      state.initialModHoldKey = key;
    }
    state = Grid::updateModKeyCombinationTracking(key, state);

    // Copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Grid::addKeyToCopyPasteData(key, state);
    }
    // Toggle locked voltages
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 1;
      }
    }
    // Toggle active/inactive voltages
    else if (state.keyPressesSinceModHold == 3) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 0;
        state.activeVoltages[currentBank][key][i] = 0;
      }
    }
    // Recurse
    else if (state.keyPressesSinceModHold == 4) {
      for (uint8_t i = 0; i < 8; i++) {
        state.activeVoltages[currentBank][key][i] = 1;
      }
      state.keyPressesSinceModHold = 0;
      return Grid::handleGlobalEditKeyEvent(key, state);
    }
  }
  return state;
}

State Grid::handleModuleSelectKeyEvent(uint8_t key, State state) {
  state.config.currentModule = key;
  state = State::readModuleFromSDCard(state);
  return state;
}

State Grid::handleRecordChannelSelectKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentPreset = state.currentPreset;
  uint8_t currentChannel = state.currentChannel;
  if (key > 7) {
    return state;
  }

  // MOD button is being held
  if (!state.readyForModPress) {
    state = Grid::updateModKeyCombinationTracking(key, state);
    if (state.initialModHoldKey != key) {
      return state;
    }

    // Toggle automatic recording
    // If autorecord is already on, turn off both autorecord and random
    if (state.keyPressesSinceModHold == 1) {
      state.autoRecordChannels[currentBank][key] =
        !state.autoRecordChannels[currentBank][key];
      if (
        state.randomInputChannels[currentBank][key] &&
        !state.autoRecordChannels[currentBank][key]
      ) {
        state.randomInputChannels[currentBank][key] = 0;
      }
    }

    // Turn on randomly generated input.
    // Note: this does not turn off automatic recording, as we want to use random voltage as part of
    // automatic recording in this case.
    else if (state.keyPressesSinceModHold == 2) {
      // If we are back to the beginning, recurse. That is, if we started with autorecording on, and
      // we just turned it off in the block above with the first key press, with the second key
      // press we want to recurse and turn it back on again.
      if (
        !state.autoRecordChannels[currentBank][key] &&
        !state.randomInputChannels[currentBank][key]
      ) {
        state.keyPressesSinceModHold = 0;
        return Grid::handleRecordChannelSelectKeyEvent(key, state);

      // else if we are pressing a key that is not yet random, make it random.
      } else if (!state.randomInputChannels[currentBank][key]) {
        state.autoRecordChannels[currentBank][key] = 1;
        state.randomInputChannels[currentBank][key] = 1;
        // if not advancing, sample random voltage immediately
        if (!state.isAdvancing) {
          state.cachedVoltage = state.voltages[currentBank][currentPreset][currentChannel];
          state.voltages[currentBank][currentPreset][currentChannel] =
            Entropy.random(MAX_UNSIGNED_10_BIT);
        }

      // else if we are pressing any random key other than the first, turn off random and return
      // the key to the autorecord state.
      } else if (state.initialModHoldKey != key) {
        state.randomInputChannels[currentBank][key] = 0;
      }
    }

    // Recurse
    else if (state.keyPressesSinceModHold == 3) {
      state.autoRecordChannels[currentBank][key] = 0;
      state.randomInputChannels[currentBank][key] = 0;
      state.keyPressesSinceModHold = 0;
      return Grid::handleRecordChannelSelectKeyEvent(key, state);
    }
  }

  // MOD button is not being held
  else {
    state.selectedKeyForRecording = key;
    state.currentChannel = key;
    if (!state.isAdvancing) {
      // This is only the initial sample when pressing the key. When isAdvancing is true, we do not
      // record immediately upon pressing the key here, but rather when the preset changes.
      // See updateStateAfterAdvancing() within Recollections.ino.
      state.voltages[state.currentBank][state.currentPreset][key] = analogRead(CV_INPUT);
    }
  }
  return state;
}

State Grid::handleSectionSelectKeyEvent(uint8_t key, State state) {
  bool const modButtonIsBeingHeld = !state.readyForModPress;
  Quadrant_t quadrant = Utils::keyQuadrant(key);

  // Cancel save by pressing any other quadrant
  if (state.readyToSave && quadrant != QUADRANT.SE) {
    state.readyToSave = 0;
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
          state.readyToSave = 1;
        }
        else {
          bool const writeSuccess = State::writeCurrentModuleAndBankToSDCard(state);
          if (writeSuccess) {
            state.readyToSave = 0;
            state.confirmingSave = 1;
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

State Grid::handlePresetChannelSelectKeyEvent(uint8_t key, State state) {
  if (key > 7) {
    return state;
  }
  state.currentChannel = key;
  state = Nav::goBack(state);
  return state;
}

State Grid::handlePresetSelectKeyEvent(uint8_t key, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state.initialModHoldKey = key;
    state.selectedKeyForRecording = key;
    if (
      state.randomInputChannels[state.currentBank][state.currentChannel] ||
      (state.randomVoltages[state.currentBank][state.currentPreset][state.currentBank] &&
        state.config.randomOutputOverwrites)
    ) {
      state.voltages[state.currentBank][key][state.currentChannel] =
        Entropy.random(MAX_UNSIGNED_10_BIT);
    }
    else {
      state.voltages[state.currentBank][key][state.currentChannel] = analogRead(CV_INPUT);
    }
  }
  else {
    state.currentPreset = key;
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
State Grid::updateModKeyCombinationTracking(uint8_t key, State state) {
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
