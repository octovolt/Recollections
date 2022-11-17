/**
 * Recollections: Utils
 * 
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_NeoTrellis.h>
#include "State.h"
#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_UTILS_H_
#define VOLTAGE_MEMORY_UTILS_H_

typedef struct Utils {
  static Quadrant_t keyQuadrant(uint8_t key);
  static uint32_t random();
  static uint16_t tenBitToTwelveBit(uint16_t n);
  static uint16_t voltageValueForStep(State state, uint8_t step, uint8_t channel);
} Utils;

#endif