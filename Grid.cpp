/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Grid.h"

#include <Adafruit_NeoTrellis.h>
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
      case SCREEN.RECORD_CHANNEL_SELECT:
        state = Grid::handleRecordChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.SECTION_SELECT:
        state = Grid::handleSectionSelectKeyEvent(key, state);
        break;
      case SCREEN.STEP_CHANNEL_SELECT:
        state = Grid::handleStepChannelSelectKeyEvent(key, state);
        break;
      case SCREEN.STEP_SELECT:
        state = Grid::handleStepSelectKeyEvent(key, state);
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
  if (state.selectedKeyForCopying < 0) { // No step selected, initiate copy.
    state.selectedKeyForCopying = key;
    state.pasteTargetKeys[key] = 1;
  }
  else { // Pressed step should be added or removed from the set of paste steps.
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
    if (state.initialKeyPressedDuringModHold < 0) {
      state.initialKeyPressedDuringModHold = key;
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

  // Gate channel
  if (state.gateChannels[currentBank][state.currentChannel]) {
    // MOD button is not being held, so toggle gate on or off
    if (state.readyForModPress) {
      state.gateSteps[currentBank][key][currentChannel] =
        !state.gateSteps[currentBank][key][currentChannel];
    }
    // MOD button is being held
    else {
      // Update mod + key tracking.
      if (state.initialKeyPressedDuringModHold < 0) {
        state.initialKeyPressedDuringModHold = key;
      }
      state = Grid::updateModKeyCombinationTracking(key, state);

      // Step is a random coin-flip between gate on or gate off
      if (state.keyPressesSinceModHold == 1) {
        state.randomSteps[currentBank][key][currentChannel] =
          !state.randomSteps[currentBank][key][currentChannel];
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
      // Clear states - is this really needed? Should the code below always toggle rather than set
      // to a value?
      if (state.keyPressesSinceModHold == 0) {
        state.lockedVoltages[currentBank][key][currentChannel] = 0;
        state.activeSteps[currentBank][key][currentChannel] = 1;
        state.randomSteps[currentBank][key][currentChannel] = 0;
      }

      // Update mod + key tracking.
      if (state.initialKeyPressedDuringModHold < 0) {
        state.initialKeyPressedDuringModHold = key;
      }
      state = Grid::updateModKeyCombinationTracking(key, state);

      // Copy-paste voltage value, or restore step to defaults
      if (state.keyPressesSinceModHold == 1) {
        state = Grid::addKeyToCopyPasteData(key, state);
      }
      // Step is locked
      else if (state.keyPressesSinceModHold == 2) {
        state = State::quitCopyPasteFlowPriorToPaste(state);
        state.lockedVoltages[currentBank][key][currentChannel] = 1;
      }
      // Step is inactive
      else if (state.keyPressesSinceModHold == 3) {
        state.lockedVoltages[currentBank][key][currentChannel] = 0;
        state.activeSteps[currentBank][key][currentChannel] = 0;
      }
      // Step is random
      else if (state.keyPressesSinceModHold == 4) {
        state.activeSteps[currentBank][key][currentChannel] = 1;
        state.randomSteps[currentBank][key][currentChannel] = 1;
      }
      // Recurse
      else if (state.keyPressesSinceModHold == 5) {
        state.randomSteps[currentBank][key][currentChannel] = 0;
        state.keyPressesSinceModHold = 0;
        return Grid::handleEditChannelVoltagesKeyEvent(key, state);
      }
    }
  }

  return state;
}

State Grid::handleGlobalEditKeyEvent(uint8_t key, State state) {
  uint8_t currentBank = state.currentBank;

  if (state.readyForModPress) { // MOD button is not being held, toggle removed step
    if (state.removedSteps[key]) {
      Serial.printf("%s %u \n", "global edit restoring step: ", key);
      state.removedSteps[key] = 0;
    }
    else {
      uint8_t totalRemovedSteps = 0;
      for (uint8_t i = 0; i < 16; i++) {
        if (state.removedSteps[i]) {
          totalRemovedSteps = totalRemovedSteps + 1;
        }
      }
      Serial.printf("%s %u \n", "total removed steps: ", totalRemovedSteps);
      Serial.printf("%s %u \n", "removing step, unless it is the last one: ", key);
      state.removedSteps[key] = totalRemovedSteps < 15 ? 1 : 0;
    }
  }
  // MOD button is being held
  else {
    // Clear states for faster work flow -- is this actually needed?
    if (state.keyPressesSinceModHold == 0) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 0;
        state.activeSteps[currentBank][key][i] = 1;
      }
    }

    // Update mod + key tracking.
    if (state.initialKeyPressedDuringModHold < 0) {
      state.initialKeyPressedDuringModHold = key;
    }
    state = Grid::updateModKeyCombinationTracking(key, state);

    // Copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Grid::addKeyToCopyPasteData(key, state);
    }

    // Toggle locked voltage
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 1;
      }
    }

    // Toggle active/inactive step
    else if (state.keyPressesSinceModHold == 3) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][key][i] = 0;
        state.activeSteps[currentBank][key][i] = 0;
      }
    }

    // Recurse
    else if (state.keyPressesSinceModHold == 4) {
      for (uint8_t i = 0; i < 8; i++) {
        state.activeSteps[currentBank][key][i] = 1;
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
  if (key > 7) {
    return state;
  }

  if (!state.readyForModPress) { // MOD button is being held
    if (state.initialKeyPressedDuringModHold < 0) {
      state.initialKeyPressedDuringModHold = key;
    }
    state = Grid::updateModKeyCombinationTracking(key, state);

    // if (state.initialKeyPressedDuringModHold == key) {
      // Toggle automatic recording
      // If autorecord is already on, turn off both autorecord and random and start the cycle again.
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
      // Note: this not turn off automatic recording, as we want to use random voltage as part of
      // automatic recording in this case.
      // I got a bit confused here -- maybe there is a way to simplify this?
      else if (state.keyPressesSinceModHold == 2) {
        // if we are back to the beginning, recurse
        if (
          state.initialKeyPressedDuringModHold == key &&
          !state.autoRecordChannels[currentBank][key] &&
          !state.randomInputChannels[currentBank][key]
        ) {
          state.keyPressesSinceModHold = 0;
          return Grid::handleRecordChannelSelectKeyEvent(key, state);

        // else if we are pressing a key that is not yet random, make it random.
        // if this is the first key this will also advance the keyPressesSinceModHold count.
        } else if (!state.randomInputChannels[currentBank][key]) {
          state.autoRecordChannels[currentBank][key] = 1;
          state.randomInputChannels[currentBank][key] = 1;

        // else if we are pressing any random key other than the first, turn off random and return
        // the key to the autorecord state.
        } else if (state.initialKeyPressedDuringModHold != key) {
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
  else if ( // MOD button is not being held
    !state.lockedVoltages[state.currentBank][state.currentStep][key]
  ) {
    state.currentChannel = key;
    state.selectedKeyForRecording = key;
    state.voltages[state.currentBank][state.currentStep][key] = analogRead(CV_INPUT);
  }
  return state;
}

State Grid::handleSectionSelectKeyEvent(uint8_t key, State state) {
  bool const modButtonIsBeingHeld = !state.readyForModPress;
  switch (Utils::keyQuadrant(key)) {
    case QUADRANT.INVALID:
      state.screen = SCREEN.ERROR;
      break;
    case QUADRANT.NW: // yellow: navigate to channel editing or configure output voltage
      if (modButtonIsBeingHeld) {
        // TODO: configure output voltage
      } else {
        state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.NE: // red: navigate to recording or configure input voltage
      if (modButtonIsBeingHeld) {
        // TODO: configure input voltage
      } else {
        state = Nav::goForward(state, SCREEN.RECORD_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.SW: // green: navigate to global edit or load module
      if (modButtonIsBeingHeld) {
        state.initialKeyPressedDuringModHold = key;
        state = Nav::goForward(state, SCREEN.MODULE_SELECT);
      } else {
        state = Nav::goForward(state, SCREEN.GLOBAL_EDIT);
      }
      break;
    case QUADRANT.SE: // blue: navigate to bank select or save bank to SD
      if (modButtonIsBeingHeld) {
        state.initialKeyPressedDuringModHold = key;
        bool const writeSuccess = State::writeCurrentModuleAndBankToSDCard(state);
        if (!writeSuccess) {
          state = Nav::goForward(state, SCREEN.ERROR);
        } else {
          // TODO: do something visual to confirm the write
        }
      } else {
        state = Nav::goForward(state, SCREEN.BANK_SELECT);
      }
      break;
  }
  return state;
}

State Grid::handleStepChannelSelectKeyEvent(uint8_t key, State state) {
  if (key > 7) {
    return state;
  }
  state.currentChannel = key;
  state = Nav::goBack(state);
  return state;
}

State Grid::handleStepSelectKeyEvent(uint8_t key, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state.initialKeyPressedDuringModHold = key;
    state.selectedKeyForRecording = key;
    state.voltages[state.currentBank][state.currentStep][state.currentChannel] = analogRead(CV_INPUT);
  }
  else {
    state.currentStep = key;
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
    if (state.initialKeyPressedDuringModHold < 0) {
      state.initialKeyPressedDuringModHold = key;
      state.keyPressesSinceModHold = 1;
    }
    // initial key is pressed repeatedly
    else if (state.initialKeyPressedDuringModHold == key) {
      state.keyPressesSinceModHold = state.keyPressesSinceModHold + 1;
    }

    // paranoid defensiveness, maybe remove this?
    // uint8_t maxIterations = (
    //   state.screen == SCREEN.EDIT_CHANNEL_SELECT
    // ) ? 3 : 4;
    // if (state.keyPressesSinceModHold > maxIterations) {
    //   state.keyPressesSinceModHold = 1;
    // }
  }
  return state;
}
