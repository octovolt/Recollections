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
    state.screen = SCREEN.ERROR;
  } else {
    state.screen = state.navHistory[state.navHistoryIndex];
  }
  return state;
}

State Nav::goForward(State state, Screen_t screen) {
  state.navHistoryIndex = state.navHistoryIndex + 1;
  if (state.navHistoryIndex > 3) {
    Serial.println("Attempting to go forward past the maximum step in the navHistory.");
    state.navHistoryIndex = 3;
    state.screen = SCREEN.ERROR;
  } else {
    state.screen = screen;
    state.navHistory[state.navHistoryIndex] = state.screen;
  }
  
  return state;
}

