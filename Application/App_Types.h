/* ----------------------------   Application types for nRF52832   ----------------------------- */
/*  File      -  Application type definition header file                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

/****************************************   INCLUDES   *******************************************/
#include <stdint.h>

/******************************************   TYPES   ********************************************/
/**
 * App_tenuStatus Enumeration of the different possible application operation outcomes
*/
typedef enum
{
    Application_Success = 0, /* Operation performed successfully */
    Application_Failure      /* Operation failed                 */
}App_tenuStatus;

#endif /* _APP_TYPES_H_ */