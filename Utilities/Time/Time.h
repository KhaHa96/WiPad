/* -----------------------------   Time utilities for nRF52832   ------------------------------- */
/*  File      -  Time utilities header file                                                      */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

#ifndef _UTIL_TIME_H_
#define _UTIL_TIME_H_

/******************************************   INCLUDES   *****************************************/
#include <stdint.h>
#include "system_config.h"
#include "ble_cts_c.h"

/*************************************   PUBLIC FUNCTIONS   **************************************/
/**
 * @brief u32TimeToEpoch Converts a given Gregorian calendar date to a Unix timestamp.
 *
 * @note This function is based on Edward Graham Richards' algorithm to compute a Gregorian
 *       calendar date out of a Julian day number, but instead it does the exact opposite. Given a
 *       Gregorian calendar date, this algorithm will compute the equivalent Julian day number,
 *       take away the Unix timestamp base date converted to a Julian calendar date, and account
 *       for the extra fraction of a day if any.
 *
 * @param pstrTime Pointer to structure containing the Gregorian calendar date.
 *
 * @return uint32_t Unix timestamp corresponding to the given Gregorian calendar date if input is
 *         sound, 0 otherwise.
 */
uint32_t u32TimeToEpoch(exact_time_256_t *pstrTime);

#endif /* _UTIL_TIME_H_ */