/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Grid.h"

#include <Adafruit_NeoTrellis.h>
#include "Hardware.h"
#include "Nav.h"
#include "Utils.h"
#include "constants.h"

State Grid::addKeyToCopyPasteData(keyEvent evt, State state) {
  if (state.selectedKeyForCopying == evt.bit.NUM) {
    Serial.println("Somehow began copy/paste incorrectly. This should never happen.");
  }
  if (state.selectedKeyForCopying < 0) { // No step selected, initiate copy.
    state.selectedKeyForCopying = evt.bit.NUM;
    state.pasteTargetKeys[evt.bit.NUM] = 1;
  } 
  else { // Pressed step should be added or removed from the set of paste steps.
    state.pasteTargetKeys[evt.bit.NUM] = !state.pasteTargetKeys[evt.bit.NUM];
  }
  return state;
}

State Grid::handleBankSelectKeyEvent(keyEvent evt, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state = Grid::updateModKeyCombinationTracking(evt, state);
    if (state.selectedKeyForCopying != evt.bit.NUM) {
      state = Grid::addKeyToCopyPasteData(evt, state);
    } 
    else { // Pressed the original bank again, quit copy-paste and clear the paste banks.
      state = State::quitCopyPasteFlowPriorToPaste(state);
    }
  }
  else if (evt.bit.NUM != state.currentBank) {
    state.currentBank = evt.bit.NUM;
  }
  return state;
}

State Grid::handleEditChannelSelectKeyEvent(keyEvent evt, State state) {
  uint8_t bank = state.currentBank;
  if (state.readyForModPress) { // MOD button is not being held, select channel and navigate
    if (evt.bit.NUM > 7) {
      return state;
    }
    state.currentChannel = evt.bit.NUM;
    state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_VOLTAGES);
  }
  else { // MOD button is being held
    if (state.initialKeyPressedDuringModHold < 0) {
      state.initialKeyPressedDuringModHold = evt.bit.NUM;
    }
    state = Grid::updateModKeyCombinationTracking(evt, state);
    
    // copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Grid::addKeyToCopyPasteData(evt, state);
    }

    // toggle as gate channel
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      state.gateChannels[bank][evt.bit.NUM] = !state.gateChannels[bank][evt.bit.NUM];
    }

    // set as random channel
    else if (state.keyPressesSinceModHold == 3) {
      state.gateChannels[bank][evt.bit.NUM] = !state.gateChannels[bank][evt.bit.NUM];
      state.randomOutputChannels[state.currentBank][evt.bit.NUM] = 1;
    }

    // recurse
    else if (state.keyPressesSinceModHold == 4) {
      state.randomOutputChannels[state.currentBank][evt.bit.NUM] = 0;
      state.keyPressesSinceModHold = 0;
      return Grid::handleEditChannelSelectKeyEvent(evt, state);
    }
  }

  return state;
}

