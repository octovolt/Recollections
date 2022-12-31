/**
 * Recollections: State
 *
 * Copyright 2022 William Edward Fisher.
 */

#include "Config.h"
#include "constants.h"
#include "typedefs.h"

#ifndef RECOLLECTIONS_STATE_H_
#define RECOLLECTIONS_STATE_H_

/**
 * The sole state object. All state goes here, nowhere else.
 *
 * Many of the data structures in the state object are based on a 3D array with the following
 * indices or axes, in this order: [bank][preset][channel]. When we refer the value at the
 * intersection of these three axes, we refer to it as a "voltage", regardless of whether it is
 * an actual voltage value or a boolean.
 *
 * Some channel configurations affect every voltage on that channel. In these cases, the preset axis
 * is dropped and a 2D array of [bank][channel] is used instead.
 */
typedef struct State {
  /** Global config. Values here should very rarely change. Initial values provided in setup(). */
  Config config;

  /** Current screen presented on the 16 illuminated keys. See constants.h. */
  Screen_t screen;

  /** Array to track navigational history. Used to restore previous step in navigation. */
  Screen_t navHistory[4];

  /** The current index within the navHistory. */
  uint8_t navHistoryIndex;

  /** Current bank, 0-15. */
  uint8_t currentBank;

  /** Current preset, 0-15. */
  uint8_t currentPreset;

  /** Current selected output channel, 0-7. */
  uint8_t currentChannel;

  /**
   * The number of steps to advance when advancing through presets. Normally, this is 1. A value of
   * -1 would be used for reversing the direction of the sequence. However, in theory, this value
   * could be anything between -15 and 15.
   */
  int8_t advancePresetAddend;

  /**
   * The number of steps to advance when advancing through banks. Normally, this is 1. A value of
   * -1 would be used for reversing the direction of the sequence. However, in theory, this value
   * could be anything between -15 and 15.
   */
  int8_t advanceBankAddend;

  /** Flag to track whether we have recently received a gate or trigger on the ADV input. */
  bool isAdvancingPresets;

  /** Flag to track whether we are receiving regular gates or triggers on the ADV input. */
  bool isClocked;

  /** Flag to track whether to respond to a clock/gate/trigger on the ADV input. */
  bool readyForAdvInput;

  /** Flag to track whether to respond to a clock/gate/trigger on the REC input. */
  bool readyForRecInput;

  /** Flag to track whether to respond to a gate/trigger on the RESET input (expansion only). */
  bool readyForResetInput;

  /** Flag to track whether to respond to a gate/trigger on the REV input (expansion only). */
  bool readyForReverseInput;

  /** Flag to track whether to respond to a gate/trigger on the BANK ADV input (expansion only). */
  bool readyForBankAdvanceInput;

  /** Flag to track whether to respond to a gate/trigger on the BANK REV input (expansion only). */
  bool readyForBankReverseInput;

  /** Flag to track whether to respond to key press. */
  bool readyForKeyPress;

  /** Flag to track whether to respond to MOD press. */
  bool readyForModPress;

  /** Flag for the alternate preset selection flow in EDIT_CHANNEL_VOLTAGES and GLOBAL_EDIT */
  bool readyForPresetSelection;

  /**
   * Flag to track whether we should save the current bank on the next key press. This is
   * equivalent to an "Are you sure?" dialog in a computer application. The next key press will
   * confirm the user's intent to save.
   */
  bool readyToSave;

  /** Flag to track whether we are currently showing the visual save confirmation. */
  bool confirmingSave;

  /** Count the number of flashes since saving to help manage the visual save confirmation. */
  uint8_t flashesSinceSave;

  /**
   * Count the number of flashes to determine if enough time has elapsed to where a new random
   * color should be rendered. This number will update regardless of whether any preset
   * or channel has been set to utilize randomization.
   */
  uint8_t flashesSinceRandomColorChange;

  /**
   * Flag to track if we should change colors. This flag will update regardless of whether any
   * preset or channel has been set to utilize randomization.
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

  /**
   * Time in ms since the last time a clock/gate/trigger was received at the ADV input. We keep the
   * last three values to determine whether the module is advancing or being clocked, as well as how
   * long gates should be.
   */
  unsigned long lastAdvReceivedTime[3];

  /** Time in ms since last MOD button press. */
  unsigned long lastModPressTime;

  /**
   * Which key was initially pressed while holding the MOD button.
   * This is also how we track whether *any* key was pressed while holding the MOD button.
   * A negative number indicates no key was pressed, or no key has been pressed yet.
   */
  int8_t initialModHoldKey;

  /**
   * The number of times the initialModHoldKey has been pressed since the MOD button
   * was held down. This is used to cycle through various outcomes of the MOD + key combination.
   */
  uint8_t keyPressesSinceModHold;

  /**
   * The average between the last two times a clock/gate/trigger was received at the ADV input.
   * This is used to calculate the gate length when a channel is configured to send gates.
   */
  unsigned long gateMillis;

  /**
   * Current selected preset for recording, 0-15. This is used to continually record while a key is
   * held down. A value below zero denotes that no preset is selected; no key is pressed or we are
   * not in a recording screen.
   */
  int8_t selectedKeyForRecording;

  /** Keys representing banks, channels, presets or sets of presets to be copied. */
  int8_t selectedKeyForCopying;

  /** Keys representing banks, channels, presets or sets of presets to be  pasted. */
  bool pasteTargetKeys[16];

  /**
   * The presets that will be skipped entirely during sequencing.
   * This is set in GLOBAL_EDIT screen.
   */
  bool removedPresets[16];

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
   * If a voltage is not active, its value will be ignored in favor of the last previous
   * active voltage. There must always be at least one active voltage.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][preset][channel].
   */
  bool activeVoltages[16][16][8];

  /**
   * The voltages (aka "steps") that will produce gates on a specified channel.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][preset][channel].
   */
  bool gateVoltages[16][16][8];

  /**
   * The voltages (aka "steps") that will produce a random value, either CV or gate, on a specified
   * channel.
   * This is set in EDIT_CHANNEL_VOLTAGES screen.
   * Indices are [bank][preset][channel].
   */
  bool randomVoltages[16][16][8];

  /**
   * Voltages that cannot be changed in RECORD_CHANNEL_SELECT screen or through automatic recording.
   * Indices are [bank][preset][channel].
   */
  bool lockedVoltages[16][16][8];

  /**
   * 10-bit stored voltage values for channels per preset per bank (max value is 1023).
   * Indices are [bank][preset][channel].
   */
  uint16_t voltages[16][16][8];

  /**
   * Ephemeral cached voltage value for when we need to be able to get back to a voltage value
   * instead of overwriting it permanently. Note there is only one of these -- this is truly
   * ephemeral, and the ephemerality should be enforced. This is used in the situation where we have
   * navigated to random input but actually did not want to overwrite. The MOD button is still being
   * pressed so pressing the key again will navigate forward in the functionality cycle and restore
   * the cached voltage to the preset. TODO: check if this is needed for random output as well.
   */
  uint16_t cachedVoltage;

  /** For debugging */
  bool initialLoopCompleted;

  // ------------------------------- static methods ------------------------------------------------

  /**
   * @brief Record voltage on the channels set up for automatic recording.
   *
   * @param state
   * @return State
   */
  static State autoRecord(State state);

  /**
   * @brief Edit voltage for preset selected by hand.
   *
   * @param state
   * @return State
   */
  static State editVoltageOnSelectedPreset(State state);

  /**
   * @brief Record new voltage
   *
   * @param state
   * @return State
   */
  static State recordContinuously(State state);

  /**
   * @brief Record voltage for a channel selected by hand.
   *
   * @param state
   * @return State
   */
  static State recordVoltageOnSelectedChannel(State state);

  /**
   * @brief Universal entry point for all pastes.
   *
   * @param state
   * @return State
   */
  static State paste(State state);

  /**
   * @brief Paste the voltages from one bank to a number of other banks, across all 16 presets and
   * all 8 channels.
   *
   * @param state
   * @return State
   */
  static State pasteBanks(State state);

  /**
   * @brief Paste the voltages from one channel to a number of other channels, across all 16 presets.
   *
   * @param state
   * @return State
   */
  static State pasteChannels(State state);

  /**
   * @brief Paste the voltage from one preset to a number of other presets on the same channel.
   *
   * @param state
   * @return State
   */
  static State pasteVoltages(State state);

  /**
   * @brief Paste the voltage from one preset to a number of other presets, across all 8 channels.
   *
   * @param state
   * @return State
   */
  static State pastePresets(State state);

  /**
   * @brief Clean up state related to copy-paste.
   *
   * @param state
   * @return State
   */
  static State quitCopyPasteFlowPriorToPaste(State state);
 } State;

 #endif
