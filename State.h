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

  /** Flag to track whether we have recently received a gate or trigger on the ADV input. */
  bool isAdvancing;

  /** Flag to track whether we are receiving regular gates or triggers on the ADV input. */
  bool isClocked;

  /** Flag to track whether we should respond to a clock/gate/trigger on the ADV input. */
  bool readyForAdvInput;

  /** Flag to track whether we should respond to a clock/gate/trigger on the REC input. */
  bool readyForRecInput;

  /** Flag to track whether we should respond to key press. */
  bool readyForKeyPress;

  /** Flag to track whether we should respond to MOD press. */
  bool readyForModPress;

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

  /** Time in ms since the last time a clock/gate/trigger was received at the ADV input. We keep the
   * last three values to determine whether the module is advancing or being clocked, as well as how
   * long gates should be.
   */
  unsigned long lastAdvReceived[3];

  /** Time in ms since last MOD button press. */
  unsigned long timeModPressed;

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
   * The channels that will sample the incoming voltage when a gate or trigger is received on the
   * REC input. The incoming voltage could be from the CV input, the internal voltage source, or if
   * the channel is in the list of randomInputChannels, a randomly generated voltage value.
   * Indices are [bank][channel].
   */
  bool autoRecordChannels[16][8];

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
  static State pasteChannelStepVoltages(State state);

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
   * @brief Make sure we have the correct path of directories set up on the SD card, or else create
   * these directories. This is required to create a file.
   *
   * @param state
   */
  static void confirmOrCreatePathOnSDCard(State state);

  /**
   * @brief Read an entirely new module from the SD card, reading from both Module.txt and all the
   * Bank_n.txt files within a new Module_n directory, so an entirely new set of 16 banks becomes
   * available. Creat the directory structure and files if they do not yet exist.
   *
   * @param state
   * @return State
   */
  static State readModuleFromSDCard(State state);

  /**
   * @brief Read the persisted state values from the Module.txt file on the SD card. Create the
   * file if it does not yet exist.
   *
   * @param state
   * @return State
   */
  static State readModuleFileFromSDCard(State state);

  /**
   * @brief Read the persisted state values from one of the Bank_n.txt fils the SD card. Create the
   * file if it does not yet exist.
   *
   * @param state
   * @param bank
   * @return State
   */
  static State readBankFileFromSDCard(State state, uint8_t bank);

  /**
   * @brief Get the persisted state values from the state struct and write them to the SD card.
   * Returns a bool value denoting whether the write was successful.
   *
   * @param state
   * @return true
   * @return false
   */
  static bool writeCurrentModuleAndBankToSDCard(State state);
 } State;

 #endif
