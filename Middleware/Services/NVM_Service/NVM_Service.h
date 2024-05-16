/* -----------------------------   NVM Service for nRF52832   ---------------------------------- */
/*  File      -  NVM Service header file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_NVM_H_
#define _MID_NVM_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "system_config.h"

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief vidNvm_Init
 *
 * @note This function is invoked by the Main function.
 *
 * @pre This function requires no prerequisites.
 *
 * @return Nothing.
 */
Mid_tenuStatus enuNvm_Init(void);

#endif /* _MID_NVM_H_ */