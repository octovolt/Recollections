/**
 * Recollections: Navigation
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"
#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_NAV_H_
#define VOLTAGE_MEMORY_NAV_H_

typedef struct Nav {
  static State goBack(State state);
  static State goForward(State state, Screen_t screen);
} Nav;

#endif
