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
#include "ble_cts_c.h"

/*************************************   PUBLIC DEFINES   ****************************************/
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
 * vidCtsCallback Current time data callback function prototype.
 *
 * @note This prototype is used to define a placeholder for a callback function used by Ble_Service
 *       to return current time readings to the Attribution application after acquiring them from
 *       peer's GATT server.
 *
 * @note Functions of this type take one argument:
 *         - exact_time_256_t *pstrCurrentTime: Pointer to acquired current time reading
 */
typedef void (*vidCtsCallback)(exact_time_256_t *pstrCurrentTime);

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
 * @brief vidBleGetCurrentTime Solicits peer's GATT server to obtain a current time reading.
 *
 * @note Acquiring a current time reading from peer is an asynchronous process, the outcome of
 *       which is obtained through a callback that should already have been registered by the
 *       calling module.
 *
 * @pre This function requires a connection and bond be established and a current time data
 *      callback be registered.
 *
 * @return Nothing.
 */
void vidBleGetCurrentTime(void);

/**
 * @brief vidRegisterCtsCallback Registers a callback to be invoked upon obtaining a current time
 *        reading.
 *
 * @note This function must be called before initiating any attempt to get a current time reading.
 *
 * @param pfCallback Pointer to current time data callback.
 *
 * @return Nothing.
 */
void vidRegisterCtsCallback(vidCtsCallback pfCallback);

#endif /* _MID_BLE_H_ */