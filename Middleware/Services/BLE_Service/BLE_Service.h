/* -----------------------------   BLE Service for nRF51422   ---------------------------------- */
/*  File      -  BLE Service header file                                                         */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_BLE_H_
#define _MID_BLE_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "system_config.h"

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuBle_Init Creates Ble stack task, binary semaphore to signal receiving Ble events
 *        from Softdevice and registers callback to dispatch notifications to the application
 *        layer through the Application Manager.
 *
 * @note This function is invoked by the System Manager.
 *
 * @pre This function requires no prerequisites.
 *
 * @return Mid_tenuStatus Middleware_Success if initialization was performed successfully,
 *         Middleware_Failure otherwise
 */
Mid_tenuStatus enuBle_Init(void);

#endif /* _MID_BLE_H_ */