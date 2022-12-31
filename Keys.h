/**
 * Recollections: Keys
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"
#include "typedefs.h"

#ifndef RECOLLECTIONS_KEYS_H_
#define RECOLLECTIONS_KEYS_H_

typedef struct Keys {
  static State handleKeyEvent(keyEvent evt, State state);
  private:
  static State addKeyToCopyPasteData(uint8_t key, State state);
  static State carryRestsToInactivePresets(uint8_t key, State state);
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
} Keys;

#endif
