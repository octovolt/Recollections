/**
 * Copyright 2022 William Edward Fisher.
 */
#include "Utils.h"

#include <Entropy.h>

#include "constants.h"

Quadrant_t Utils::keyQuadrant(uint8_t key) {
  if (key > 15) {
    Serial.println("Key is outside of range");
    return QUADRANT.INVALID;
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
uint16_t Utils::voltageValue(State state, uint8_t preset, uint8_t channel) {
  uint8_t currentBank = state.currentBank;

  // Gate channels
  if (state.gateChannels[currentBank][channel]) {
    if (!state.config.randomOutputOverwrites && state.randomVoltages[currentBank][preset][channel]) {
      return
        Entropy.random(2) &&
        millis() - state.lastAdvReceivedTime[0] < state.gateMillis
        ? VOLTAGE_VALUE_MAX
        : 0;
    }
    return
      state.gateVoltages[currentBank][preset][channel] &&
      millis() - state.lastAdvReceivedTime[0] < state.gateMillis
        ? VOLTAGE_VALUE_MAX
        : 0;
  }

  // Inactive presets within CV channels
  if (!state.activeVoltages[currentBank][preset][channel]) {
    // To get the voltage for an inactive preset, we need to find the last active preset, even if that
    // means wrapping around the sequence.
    for (uint8_t i = 1; i < 15; i++) {
      int8_t priorPreset = preset - i;
      uint8_t candidatePreset = priorPreset >= 0 ? priorPreset : 16 + priorPreset;
      if (candidatePreset == preset) {
        continue;
      }
      else if (state.activeVoltages[currentBank][candidatePreset][channel])
      {
        return Utils::outputControlVoltageValue(state, candidatePreset, channel);
      }
    }
  }

  // Default CV channel behavior
  return Utils::outputControlVoltageValue(state, preset, channel);
}

//--------------------------------------- PRIVATE --------------------------------------------------

uint16_t Utils::outputControlVoltageValue(State state, uint8_t preset, uint8_t channel) {
  uint8_t currentBank = state.currentBank;
  if (
    !state.config.randomOutputOverwrites &&
    (state.randomOutputChannels[currentBank][channel] ||
      state.randomVoltages[currentBank][preset][channel])
  ) {
    return Entropy.random(MAX_UNSIGNED_10_BIT);
  }
  return state.voltages[currentBank][preset][channel];
}
