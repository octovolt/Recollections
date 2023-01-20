/**
 * Recollections: Advance
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"

#ifndef RECOLLECTIONS_ADVANCE_H_
#define RECOLLECTIONS_ADVANCE_H_

typedef struct Advance {
  static void advancePreset(unsigned long *loopStartTime, State *state);
  static State updateStateAfterAdvancing(unsigned long loopStartTime, State state);
} Advance;

#endif