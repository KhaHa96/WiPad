/* ------------------------------   Driver types for nRF51422   -------------------------------- */
/*  File      -  Driver type definition header file                                              */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef DRV_TYPES_H_
#define DRV_TYPES_H_

/******************************************   INCLUDES   *****************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Maths.h"

/******************************************   TYPES   ********************************************/
/**
 * Drivers_tenuStatus Enumeration of the different possible driver operation outcomes
*/
typedef enum
{
    Driver_Success = 0, /* Operation performed successfully */
    Driver_Failure      /* Operation failed                 */
}Drivers_tenuStatus;

#endif /* DRV_TYPES_H_ */