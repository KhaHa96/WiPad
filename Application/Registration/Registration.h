/* -------------------------   User Registration app for nRF51422   ---------------------------- */
/*  File      -  User registration application header file                                       */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_USEREG_H_
#define _APP_USEREG_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
/* Event bits */
#define APP_USEREG_PEER_DISCONNECTION (1 << 2) /* Disconnected from peer                    */
#define APP_USEREG_NOTIF_ENABLED (1 << 10)     /* Peer enabled notifications on ble_reg     */
#define APP_USEREG_NOTIF_DISABLED (1 << 11)    /* Peer disabled notifications on ble_reg    */
#define APP_USEREG_USR_INPUT_RX (1 << 12)      /* Received data from peer on Id/Pwd charac  */
#define APP_USEADM_NOTIF_ENABLED (1 << 13)     /* Peer enabled notifications on ble_adm     */
#define APP_USEADM_NOTIF_DISABLED (1 << 14)    /* Peer disabled notifications on ble_adm    */
#define APP_USEADM_USR_INPUT_RX (1 << 15)      /* Received data from peer on command charac */
#define APP_USEADM_USR_ADDED_TO_NVM (1 << 16)  /* New user added to NVM                     */
#define APP_USEADM_PASSWORD_UPDATED (1 << 17)  /* User password updated                     */

/* Dispatchable events */
#define BLE_USEREG_VALID_INPUT 4U     /* User entered a valid input display pattern         */
#define BLE_USEREG_INVALID_INPUT 5U   /* User entered an invalid input display pattern      */
#define BLE_USEREG_USER_ADDED 8U      /* User added successfully to NVM display pattern     */
#define BLE_USEREG_USER_DATA 9U       /* User data extracted successfully display pattern   */
#define BLE_USEREG_NOTIF_DISABLED 10U /* Received data with notifs disabled display pattern */
#define BLE_USEREG_USER_SIGNED_IN 21U /* Notify attribution of user signing in successfully */

/**************************************   PUBLIC TYPES   *****************************************/

/**
 * RegistrationAction State machine event-triggered action function pointer.
 *
 * @note This prototype is used to define state machine actions associated to different state
 *       triggers. A state action is invoked upon receiving its trigger.
 *
 * @note Functions of this type take one argument:
 *         - void *pvArg: Pointer to event-related data passed to state machine entry's action.
 */
typedef void (*RegistrationAction)(void *pvArg);

/**
 * Registration_tenuAdmCmdType Enumeration of the different possible Admin command types
 */
typedef enum
{
    Adm_AddUser = 0, /* Admin add user command */
    Adm_UserData,    /* Admin user data command */
    Adm_InvalidCmd   /* Admin invalid command */
} Registration_tenuAdmCmdType;

/**
 * Registration_tstrState State-defining structure for the Registration application
 */
typedef struct
{
    uint32_t u32Trigger;
    RegistrationAction pfAction;
} Registration_tstrState;

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuRegistration_Init Creates User registration task, event group to receive
 *        notifications from other tasks or events dispatched from BLE stack and registers
 *        callback to report back to Application Manager.
 *
 * @note This function is invoked by the Application Manager.
 *
 * @pre This function requires no prerequisites.
 *
 * @return App_tenuStatus Application_Success if initialization was performed successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuRegistration_Init(void);

/**
 * @brief enuRegistration_GetNotified Notifies User registration task of an incoming event by
 *        setting it in local event group and setting its accompanying data in local message
 *        queue.
 *
 * @note This function is invoked by the Application Manager signaling that another task
 *       wants to communicate with the User registration task.
 *
 * @pre This function can't be called unless User registration task is initialized and running.
 *
 * @param u32Event Event to be posted in local event group
 * @param pvData Pointer to event-related data
 *
 * @return App_tenuStatus Application_Success if notification was posted successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuRegistration_GetNotified(uint32_t u32Event, void *pvData);

#endif /* _APP_USEREG_H_ */