/* -----------------------------   Math utilities for nRF52832   ------------------------------- */
/*  File      -  Computational core header file                                                  */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _UTIL_MATH_H_
#define _UTIL_MATH_H_

/******************************************   INCLUDES   *****************************************/
#include <stdint.h>

/***************************************   PUBLIC MACROS   ***************************************/
#define MODULUS(x) ((x >= 0)?x:-x)

/*************************************   PUBLIC FUNCTIONS   **************************************/
/**
 * @brief s32Power Computes the outcome of an integer base raised to the power of a given integer
 *        exponent.
 *
 * @warning This function has a computational ceiling dictated by the upper bound of int32_t
 *          representable integers.
 *
 * @param u8Base Power base.
 * @param u8Exponent Power exponent.
 *
 * @return int32_t u8Base raised to the power of u8Exponent if it doesn't exceed 0x7FFFFFFF,
 *         0xFFFFFFFF otherwise.
 */
int32_t s32Power(uint8_t u8Base, uint8_t u8Exponent);

/**
 * @brief u8DigitCount Computes the number of digits contained in a given integer.
 *
 * @param u32Integer Integer whose number of digits is to be computed.
 *
 * @return uint8_t Number of digits in the given integer.
 */
uint8_t u8DigitCount(uint32_t u32Integer);

#endif /* _UTIL_MATH_H_ */