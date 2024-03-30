/* -------------------------   User Registration app for nRF52832   ---------------------------- */
/*  File      -  User registration application header file                                       */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_USEREG_H_
#define _APP_USEREG_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define APP_USE_REG_TEST_EVENT1 (1 << 0)
#define APP_USE_REG_TEST_EVENT2 (1 << 1)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * State machine event-triggered action function pointer
 *
 * Functions of this type take one argument:
 *  - void *pvArg: Pointer to event-related data passed to state machine entry's action
*/
typedef void (*RegistrationAction)(void *pvArg);

/**
 * Registration_tstrState State-defining structure for the Registration application
*/
typedef struct
{
    uint32_t u32Trigger;
    RegistrationAction pfAction;
}Registration_tstrState;

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