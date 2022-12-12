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
  static State handleKeyEvent(keyEvent evt, State state);
  private:
  static State addKeyToCopyPasteData(uint8_t key, State state);
  static State handleBankSelectKeyEvent(uint8_t key, State state);
  static State handleEditChannelSelectKeyEvent(uint8_t key, State state);
  static State handleEditChannelVoltagesKeyEvent(uint8_t key, State state);
  static State handleGlobalEditKeyEvent(uint8_t key, State state);
  static State handleModuleSelectKeyEvent(uint8_t key, State state);
  static State handleRecordChannelSelectKeyEvent(uint8_t key, State state);
  static State handleSectionSelectKeyEvent(uint8_t key, State state);
  static State handlePresetChannelSelectKeyEvent(uint8_t key, State state);
  static State handlePresetSelectKeyEvent(uint8_t key, State state);
  static State updateModKeyCombinationTracking(uint8_t key, State state);
} Grid;

#endif
