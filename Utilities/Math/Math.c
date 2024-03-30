/* -----------------------------   Math utilities for nRF52832   ------------------------------- */
/*  File      -  Computational core source file                                                  */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/***************************************   INCLUDES   ********************************************/
#include "Math.h"

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