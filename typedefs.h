/**
 * Recollections: Type Definitions
 *
 * Copyright 2022 William Edward Fisher.
 */

#include <inttypes.h>

#ifndef RECOLLECTIONS_TYPEDEFS_H_
#define RECOLLECTIONS_TYPEDEFS_H_

/**
 * The screens of the module. See constants.h.
 */
typedef uint8_t Screen_t;

/**
 * The quadrants of the 16 buttons. See constants.h.
 */
typedef uint8_t Quadrant_t;

/**
 * Any color expressed as three 8-bit values such as [255, 255, 255].
 * The indices are [red, green, blue].
 */
typedef uint8_t RGBColorArray_t[3];

#endif
