/* --------------------------   Key Attribution app for nRF52832   ----------------------------- */
/*  File      -  Key Attribution application header file                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_KEYATT_H_
#define _APP_KEYATT_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
/* Event bits */
#define APP_KEYATT_DISCONNECTION  (1 << 2 )  /* Disconnected from peer                           */
#define APP_KEYATT_NOTIF_ENABLED  (1 << 18)  /* Peer enabled notifications on ble_att            */
#define APP_KEYATT_NOTIF_DISABLED (1 << 19)  /* Peer disabled notifications on ble_att           */
#define APP_KEYATT_USER_SIGNED_IN (1 << 20)  /* Active user successfully signed in               */
#define APP_KEYATT_USR_INPUT_RX   (1 << 21)  /* Received data from peer on Key Activation charac */

/* Dispatchable events */
#define BLE_KEYATT_INVALID_INPUT  5U  /* User entered an invalid input display pattern      */
#define BLE_KEYATT_GRANT_ACCESS   6U  /* Key activated legitimately and access is granted   */
#define BLE_KEYATT_ACCESS_DENIED  7U  /* Key activation request rejected                    */
#define BLE_KEYATT_NOTIF_DISABLED 10U /* Received data with notifs disabled display pattern */

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * AttributionAction State machine event-triggered action function pointer.
 *
 * @note This prototype is used to define state machine actions associated to different state
 *       triggers. A state action is invoked upon receiving its trigger.
 *
 * @note Functions of this type take one argument:
 *         - void *pvArg: Pointer to event-related data passed to state machine entry's action.
*/
typedef void (*AttributionAction)(void *pvArg);

/**
 * Attribution_tstrState State-defining structure for the Attribution application.
*/
typedef struct
{
    uint32_t u32Trigger;
    AttributionAction pfAction;
}Attribution_tstrState;

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuAttribution_Init Creates Key Attribution task, event group to receive notifications
 *        from other tasks and message queue to hold data potentially accompanying incoming events.
 *
 * @note This function is invoked by the Application Manager.
 *
 * @pre This function requires no prerequisites.
 *
 * @return App_tenuStatus Application_Success if initialization was performed successfully,
 *         Application_Failure otherwise.
 */
App_tenuStatus enuAttribution_Init(void);

/**
 * @brief enuAttribution_GetNotified Notifies Key Attribution task of an incoming event by
 *        setting it in local event group and setting its accompanying data in local
 *        message queue.
 *
 * @note This function is invoked by the Application Manager signaling that another task
 *       wants to communicate with the Key Attribution task.
 *
 * @pre This function can't be called unless the Key Attribution task is initialized and running.
 *
 * @param u32Event Event to be posted in local event group.
 * @param pvData Pointer to event-related data.
 *
 * @return App_tenuStatus Application_Success if notification was posted successfully,
 *         Application_Failure otherwise.
 */
App_tenuStatus enuAttribution_GetNotified(uint32_t u32Event, void *pvData);

#endif /* _APP_KEYATT_H_ */