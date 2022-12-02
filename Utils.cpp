/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Utils.h"

#include <Entropy.h>

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
    if (!state.config.randomOutputOverwritesSteps && state.randomSteps[currentBank][step][channel]) {
      return
        Entropy.random(2) &&
        millis() - state.lastAdvReceived[0] < state.gateMillis
        ? VOLTAGE_VALUE_MAX
        : 0;
    }
    return
      state.gateSteps[currentBank][step][channel] &&
      millis() - state.lastAdvReceived[0] < state.gateMillis
        ? VOLTAGE_VALUE_MAX
        : 0;
  }

  // Inactive steps within CV channels
  if (!state.activeSteps[currentBank][step][channel]) {
    // To get the voltage for an inactive step, we need to find the last active step, even if that
    // means wrapping around the sequence.
    for (uint8_t i = 1; i < 15; i++) {
      int8_t priorStep = step - i;
      uint8_t candidateStep = priorStep >= 0 ? priorStep : 16 + priorStep;
      if (candidateStep == step) {
        continue;
      }
      else if (state.activeSteps[currentBank][candidateStep][channel])
      {
        return Utils::outputControlVoltageValueForStep(state, candidateStep, channel);
      }
    }
  }

  // Default CV channel behavior
  return Utils::outputControlVoltageValueForStep(state, step, channel);
}

//--------------------------------------- PRIVATE --------------------------------------------------

uint16_t Utils::outputControlVoltageValueForStep(State state, uint8_t step, uint8_t channel) {
  uint8_t currentBank = state.currentBank;
  if (
    !state.config.randomOutputOverwritesSteps &&
    (state.randomOutputChannels[currentBank][channel] ||
      state.randomSteps[currentBank][step][channel])
  ) {
    return Entropy.random(MAX_UNSIGNED_10_BIT);
  }
  return state.voltages[currentBank][step][channel];
}
