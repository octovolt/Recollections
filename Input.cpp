/**
 * Copyright 2022 William Edward Fisher.
 */

#include "Input.h"

#include "Advance.h"
#include "Nav.h"
#include "Utils.h"
#include "constants.h"

State Input::handleInput(unsigned long loopStartTime, State state) {
  // hmmmm... why doesn't a function composition approach work here? pointers or something?
  state = Input::handleModButton(loopStartTime, state);
  state = Input::handleResetInput(state);
  state = Input::handleBankReverseInput(state);
  state = Input::handleBankAdvanceInput(state);
  state = Input::handleReverseInput(state);
  state = Input::handleAdvInput(loopStartTime, state);
  state = Input::handleRecInput(state);
  return state;
}

State Input::handleAdvInput(unsigned long loopStartTime, State state) {
  if (state.readyForAdvInput && !digitalRead(ADV_INPUT)) {
    state.readyForAdvInput = false;

    // if ( // protect against overflow
    //   !(loopStartTime > state.lastAdvReceivedTime[0] &&
    //   state.lastAdvReceivedTime[0] > state.lastAdvReceivedTime[1] &&
    //   state.lastAdvReceivedTime[1] > state.lastAdvReceivedTime[2])
    // ) {
    //   Serial.println("Overflow protection");
    //   if (loopStartTime < 3) {
    //     loopStartTime = 3;
    //   }
    //   state.lastAdvReceivedTime[0] = loopStartTime - 1;
    //   state.lastAdvReceivedTime[1] = loopStartTime - 2;
    //   state.lastAdvReceivedTime[2] = loopStartTime - 3;
    // }

    uint16_t lastInterval = loopStartTime - state.lastAdvReceivedTime[0];
    state.isAdvancingPresets = lastInterval < state.config.isAdvancingPresetsMaxInterval;
    state = Input::updateIsClocked(lastInterval, state);

    if (state.config.randomOutputOverwrites) {
      // set random output voltages of next preset before advancing
      state = State::setRandomVoltagesForPreset(state.currentPreset + 1, state);
    }

    Advance::advancePreset(&loopStartTime, &state);
    state = Advance::updateStateAfterAdvancing(loopStartTime, state);
  }
  else if (!state.readyForAdvInput && digitalRead(ADV_INPUT)) {
    state.readyForAdvInput = true;
  }

  return state;
}

State Input::handleBankAdvanceInput(State state) {
  if (state.readyForBankAdvanceInput && digitalRead(BANK_ADV_INPUT)) {
    Serial.println("BANK ADV input");
    state.readyForBankAdvanceInput = false;
    if (-15 > state.advanceBankAddend || state.advanceBankAddend > 15) {
      Serial.println("advanceBankAddend out of range, resetting it to 1");
      state.advanceBankAddend = 1;
    }
    int8_t advancedBank = state.currentBank + state.advanceBankAddend;
    state.currentBank =
      advancedBank > 15
        ? advancedBank - 16
        : advancedBank < 0
          ? advancedBank + 16
          : advancedBank;
  }
  else if (!state.readyForBankAdvanceInput && !digitalRead(BANK_ADV_INPUT)) {
    state.readyForBankAdvanceInput = true;
  }
  return state;
}

State Input::handleBankReverseInput(State state) {
  if (state.readyForBankReverseInput && digitalRead(BANK_REV_INPUT)) {
    Serial.println("BANK REV input");
    state.readyForBankReverseInput = false;
    state.advanceBankAddend = state.advanceBankAddend * -1;
  }
  else if (!state.readyForBankReverseInput && !digitalRead(BANK_REV_INPUT)) {
    state.readyForBankReverseInput = true;
  }
  return state;
}

