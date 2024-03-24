/* --------------------------   Key attribution app for nRF51422   ----------------------------- */
/*  File      -  Key attribution application header file                                         */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_KEYATT_H_
#define _APP_KEYATT_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define APP_KEY_ATT_TEST_EVENT1 (1 << 0)
#define APP_KEY_ATT_TEST_EVENT2 (1 << 1)
#define APP_KEY_ATT_EVENT_MASK  (APP_KEY_ATT_TEST_EVENT1 | APP_KEY_ATT_TEST_EVENT2)

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuAttribution_Init Creates Key attribution task, event group to receive notifications
 *        from other tasks or events dispatched from BLE stack and registers callback to
 *        report back to Application Manager.
 *
 * @note This function is invoked by the Application Manager.
 *
 * @pre This function requires no prerequisites.
 *
 * @return App_tenuStatus Application_Success if initialization was performed successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuAttribution_Init(void);

/**
 * @brief enuAttribution_GetNotified Notifies Key Attribution task of an incoming event by
 *        setting it in local event group.
 *
 * @note This function is invoked by the Application Manager signaling that another task
 *       wants to communicate with the Key Attribution task.
 *
 * @pre This function can't be called unless Key Attribution task is initialized and running.
 *
 * @param u32Event Event to be posted in local event group
 *
 * @return App_tenuStatus Application_Success if notification was posted successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuAttribution_GetNotified(uint32_t u32Event);

#endif /* _APP_KEYATT_H_ */