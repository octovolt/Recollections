/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Utils.h"

#include "constants.h"

Quadrant_t Utils::keyQuadrant(uint8_t key) {
  if (key > 15) {
    // todo: error handling
  }
  if (key < 2 || (key > 3 && key < 6)) {
    return QUADRANT.NW;
  } else if (key < 8) {
    return QUADRANT.NE;
  } else if (key < 10 || (key > 11 && key < 14)) {
    return QUADRANT.SW;
  } else {
    return QUADRANT.SE;
  }
}

uint32_t Utils::random() {
  // For random number generation on Teensy 3.6.
  // See: https://forum.pjrc.com/threads/48745-Teensy-3-6-Random-Number-Generator
  // See also setup() in VoltageMemory.ino and constant.h.
  RNG_CR |= RNG_CR_GO_MASK;
  // while((RNG_SR & RNG_SR_OREG_LVL(0xF)) == 0); // wait -- from orignal sketch. just hangs; does not work.
  return RNG_OR;
}

uint16_t Utils::tenBitToTwelveBit(uint16_t n) {
  if (n > MAX_UNSIGNED_10_BIT) {
    Serial.println("invalid 10-bit integer");
    return 0;
  }
  else if (n == 0) {
    return 0;
  }
  else if (n == MAX_UNSIGNED_10_BIT) {
    return MAX_UNSIGNED_12_BIT;
  }
  return n << 2;
}

// TODO: create unit tests for this
uint16_t Utils::voltageValueForStep(State state, uint8_t step, uint8_t channel) {
  uint8_t currentBank = state.currentBank;

  // Gate channels
  if (state.gateChannels[currentBank][channel]) {
    return  
      state.activeSteps[currentBank][step][channel] && 
      millis() - state.lastAdvReceived < state.gateMillis
        ? VOLTAGE_VALUE_MAX 
        : 0;
  }

  // Inactive steps within CV channels
  if (!state.activeSteps[currentBank][step][channel]) {
    // To get the voltage for an inactive step, we need to find the last active step, even if that
    // means wrapping around the sequence.
    for (uint8_t i = 0; i < 15; i++) {
      int8_t offset = step - i;
      uint8_t candidateStep = (offset >= 0) ? offset : (16 + offset);
      if (candidateStep == step) {
        continue;
      } 
      else if (state.activeSteps[currentBank][step][channel]) 
      {
        return state.voltages[currentBank][candidateStep][channel];
      }
    }
  }

  // Default CV channel behavior
  return state.voltages[currentBank][step][channel];
}