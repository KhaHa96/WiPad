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
#include "Maths.h"

/******************************************   TYPES   ********************************************/
/**
 * App_tenuStatus Enumeration of the different possible application operation outcomes.
*/
typedef enum
{
    Application_Success = 0, /* Operation performed successfully */
    Application_Failure      /* Operation failed                 */
}App_tenuStatus;

/**
 * App_tenuKeyTypes Enumeration of the different available key types.
*/
typedef enum
{
    App_KeyLowerBound = 0,
    App_OneTimeKey,         /* One-time key, valid for a single use                     */
    App_CountRestrictedKey, /* Count-restricted key, valid for a finite number of times */
    App_UnlimitedKey,       /* Unlimited key, infinitely reusable                       */
    App_TimeRestrictedKey,  /* Time-restricted key, valid for a limited time span       */
    App_AdminKey,           /* Admin key, granted to authenticated Admin users only     */
    App_KeyUpperBound
}App_tenuKeyTypes;

#endif /* _APP_TYPES_H_ */