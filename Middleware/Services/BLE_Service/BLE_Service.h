/* -----------------------------   BLE Service for nRF52832   ---------------------------------- */
/*  File      -  BLE Service header file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_BLE_H_
#define _MID_BLE_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
/* External notifications */
#define BLE_START_ADVERTISING   (1 << 0)
#define BLE_GET_CURRENT_TIME    (1 << 1)
#define BLE_HANDLE_TIME_READING (1 << 2)

/* Dispatchable events */
#define BLE_CONNECTION_EVENT    3U
#define BLE_DISCONNECTION_EVENT 4U
#define BLE_ADVERTISING_STARTED 5U

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * BleAction State machine event-triggered action function prototype.
 *
 * @note This prototype is used to define state machine actions associated to different state
 *       triggers. A state action is invoked upon receiving its trigger.
 *
 * @note Functions of this type take one argument:
 *         - void *pvArg: Pointer to event-related data passed to state machine entry's action.
 */
typedef void (*BleAction)(void *pvArg);

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
 * @brief enuBle_Init Creates Ble stack task, event group to signal receiving events from the
 *        application layer through the Application Manager, message queue to hold data potentially
 *        accompanying incoming events and initializes the Ble stack.
 *
 * @note This function is invoked by the Main function.
 *
 * @pre This function requires no prerequisites.
 *
 * @return Mid_tenuStatus Middleware_Success if initialization was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBle_Init(void);

/**
 * @brief enuBle_GetNotified Notifies Ble task of an incoming event by setting it in
 *        local event group and setting its accompanying data in local message queue.
 *
 * @note Posting the received event in the local event group is not directly responsible
 *       for unblocking the Ble task. Rather this function unblocks the Ble task upon
 *       receiving an event through giving a task notification (equivalent to giving
 *       counting semaphore) and posting received event to local event group for the task
 *       function to retrieve and process.
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