/* ---------------------------   Application Manager for nRF52832   ---------------------------- */
/*  File      -  Application Manager definition source file                                      */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "AppMgr.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_MANAGER_POWER_BASE 2U

/*************************************   PRIVATE MACROS   ****************************************/
#define APPMGR_ASSERT_EVENT(ARG)        \
(                                       \
    ( (ARG) > AppMgr_LowerBoundEvt ) && \
    ( (ARG) < AppMgr_UpperBoundEvt )    \
)

/************************************   PRIVATE VARIABLES   **************************************/
/* Applications' public interface list */
static const AppMgr_tstrInterface strApplicationList[] =
{
    {enuAttribution_Init , enuAttribution_GetNotified },
    {enuRegistration_Init, enuRegistration_GetNotified},
    {enuDisplay_Init     , enuDisplay_GetNotified     }
};

/* Dispatchable events' pub/sub scheme list
 *
 * Note: An application is said to be subscribed to an event if it requires being notified as soon
 *       as that event is dispatched to the Application Manager.
 */
static const AppMgr_tstrEventSub strEventSubscriptionList[] =
{
    {AppMgr_DisplayAdvertising   , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayConnected     , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayDisconnected  , {App_DisplayId, App_RegistrationId, App_AttributionId}, 3},
    {AppMgr_DisplayValidInput    , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayInvalidInput  , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayAccessGranted , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayAccessDenied  , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayAdminAdd      , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayAdminCheck    , {App_DisplayId                                       }, 1},
    {AppMgr_DisplayNotifsDisabled, {App_DisplayId                                       }, 1},
    {AppMgr_RegNotifEnabled      , {App_RegistrationId                                  }, 1},
    {AppMgr_RegNotifDisabled     , {App_RegistrationId                                  }, 1},
    {AppMgr_RegUsrInputRx        , {App_RegistrationId                                  }, 1},
    {AppMgr_AdmNotifEnabled      , {App_RegistrationId                                  }, 1},
    {AppMgr_AdmNotifDisabled     , {App_RegistrationId                                  }, 1},
    {AppMgr_AdmUsrInputRx        , {App_RegistrationId                                  }, 1},
    {AppMgr_AdmUsrAddedToNvm     , {App_RegistrationId                                  }, 1},
    {AppMgr_RegPasswordUpdated   , {App_RegistrationId                                  }, 1},
    {AppMgr_AttNotifEnabled      , {App_AttributionId                                   }, 1},
    {AppMgr_AttNotifDisabled     , {App_AttributionId                                   }, 1},
    {AppMgr_AttUserSignedIn      , {App_AttributionId                                   }, 1},
    {AppMgr_AttInputRx           , {App_AttributionId                                   }, 1}
};

/*************************************   PUBLIC FUNCTIONS   **************************************/
App_tenuStatus AppMgr_enuInit(void)
{
    App_tenuStatus enuRetVal = Application_Success;

    /* Initialize all applications */
    for(uint8_t u8Index = 0; u8Index < APPLICATION_COUNT; u8Index++)
    {
        if(Application_Failure == strApplicationList[u8Index].pfInit())
        {
            enuRetVal = Application_Failure;
            break;
        }
    }

    return enuRetVal;
}

extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData)
{
    App_tenuStatus enuRetVal = Application_Failure;

    /* Make sure argument is a valid dispatched event */
    if(APPMGR_ASSERT_EVENT(u32Event))
    {
        for(const AppMgr_tstrEventSub *pstrEvent = strEventSubscriptionList;
            pstrEvent < strEventSubscriptionList + (sizeof(strEventSubscriptionList) / sizeof(AppMgr_tstrEventSub));
            pstrEvent++)
        {
            if(u32Event == (uint32_t)pstrEvent->enuPublishedEvent)
            {
                /* Event located in event pub/sub scheme list */
                enuRetVal = Application_Success;

                /* Notify all subscribed applications */
                for(uint8_t u8Index = 0; u8Index < pstrEvent->u8SubCnt; u8Index++)
                {
                    if(strApplicationList[pstrEvent->enuSubscribedApps[u8Index]].pfNotif)
                    {
                        strApplicationList[pstrEvent->enuSubscribedApps[u8Index]].pfNotif((uint32_t)s32Power(APP_MANAGER_POWER_BASE,
                                                                                                             u32Event-1),
                                                                                                             pvData);
                    }
                }

                /* No need to keep going through event scheme list */
                break;
            }
        }
    }

    return enuRetVal;
}