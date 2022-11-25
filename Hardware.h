/**
 * Recollections: Hardware
 * 
 * Copyright 2022 William Edward Fisher.
 */

#include "State.h"
#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_HARDWARE_H_
#define VOLTAGE_MEMORY_HARDWARE_H_

typedef struct Hardware {
  static bool prepareRenderingOfRandomizedKey(State state, uint8_t key);
  static bool prepareRenderingOfStepGate(State state, uint8_t step);
  static bool prepareRenderingOfStepVoltage(State state, uint8_t step, uint32_t color);
  static bool reflectState(State state);
  static bool renderBankSelect(State state);
  static bool renderEditChannelSelect(State state);
  static bool renderEditChannelVoltages(State state);
  static bool renderError(State state);
  static bool renderGlobalEdit(State state);
  static bool renderSectionSelect(State state);
  static bool renderRecordChannelSelect(State state);
  static bool renderStepChannelSelect(State state);
  static bool renderStepSelect(State state);
  static bool setOutput(State state, const int8_t channel, const uint16_t voltageValue);
  static bool setOutputsAll(State state);
} Hardware;

#endif