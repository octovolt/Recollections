/**
 * Recollections: Hardware
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"
#include "typedefs.h"

#ifndef RECOLLECTIONS_HARDWARE_H_
#define RECOLLECTIONS_HARDWARE_H_

typedef struct Hardware {
  static bool reflectState(State state);
  static State updateFlashTiming(unsigned long loopStartTime, State state);

  private:
  static bool prepareRenderingOfChannelEditGateKey(State state, uint8_t preset);
  static bool prepareRenderingOfChannelEditVoltageKey(State state, uint8_t preset);
  static bool prepareRenderingOfKey(State state, uint8_t key, uint8_t rgbColor[]);
  static bool prepareRenderingOfRandomizedKey(State state, uint8_t key);
  static bool renderBankSelect(State state);
  static bool renderEditChannelSelect(State state);
  static bool renderEditChannelVoltages(State state);
  static bool renderError(State state);
  static bool renderGlobalEdit(State state);
  static bool renderModuleSelect(State state);
  static bool renderRecordChannelSelect(State state);
  static bool renderSectionSelect(State state);
  static bool renderPresetChannelSelect(State state);
  static bool renderPresetSelect(State state);
  static bool setOutput(State state, const int8_t channel, const uint16_t voltageValue);
  static bool setOutputsAll(State state);
} Hardware;

#endif