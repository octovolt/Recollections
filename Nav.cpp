/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Nav.h"

#include <Adafruit_NeoTrellis.h>
#include "Hardware.h"
#include "Utils.h"
#include "constants.h"

State Nav::goBack(State state) {
  state.navHistoryIndex = state.navHistoryIndex - 1;
  if (state.navHistoryIndex < 0) {
    Serial.println("Attempting to go back past the earliest step in the navHistory.");
    state.navHistoryIndex = 0;
    state.mode = MODE.ERROR;
  } else {
    state.mode = state.navHistory[state.navHistoryIndex];
  }
  return state;
}

State Nav::goForward(State state, Mode_t mode) {
  state.navHistoryIndex = state.navHistoryIndex + 1;
  if (state.navHistoryIndex > 3) {
    Serial.println("Attempting to go forward past the maximum step in the navHistory.");
    state.navHistoryIndex = 3;
    state.mode = MODE.ERROR;
  } else {
    state.mode = mode;
    state.navHistory[state.navHistoryIndex] = state.mode;
  }
  
  return state;
}

