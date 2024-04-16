/* -----------------------------   BLE Service for nRF52832   ---------------------------------- */
/*  File      -  BLE Service header file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_BLE_H_
#define _MID_BLE_H_

/****************************************   INCLUDES   *******************************************/
#include "Middleware_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_START_ADVERTISING (1 << 0)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * BleAction State machine event-triggered action function prototype.
 *
 * @note This prototype is used to define state machine actions associated to different state
 *       triggers. A state action is invoked upon receiving its trigger.
 *
 * @note Functions of this type take no arguments.
 */
typedef void (*BleAction)(void);

/**
 * Ble_tstrState State-defining structure for the BLE middleware.
*/
typedef struct
{
    uint32_t u32Trigger;
    BleAction pfAction;
}Ble_tstrState;

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
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBle_Init(void);

/**
 * @brief enuBle_GetNotified Notifies Ble task of an incoming event by setting it in
 *        local event group.
 *
 * @pre This function can't be called unless Ble task is initialized and running.
 *
 * @param u32Event Event to be posted in local event group.
 * @param pvData Pointer to event-related data.
 *
 * @return Mid_tenuStatus Middleware_Success if notification was posted successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBle_GetNotified(uint32_t u32Event, void *pvData);

#endif /* _MID_BLE_H_ */