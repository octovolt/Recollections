/**
 * Recollections: Grid
 * 
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"
#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_GRID_H_
#define VOLTAGE_MEMORY_GRID_H_

typedef struct Grid {
  static State addKeyToCopyPasteData(keyEvent evt, State state);
  static State handleBankSelectKeyEvent(keyEvent evt, State state);
  static State handleEditChannelSelectKeyEvent(keyEvent evt, State state);
  static State handleEditChannelVoltagesKeyEvent(keyEvent evt, State state);
  static State handleGlobalEditKeyEvent(keyEvent evt, State state);
  static State handleSectionSelectKeyEvent(keyEvent evt, State state);
  static State handleRecordChannelSelectKeyEvent(keyEvent evt, State state);
  static State handleStepChannelSelectKeyEvent(keyEvent evt, State state);
  static State handleStepSelectKeyEvent(keyEvent evt, State state);
  static State updateModKeyCombinationTracking(keyEvent evt, State state);
} Grid;

#endif