State Grid::handleEditChannelVoltagesKeyEvent(keyEvent evt, State state) {
  uint8_t currentBank = state.currentBank;
  uint8_t currentChannel = state.currentChannel;

  // TODO: how to abstract this repetitive code?

  // Gate channel
  if (state.gateChannels[currentBank][state.currentChannel]) {
    // MOD button is not being held, so toggle gate on or off
    if (state.readyForModPress) { 
      state.gateSteps[currentBank][evt.bit.NUM][currentChannel] = 
        !state.gateSteps[currentBank][evt.bit.NUM][currentChannel];
    }
    // MOD button is being held
    else {
      // Clear states
      if (state.keyPressesSinceModHold == 0) {
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 0;
        state.gateLengths[currentBank][evt.bit.NUM][currentChannel] = 0.5;
      }
      // Update mod + key tracking. This must occur after clearing state because it updates the 
      // value of keyPressesSinceModHold.
      state = Grid::updateModKeyCombinationTracking(evt, state);

      // Step is a random coin-flip between gate on or gate off
      if (state.keyPressesSinceModHold == 1) {
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 1;
      }
      // Set gate length to custom value. Note that this is not continually recording, but a sample.
      else if (state.keyPressesSinceModHold == 2) {
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 0;
        state.gateLengths[currentBank][evt.bit.NUM][currentChannel] = 
          analogRead(CV_INPUT) * PERCENTAGE_MULTIPLIER_10_BIT;
      }
      // Recurse
      else if (state.keyPressesSinceModHold == 3) {
        state.gateLengths[currentBank][evt.bit.NUM][currentChannel] = 0.5; // paranoia
        state.keyPressesSinceModHold = 0;
        return Grid::handleEditChannelVoltagesKeyEvent(evt, state);
      }
    }
  }

  // CV channel
  else {
    // MOD button is not being held, so edit voltage
    if (state.readyForModPress) { 
      state.selectedKeyForRecording = evt.bit.NUM;
      // See also continual recording in loop().
      state.voltages[currentBank][evt.bit.NUM][currentChannel] = analogRead(CV_INPUT);
    }
    // MOD button is being held
    else {
      // Clear states
      if (state.keyPressesSinceModHold == 0) {
        state.lockedVoltages[currentBank][evt.bit.NUM][currentChannel] = 0;
        state.activeSteps[currentBank][evt.bit.NUM][currentChannel] = 1;
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 0;
      }
      // Update mod + key tracking. This must occur after clearing state because it updates the 
      // value of keyPressesSinceModHold.
      state = Grid::updateModKeyCombinationTracking(evt, state);

      // Copy-paste voltage value, or restore step to defaults
      if (state.keyPressesSinceModHold == 1) {
        state = Grid::addKeyToCopyPasteData(evt, state);
      }      
      // Step is locked
      else if (state.keyPressesSinceModHold == 2) {
        state = State::quitCopyPasteFlowPriorToPaste(state);
        state.lockedVoltages[currentBank][evt.bit.NUM][currentChannel] = 1;
      }
      // Step is inactive
      else if (state.keyPressesSinceModHold == 3) {
        state.lockedVoltages[currentBank][evt.bit.NUM][currentChannel] = 0;
        state.activeSteps[currentBank][evt.bit.NUM][currentChannel] = 0;
      }
      // Step is random
      else if (state.keyPressesSinceModHold == 4) {
        state.activeSteps[currentBank][evt.bit.NUM][currentChannel] = 1;
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 1;
      }
      // Recurse
      else if (state.keyPressesSinceModHold == 5) {
        state.randomSteps[currentBank][evt.bit.NUM][currentChannel] = 0; // paranoia
        state.keyPressesSinceModHold = 0;
        return Grid::handleEditChannelVoltagesKeyEvent(evt, state);
      }
    }
  }

  return state;
}

State Grid::handleGlobalEditKeyEvent(keyEvent evt, State state) {
  uint8_t currentBank = state.currentBank;

  if (state.readyForModPress) { // MOD button is not being held, toggle removed step
    if (state.removedSteps[evt.bit.NUM]) {
      state.removedSteps[evt.bit.NUM] = 0;
    } 
    else {
      uint8_t totalRemovedSteps = 0;
      for (uint8_t i = 0; i < 16; i++) {
        if (state.removedSteps[i]) {
          totalRemovedSteps = totalRemovedSteps + 1;
        }
      }
      state.removedSteps[evt.bit.NUM] = totalRemovedSteps < 15 ? 1 : 0;
    }
  }
  // MOD button is being held
  else { 
    // Clear states for faster work flow.
    if (state.keyPressesSinceModHold == 0) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][evt.bit.NUM][i] = 0;
        state.activeSteps[currentBank][evt.bit.NUM][i] = 1;
      }
    }
    // Update mod + key tracking. This must occur after clearing state because it updates the 
    // value of keyPressesSinceModHold.
    state = Grid::updateModKeyCombinationTracking(evt, state);
    // Copy-paste
    if (state.keyPressesSinceModHold == 1) {
      state = Grid::addKeyToCopyPasteData(evt, state);
    } 
    // Toggle locked voltage
    else if (state.keyPressesSinceModHold == 2) {
      state = State::quitCopyPasteFlowPriorToPaste(state);
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][evt.bit.NUM][i] = 1;
      }
    }
    // Toggle active/inactive step
    else if (state.keyPressesSinceModHold == 3) {
      for (uint8_t i = 0; i < 8; i++) {
        state.lockedVoltages[currentBank][evt.bit.NUM][i] = 0;
        state.activeSteps[currentBank][evt.bit.NUM][i] = 0;
      }
    }
    // Recurse
    else if (state.keyPressesSinceModHold == 4) {
      for (uint8_t i = 0; i < 8; i++) {
        state.activeSteps[currentBank][evt.bit.NUM][i] = 1;
      }
      state.keyPressesSinceModHold = 0;
      return Grid::handleGlobalEditKeyEvent(evt, state);
    }
  }
  return state;
}

