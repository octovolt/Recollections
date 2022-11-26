/**
 * Recollections: State
 * 
 * Copyright 2022 William Edward Fisher.
 */

#include <ArduinoJson.h>
#include <StreamUtils.h>

#include "Config.h"
#include "constants.h"
#include "typedefs.h"

#ifndef VOLTAGE_MEMORY_STATE_H_
#define VOLTAGE_MEMORY_STATE_H_

/**
 * The sole state object. All state goes here, nowhere else.
 */
typedef struct State {
  /** Global config. Values here should very rarely change. Initial values provided in setup(). */
  Config config;

  /** Current operating screen. See constants.h. */
  Screen_t screen;

  /** Array to track navigational history. Used to restore previous step in navigation. */
  Screen_t navHistory[4];

  /** The current index within the navHistory. */
  uint8_t navHistoryIndex;

  /** 
   * Flag to track whether the persisted state has changed and we need to eventually write to the 
   * SD card. TODO: clean this up? Not sure about this.
   */
  // bool persistedStateChanged;

  /** Flag to track if we are waiting for a gate signal on the REC input. */
  bool autoRecordEnabled;

  /** Flag to track whether we should respond to a clock/gate/trigger on the ADV input. */
  bool readyForAdvInput;

  /** Flag to track whether we should respond to a clock/gate/trigger on the REC input. */
  bool readyForRecInput;
  
  /** Flag to track whether we should respond to key press. */
  bool readyForKeyPress;
  
  /** Flag to track whether we should respond to MOD press. */
  bool readyForModPress;

  /** 
   * Flag to track whether we are waiting for a channel to be selected or deselected for random 
   * input or output.
   */
  bool readyForRandom;

  /** 
   * Count the number of flashes to determine if enough time has elapsed to where a new random 
   * color should be rendered. This number will update regardless of whether any step
   * or channel has been set to utilize randomization.
   */
  uint8_t flashesSinceRandomColorChange;

  /** 
   * Flag to track if we should change colors. This flag will update regardless of whether any step
   * or channel has been set to utilize randomization.
   */
  bool randomColorShouldChange;

  /** 
   * Flag to track whether we are currently flashing a key on or off. Default is on. This flag will 
   * update regardless of whether any key is currently required to flash.
   */
  bool flash;

  /** 
   * When flashing a key, this is the last time it flashed on or off. This number will update 
   * regardless of whether any key is currently required to flash.
   */
  unsigned long lastFlashToggle;

  /** Time in ms since the last time a clock/gate/trigger was received at the ADV input. */
  unsigned long lastAdvReceived;

  /** Time in ms since last MOD button press. */
  unsigned long timeModPressed;

  /** 
   * Time in ms since last key release. We track the time of key release and write to SD after no 
   * key has been released for WRITE_AFTER_KEY_RELEASE_TIME. 
   */
  unsigned long timeKeyReleased;

  /** 
   * Which key was initially pressed while holding the MOD button. 
   * This is also how we track whether *any* key was pressed while holding the MOD button.
   * A negative number indicates no key was pressed, or has been pressed yet. 
   */
  int8_t initialKeyPressedDuringModHold;

  /** 
   * The number of times the initialKeyPressedDuringModHold has been pressed since the MOD button 
   * was held down. This is used to cycle through various outcomes of the MOD + key combination.
   */
  uint8_t keyPressesSinceModHold;

  /** 
   * The average between the last two times a clock/gate/trigger was received at the ADV input. 
   * This is used to calculate the gate length when a channel is configured to send gates.
   */
  unsigned long gateMillis;
  
  /** Current step, 0-15. */
  uint8_t currentStep;
  
  /** Current bank, 0-15. */
  uint8_t currentBank;
  
  /** Current selected output channel, 0-7. */
  int8_t currentChannel;

  /** 
   * Current selected step for recording, 0-15. This is used to continually record while a key is 
   * held down. A value below zero denotes that no step is selected; no key is pressed or we are not
   * in a recording screen.
   */
  int8_t selectedKeyForRecording;

  /** Keys representing banks, channels, channel steps or global steps that will be copied. */
  int8_t selectedKeyForCopying;

  /** Keys representing banks, channels, channel steps or global steps that will be pasted. */
  bool pasteTargetKeys[16];

  /** 
   * If a step is not active, its voltage value will be ignored in favor of the last previous active
   * step. There must always be at least one active step.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][step][channel].
   */
  bool activeSteps[16][16][8];

  /** 
   * The steps that will produce gates when the steps are sequenced via triggers on the ADV_INPUT 
   * jack.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][step][channel].
   */
  bool gateSteps[16][16][8];

  /**
   * The steps that will produce a random value, either CV or gate.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][step][channel].
   */
  bool randomSteps[16][16][8];
  
  /** 
   * The steps that will be skipped entirely during sequencing. 
   * This is set in GLOBAL_EDIT screen. 
   */
  bool removedSteps[16];

  /** 
   * The channels where the voltage will be either 5v or 0v and the duration of 5v will be based on
   * the measured time between clock signals received at the ADV input.
   * Indices are [bank][channel]. 
   */
  bool gateChannels[16][8];

  /** 
   * The channels where the output voltage will be random.
   * Indices are [bank][channel]. 
   */
  bool randomOutputChannels[16][8];

  /** 
   * The channels where the input voltage will be random. This only applies to automatic recording.
   * Indices are [bank][channel]. 
   */
  bool randomInputChannels[16][8];

  /** 
   * The stored lengths as a percentage of either 0ms to 1000ms or the time between the last 
   * gate/trigger/clock receive at the ADV input and the most recent gate/trigger/clock received.
   * Indices are [bank][step][channel].
   */
  uint8_t gateLengths[16][16][8];

  /**
   * Voltages that cannot be changed in RECORD_CHANNEL_SELECT screen or through automatic recording.
   * Indices are [bank][step][channel]. 
   */
  bool lockedVoltages[16][16][8];
  
  /** 
   * 10-bit stored voltages for channels per step per bank (max value is 1023).
   * Indices are [bank][step][channel]. 
   */
  uint16_t voltages[16][16][8];

  // static methods

  /**
   * @brief Paste the voltages from one bank to a number of other banks, across all 16 steps and all
   * 8 channels.
   * 
   * @param state 
   * @return State 
   */
  static State pasteBanks(State state);

  /**
   * @brief Paste the voltages from one channel to a number of other channels, across all 16 steps.
   * 
   * @param state 
   * @return State 
   */
  static State pasteChannels(State state);

  /**
   * @brief Paste the voltage from one step to a number of other steps on the same channel.
   * 
   * @param state 
   * @return State 
   */
  static State pasteChannelSteps(State state);

  /**
   * @brief Paste the voltage from one step to a number of other steps, across all 8 channels.
   * 
   * @param state 
   * @return State 
   */
  static State pasteGlobalSteps(State state);

  /**
   * @brief Clean up state related to copy-paste.
   * 
   * @param state 
   * @return State 
   */
  static State quitCopyPasteFlowPriorToPaste(State state);

  /**
   * @brief Read the persisted state values from the SD card that pertain to the entire module and 
   * copy them into the returned state struct.
   * 
   * @param state
   * @param moduleFile 
   * @return State 
   */
  static State readModuleFromSDCard(State state);

    /**
   * @brief Read the persisted state values from the SD card that pertain to a specified bank and 
   * copy them into the returned state struct.
   * 
   * @param state
   * @param bankFile 
   * @return State 
   */
  static State readBankFromSDCard(State state, uint8_t bank);

  /**
   * @brief Get the persisted state values from the state struct and write them to the SD card.
   * Returns a bool value denoting whether the write was successful.
   * 
   * @param state 
   * @return true 
   * @return false 
   */
  static bool writeModuleAndBankToSDCard(State state);
 } State;

 #endif