State Input::handleModButton(unsigned long loopStartTime, State state) {
  // long press handling
  if (
    !state.readyForModPress &&
    state.initialModHoldKey < 0 &&
    loopStartTime - state.lastModPressTime > LONG_PRESS_TIME
  ) {
    state.initialModHoldKey = 69; // faking this to prevent immediate navigation back
    if (state.screen == SCREEN.PRESET_SELECT) {
      state = Nav::goForward(state, SCREEN.PRESET_CHANNEL_SELECT);
    } else if (
      state.screen == SCREEN.EDIT_CHANNEL_VOLTAGES ||
      state.screen == SCREEN.GLOBAL_EDIT
    ) {
      state.readyForPresetSelection = true;
    }
    return state;
  }

  // When MOD_INPUT is low, the button is being pressed.
  // We have a debounce scheme here with the readyForModPress flag. Once the button is pressed, we
  // say we are not readyForModPress until the button is released and the debounce time has elapsed.
  if (state.readyForModPress && !digitalRead(MOD_INPUT)) {
    state.readyForModPress = false;
    state.lastModPressTime = loopStartTime;
    return state;
  }

  // When MOD_INPUT is high, the button is no longer being pressed.
  // When the button is not being pressed, but we are still not ready for a new press, and the
  // debounce time has elapsed, we then clear the readyForModPress state to become ready for a new
  // button press. Note that if loopStartTime overflows and starts over at zero, this will be
  // treated as if the debounce time has elapsed. In theory, this would only happen if the program
  // was running for over 50 days.
  if (
    !state.readyForModPress && digitalRead(MOD_INPUT) &&
    (
      (loopStartTime - state.lastModPressTime > MOD_DEBOUNCE_TIME) ||
      loopStartTime < state.lastModPressTime
    )
  ) {
    if (state.initialModHoldKey >= 0) {
      state.initialModHoldKey = -1;
      state.keyPressesSinceModHold = 0;
      if (state.selectedKeyForCopying >= 0) {
        state = State::paste(state);
      }
    }
    else if (state.screen == SCREEN.SECTION_SELECT && state.readyToSave) {
      state.readyToSave = false;
    }
    else if (state.screen == SCREEN.PRESET_SELECT) {
      state = Nav::goForward(state, SCREEN.SECTION_SELECT);
    }
    else if (state.readyForPresetSelection) {
      state.readyForPresetSelection = false;
    }
    else {
      state = Nav::goBack(state);
    }
    state.readyForModPress = true;
  }
  return state;
}

State Input::handleRecInput(State state) {
  if (state.readyForRecInput && !digitalRead(REC_INPUT)) {
    state.readyForRecInput = false;
  }
  else if (!state.readyForRecInput && digitalRead(REC_INPUT)) {
    state.readyForRecInput = true;
  }
  return state;
}

State Input::handleResetInput(State state) {
  if (state.readyForResetInput && digitalRead(RESET_INPUT)) {
    Serial.println("RESET input");
    state.readyForResetInput = false;
    state.currentPreset = 0;
  }
  else if (!state.readyForResetInput && !digitalRead(RESET_INPUT)) {
    state.readyForResetInput = true;
  }
  return state;
}

State Input::handleReverseInput(State state) {
  if (state.readyForReverseInput && digitalRead(REV_INPUT)) {
    Serial.println("REV input");
    state.readyForReverseInput = false;
    state.advancePresetAddend = state.advancePresetAddend * -1;
  }
  else if (!state.readyForReverseInput && !digitalRead(REV_INPUT)) {
    state.readyForReverseInput = true;
  }
  return state;
}

// Private

State Input::updateIsClocked(unsigned long lastInterval, State state) {
  uint16_t avgInterval =
    ((state.lastAdvReceivedTime[0] - state.lastAdvReceivedTime[1]) +
    (state.lastAdvReceivedTime[1] - state.lastAdvReceivedTime[2])) * 0.5;
  uint16_t toleranceMillis = avgInterval * state.config.isClockedTolerance;
  signed long signedLastInterval = lastInterval;
  state.isClocked =
    !(signedLastInterval > (avgInterval + toleranceMillis) ||
      signedLastInterval < (avgInterval - toleranceMillis));
  return state;
}