State Grid::handleSectionSelectKeyEvent(keyEvent evt, State state) {
  bool const modButtonIsBeingHeld = !state.readyForModPress;
  switch (Utils::keyQuadrant(evt.bit.NUM)) {
    case QUADRANT.INVALID:
      state.screen = SCREEN.ERROR;
      break;
    case QUADRANT.NW:
      if (modButtonIsBeingHeld) {
        // TODO: Load module
      } else {
        state = Nav::goForward(state, SCREEN.EDIT_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.NE:
      if (modButtonIsBeingHeld) {
        state.initialKeyPressedDuringModHold = evt.bit.NUM;
        bool const writeSuccess = State::writeModuleAndBankToSDCard(state);
        if (!writeSuccess) {
          state = Nav::goForward(state, SCREEN.ERROR); // do something else less drastic here?
        } else {
          // TODO: do something visual to confirm the write
        }
      } else {
        state = Nav::goForward(state, SCREEN.RECORD_CHANNEL_SELECT);
      }
      break;
    case QUADRANT.SW:
      if (modButtonIsBeingHeld) {
        // TODO: Calibration? Is this needed?
      } else {
        state = Nav::goForward(state, SCREEN.GLOBAL_EDIT);
      }
      break;
    case QUADRANT.SE:
      if (modButtonIsBeingHeld) {
        // TODO: Custom colors?
      } else {
        state = Nav::goForward(state, SCREEN.BANK_SELECT);
      }
      break;
  }
  return state;
}

State Grid::handleRecordChannelSelectKeyEvent(keyEvent evt, State state) {
  uint8_t currentBank = state.currentBank;
  if (!state.readyForModPress) { // MOD button is being held
    state.initialKeyPressedDuringModHold = evt.bit.NUM;
    if (evt.bit.NUM > 7) { 
      state.readyForRandom = !state.readyForRandom;
    } 
    else {
      state.currentChannel = evt.bit.NUM;
      if (state.readyForRandom) {
        state.randomInputChannels[currentBank][evt.bit.NUM] = 
          !state.randomInputChannels[currentBank][evt.bit.NUM];
      }
      state.autoRecordEnabled = !state.autoRecordEnabled;
      if (!state.autoRecordEnabled) {
        state.lastFlashToggle = millis();
        state.flash = 0;
      }
    }
  }
  else if (evt.bit.NUM > 7) { 
    return state;
  }
  else if (!state.lockedVoltages[state.currentBank][state.currentStep][evt.bit.NUM]) {
    state.currentChannel = evt.bit.NUM;
    state.selectedKeyForRecording = evt.bit.NUM;
    state.voltages[state.currentBank][state.currentStep][evt.bit.NUM] 
      = analogRead(CV_INPUT);
  }
  return state;
}

State Grid::handleStepChannelSelectKeyEvent(keyEvent evt, State state) {
  state.currentChannel = evt.bit.NUM;
  state = Nav::goBack(state);
  return state;
}

State Grid::handleStepSelectKeyEvent(keyEvent evt, State state) {
  if (!state.readyForModPress) { // MOD button is being held
    state.initialKeyPressedDuringModHold = evt.bit.NUM;
    state.selectedKeyForRecording = evt.bit.NUM;
    state.voltages[state.currentBank][state.currentStep][state.currentChannel] = analogRead(CV_INPUT);
  } 
  else {
    state.currentStep = evt.bit.NUM;
  }
  return state;
}

/**
 * @brief This function updates the keyPressesSinceModHold count only if this is the first key 
 * pressed or the same key as the first is pressed. If another key other than the first is pressed,
 * no update of keyPressesSinceModHold occurs.
 * 
 * @param evt 
 * @param state 
 * @return State 
 */
State Grid::updateModKeyCombinationTracking(keyEvent evt, State state) {
  // MOD button is being held
  if (!state.readyForModPress) {
    // this is the first key to be pressed
    if (state.initialKeyPressedDuringModHold < 0) { 
      state.initialKeyPressedDuringModHold = evt.bit.NUM;
      state.keyPressesSinceModHold = 1;
    }
    // initial key is pressed repeatedly
    else if (state.initialKeyPressedDuringModHold == evt.bit.NUM) {
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
