/* ----------------------------- Driver utilities for nRF52832 --------------------------------- */
/*  File      -  Utility header file for sensor drivers and low level abstraction layers         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/******************************************   INCLUDES   *****************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Maths.h"

/******************************************   TYPES   ********************************************/
typedef enum
{
    Driver_Success = 0, /* Operation performed successfully */
    Driver_Failure      /* Operation failed                 */
}Drivers_tenuStatus;