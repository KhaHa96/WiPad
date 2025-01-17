/* -------------------------   Middleware utilities for nRF52832   ----------------------------- */
/*  File      -  Utility header file for middleware services                                     */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_TYPES_H_
#define _MID_TYPES_H_

/****************************************   INCLUDES   *******************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/******************************************   TYPES   ********************************************/
/**
 * Mid_tenuStatus Enumeration of the different possible middleware operation outcomes.
*/
typedef enum
{
    Middleware_Success = 0, /* Operation performed successfully */
    Middleware_Failure      /* Operation failed                 */
}Mid_tenuStatus;

#endif /* _MID_TYPES_H_ */