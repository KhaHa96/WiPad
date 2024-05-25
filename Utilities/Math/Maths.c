/* -----------------------------   Math utilities for nRF52832   ------------------------------- */
/*  File      -  Computational core source file                                                  */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/***************************************   INCLUDES   ********************************************/
#include "Maths.h"

/******************************************   DEFINES   ******************************************/
#define S32_UPPER_BOUND 0x7FFFFFFF
#define INVALID_RETURN  0xFFFFFFFF

/************************************   PUBLIC FUNCTIONS   ***************************************/
int32_t s32Power(uint8_t u8Base, uint8_t u8Exponent)
{
    int32_t s32RetVal = INVALID_RETURN;

    /* Zero to the power of zero is undefined */
    if(u8Base || u8Exponent)
    {
        s32RetVal = 1;

        while(u8Exponent)
        {
            s32RetVal *= u8Base;
            if(s32RetVal >= (S32_UPPER_BOUND / u8Base))
            {
                s32RetVal = INVALID_RETURN;
                break;
            }
            u8Exponent--;
        }
    }

    return s32RetVal;
}

uint8_t u8DigitCount(uint32_t u32Integer)
{
    uint8_t u8RetVal = 1;

    while(u32Integer >= 10)
    {
        u32Integer /= 10;
        u8RetVal++;
    }

    return u8RetVal;
}