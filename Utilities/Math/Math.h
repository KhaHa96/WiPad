/* -----------------------------   Math utilities for nRF51422   ------------------------------- */
/*  File      -  Computational core header file                                                  */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/******************************************   INCLUDES   *****************************************/
#include <stdint.h>

/******************************************   DEFINES   ******************************************/
#define S32_UPPER_BOUND 0x7FFFFFFF
#define INVALID_RETURN  0xFFFFFFFF

/***************************************   PUBLIC MACROS   ***************************************/
#define MODULUS(x) ((x >= 0)?x:-x)

/*************************************   PUBLIC FUNCTIONS   **************************************/
/**
 * @brief u32Power Computes the outcome of an integer base raised to the power of a given integer
 *        exponent.
 *
 * @warning This function has a computational ceiling dictated by the upper bound of int32_t
 *          representable integers.
 *
 * @param u8Base Power base
 * @param u8Exponent Power exponent
 *
 * @return uint32_t u8Base raised to the power of u8Exponent if it doesn't exceed 0x7FFFFFFF,
 *         0xFFFFFFFF otherwise
 */
uint32_t u32Power(uint8_t u8Base, uint8_t u8Exponent);