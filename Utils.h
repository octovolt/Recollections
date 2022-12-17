/**
 * Recollections: Utils
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <Adafruit_NeoTrellis.h>
#include "State.h"
#include "typedefs.h"

#ifndef RECOLLECTIONS_UTILS_H_
#define RECOLLECTIONS_UTILS_H_

typedef struct Utils {
  static Quadrant_t keyQuadrant(uint8_t key);
  static uint16_t tenBitToTwelveBit(uint16_t n);
  static uint16_t voltageValue(State state, uint8_t preset, uint8_t channel);

  private:
  static uint16_t outputControlVoltageValue(State state, uint8_t preset, uint8_t channel);
} Utils;

#endif