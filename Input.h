/**
 * Recollections: Input
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"

#ifndef RECOLLECTIONS_INPUT_H_
#define RECOLLECTIONS_INPUT_H_

typedef struct Input {
  static State handleInput(unsigned long loopStartTime, State state);
  static State handleAdvInput(unsigned long loopStartTime, State state);
  static State handleBankAdvanceInput(State state);
  static State handleBankReverseInput(State state);
  static State handleModButton(unsigned long loopStartTime, State state);
  static State handleRecInput(State state);
  static State handleResetInput(State state);
  static State handleReverseInput(State state);

  private:
  static State updateIsClocked(unsigned long lastInterval, State state);
} Input;

#endif