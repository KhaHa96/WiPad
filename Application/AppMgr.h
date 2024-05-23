/* ---------------------------   Application Manager for nRF52832   ---------------------------- */
/*  File      -  Application Manager definition header file                                      */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_MGR_H_
#define _APP_MGR_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"
#include "Attribution.h"
#include "Registration.h"
#include "Display.h"

/**************************************   PRIVATE TYPES   ****************************************/
/**
 * AppMgr_tenuAppId Enumeration of the different application IDs.
*/
typedef enum
{
    App_AttributionId = 0, /* Key attribution application ID   */
    App_RegistrationId,    /* User registration application ID */
    App_DisplayId          /* Display application ID           */
}AppMgr_tenuAppId;

/**
 * AppMgr_tenuEvents Enumeration of the different dispatchable events handled by the Application
 *                   Manager.
*/
typedef enum
{
    AppMgr_LowerBoundEvt = 0,
    AppMgr_DisplayAdvertising,    /* Advertising started                                      */
    AppMgr_DisplayConnected,      /* Peer connection established                              */
    AppMgr_DisplayDisconnected,   /* Peer disconnection                                       */
    AppMgr_DisplayValidInput,     /* Valid user input                                         */
    AppMgr_DisplayInvalidInput,   /* Invalid user input                                       */
    AppMgr_DisplayAccessGranted,  /* Access granted                                           */
    AppMgr_DisplayAccessDenied,   /* Access denied                                            */
    AppMgr_DisplayAdminAdd,       /* Successful admin user add operation                      */
    AppMgr_DisplayAdminCheck,     /* Successful admin user info log operation                 */
    AppMgr_DisplayNotifsDisabled, /* User input detected with notifications disabled          */
    AppMgr_RegNotifEnabled,       /* Peer enabled notifications on User Registration service  */
    AppMgr_RegNotifDisabled,      /* Peer disabled notifications on User Registration service */
    AppMgr_RegUsrInputRx,         /* Received user input on User Id/Password characteristic   */
    AppMgr_AdmNotifEnabled,       /* Peer enabled notifications on Admin User service         */
    AppMgr_AdmNotifDisabled,      /* Peer disabled notifications on Admin User service        */
    AppMgr_AdmUsrInputRx,         /* Received user input on Command characteristic            */
    AppMgr_AdmUsrAddedToNvm,      /* Successfully added new user entry to NVM                 */
    AppMgr_UpperBoundEvt
}AppMgr_tenuEvents;

/**
 * AppMgrInitialize Application initialization function prototype.
 *
 * @note This function prototype is used to define an initialization function for each
 *       application which is then invoked by the Application Manager before starting FreeRTOS'
 *       schedule.
 *
 * @note Functions of this type take no parameters.
 *
*/
typedef App_tenuStatus (*AppMgrInitialize)(void);

/**
 * AppMgrGetNotified Application notification function prototype.
 *
 * @note This function prototype is used to define a notification function for each application
 *       responsible for posting external events dispatched from other applications and middleware
 *       tasks to the local event group.
 *
 * @note Functions of this type take two parameters:
 *  - uint32_t u32Event: Event to be posted and processed by local task.
 *  - void *pvData: Pointer to event-related data.
*/
typedef App_tenuStatus (*AppMgrGetNotified)(uint32_t u32Event, void *pvData);

/**
 * AppMgr_tstrInterface Application's public interface definition structure outlining the public
 *                      functions exposed to the Application Manager.
 *
*/
typedef struct
{
    AppMgrInitialize pfInit;   /* Application initialization function */
    AppMgrGetNotified pfNotif; /* Application notification function   */
}AppMgr_tstrInterface;

/**
 * AppMgr_tstrEventSub Application Manager's dispatchable events database structure.
 *
 * @note This structure type maps every application/middleware dispatchable event to a list
 *       of subscribed applications.
*/
typedef struct
{
    AppMgr_tenuEvents enuPublishedEvent;                   /* Dispatched event                  */
    AppMgr_tenuAppId enuSubscribedApps[APPLICATION_COUNT]; /* List of subscribed applications   */
    uint8_t u8SubCnt;                                      /* Number of subscribed applications */
}AppMgr_tstrEventSub;

/*************************************   PUBLIC FUNCTIONS   **************************************/
/**
 * @brief AppMgr_enuInit Initializes all applications.
 *
 * @note This function is invoked by the Main function.
 *
 * @pre This function requires no prerequisites.
 *
 * @return App_tenuStatus Application_Success if all applications were initialized successfully,
 *         Application_Failure otherwise.
 */
App_tenuStatus AppMgr_enuInit(void);

/**
 * @brief AppMgr_enuDispatchEvent Dispatches an event to all of its subscribed applications
 *        based on the event pub/sub scheme list.
 *
 * @note This function is invoked from within the context of application and middleware
 *       tasks that request notifying a thirdparty application of a new event.
 *
 * @param u32Event Event to be dispatched.
 * @param pvData Pointer to event-related data.
 *
 * @return App_tenuStatus Application_Success if event was dispatched successfully to its
 *         destination, Application_Failure otherwise.
 */
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

#endif /* _APP_MGR_H_ */