/* ----------------------------   String utilities for nRF52832   ------------------------------ */
/*  File      -  String utilities header file                                                    */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

#ifndef _UTIL_STRING_H_
#define _UTIL_STRING_H_

/******************************************   INCLUDES   *****************************************/
#include <stdint.h>
#include <stdbool.h>

/*************************************   PUBLIC FUNCTIONS   **************************************/
/**
 * @brief s8StringCompare Compares two strings on a character by character basis.
 *
 * @warning This function takes two data buffers as input. Failing to provide two valid strings
 *          will yield an erroneous result.
 *
 * @param pu8String1 Pointer to first string.
 * @param pu8String2 Pointer to second string.
 * @param u8Length Number of characters to compare.
 *
 * @return int8_t 2 if input is invalid, 1 if first string is larger, -1 if second string is larger
 *         and 0 if both strings are equal.
 */
int8_t s8StringCompare(const uint8_t *pu8String1, const uint8_t *pu8String2, uint8_t u8Length);

/**
 * @brief bIsAllNumerals Checks whether all characters in a string are numerals.
 *
 * @warning This function takes a data buffer as input. Failing to provide a valid string will
 *          yield an erroneous result.
 *
 * @param pu8String Pointer to string.
 * @param u8Length Number of characters to check.
 *
 * @return bool true if all characters in string are numerals, false otherwise.
 */
bool bIsAllNumerals(const uint8_t *pu8String, uint8_t u8Length);

/**
 * @brief bIsAllLowCaseAlpha Checks whether all characters in a string are low case alphabeticals.
 *
 * @note This function is case-sensitive.
 *
 * @warning This function takes a data buffer as input. Failing to provide a valid string will
 *          yield an erroneous result.
 *
 * @param pu8String Pointer to string.
 * @param u8Length Number of characters to check.
 *
 * @return bool true if all characters in string are low case alphabeticals, false otherwise.
 */
bool bIsAllLowCaseAlpha(const uint8_t *pu8String, uint8_t u8Length);

#endif /* _UTIL_STRING_H_ */