/* --------------------------   Key attribution app for nRF52832   ----------------------------- */
/*  File      -  Key attribution application source file                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Time.h"
#include "Attribution.h"
#include "BLE_Service.h"
#include "NVM_Service.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_KEYATT_PUSH_IMMEDIATELY 0U
#define APP_KEYATT_POP_IMMEDIATELY  0U
#define APP_KEYATT_ACTIVATION_TOKEN 1U
#define APP_KEYATT_EVENT_MASK       (APP_KEYATT_DISCONNECTION  | \
                                     APP_KEYATT_NOTIF_ENABLED  | \
                                     APP_KEYATT_NOTIF_DISABLED | \
                                     APP_KEYATT_USER_SIGNED_IN | \
                                     APP_KEYATT_USR_INPUT_RX     )

/************************************   PRIVATE MACROS   *****************************************/
/* Compute state machine trigger count */
#define APP_KEYATT_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Attribution_tstrState))

/* Converts time in minutes to time in seconds */
#define APP_KEYATT_MINS_TO_SECS(MIN) (MIN*60)

/************************************   GLOBAL VARIABLES   ***************************************/
/* Global function used to propagate dispatchable events to other tasks */
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvKeyAttTaskHandle;             /* Attribution task handle                   */
static QueueHandle_t pvKeyAttQueueHandle;           /* Attribution queue handle                  */
static EventGroupHandle_t pvKeyAttEventGroupHandle; /* Attribution event group handle            */
static volatile bool bAttNotifEnabled = false;      /* Notifications enabled/disabled on ble_att */
static volatile bool bAttUserSignedIn = false;      /* Active user has/hasn't already signed in  */
static void vidKeyAttDisconnected(void *pvArg);     /* Disconnection function prototype          */
static void vidKeyAttNotifEnabled(void *pvArg);     /* Notifs enabled on ble_att func prototype  */
static void vidKeyAttNotifDisabled(void *pvArg);    /* Notifs disabled on ble_att func prototype */
static void vidKeyAttUserSignedIn(void *pvArg);     /* User signed in function prototype         */
static void vidKeyAttInputReceived(void *pvArg);    /* Input received on ble_att func prototype  */
static fds_record_desc_t strActiveRecordDesc = {0}; /* Active NVM record's descriptor            */
static Nvm_tstrRecord strActiveRecord = {0};        /* Active NVM record data content            */
static const Attribution_tstrState strKeyAttStateMachine[] =
{
    {APP_KEYATT_DISCONNECTION , vidKeyAttDisconnected },
    {APP_KEYATT_NOTIF_ENABLED , vidKeyAttNotifEnabled },
    {APP_KEYATT_NOTIF_DISABLED, vidKeyAttNotifDisabled},
    {APP_KEYATT_USER_SIGNED_IN, vidKeyAttUserSignedIn },
    {APP_KEYATT_USR_INPUT_RX  , vidKeyAttInputReceived},
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidUserKeyNotify(Nvm_tstrRecord *pstrRecord)
{
    /* Make sure valid arguments are passed */
    if(pstrRecord)
    {
        switch(pstrRecord->enuKeyType)
        {
        case App_OneTimeKey:
        {
            /* One-time expirable key */
            uint8_t u8NotificationBuffer[] = "One-time expirable";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Attribution, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_CountRestrictedKey:
        {
            /* Count-restricted expirable key */
            uint8_t u8NotificationBuffer[] = "Count-limited: ";
            /* Copy count-limit value into buffer in a human-readable format */
            uint8_t u8LimitDigitCnt = u8DigitCount(pstrRecord->uKeyQuantifier.strCountRes.u16CountLimit);
            char chTimeoutStr[5];
            snprintf(chTimeoutStr, sizeof(chTimeoutStr), "%d",
                     pstrRecord->uKeyQuantifier.strCountRes.u16CountLimit -
                     pstrRecord->uKeyQuantifier.strCountRes.u16UsedCount);

            strncpy((char *)&u8NotificationBuffer[15], chTimeoutStr, u8LimitDigitCnt+1);
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)+u8LimitDigitCnt;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Attribution, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_UnlimitedKey:
        {
            /* Unlimited persistent key */
            uint8_t u8NotificationBuffer[] = "Unlimited persistent";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Attribution, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_TimeRestrictedKey:
        {
            /* Time-restricted expirable key */
            uint8_t u8NotificationBuffer[] = "Time-limited";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer);

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Attribution, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_AdminKey:
        {
            /* Admin persistent key */
            uint8_t u8NotificationBuffer[] = "Admin persistent";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Attribution, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidKeyAttDisconnected(void *pvArg)
{
    /* Reset all global variables */
    bAttNotifEnabled = false;
    bAttUserSignedIn = false;
    memset(&strActiveRecordDesc, 0, sizeof(strActiveRecordDesc));
    memset(&strActiveRecord, 0, sizeof(strActiveRecord));
}

static void vidKeyAttNotifEnabled(void *pvArg)
{
    /* Key Attribution service's notifications enabled. Toggle its notifications enabled flag */
    bAttNotifEnabled = true;

    if(bAttUserSignedIn)
    {
        /* Notify user of their key type */
        vidUserKeyNotify(&strActiveRecord);
    }
}

static void vidKeyAttNotifDisabled(void *pvArg)
{
    /* Key Attribution service's notifications disabled. Toggle its notifications enabled flag */
    bAttNotifEnabled = false;
}

static void vidKeyAttUserSignedIn(void *pvArg)
{
    /* Make sure valid parameters are passed */
    if(pvArg)
    {
        /* Extract received data */
        Nvm_tstrRecordDispatch *pstrActiveRecord = (Nvm_tstrRecordDispatch *)pvArg;
        /* Store received data record and record descriptor */
        memcpy(&strActiveRecordDesc, pstrActiveRecord->pstrRecordDesc, sizeof(fds_record_desc_t));
        memcpy(&strActiveRecord, pstrActiveRecord->pstrRecord, sizeof(Nvm_tstrRecord));

        /* Toggle user signed in flag */
        bAttUserSignedIn = true;

        if(bAttNotifEnabled)
        {
            /* Notify user of their key type */
            vidUserKeyNotify(&strActiveRecord);
        }
        else
        {
            /* Signed in with Key Attribution service's notifications disabled. Give user a visual
               heads-up */
            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_NOTIF_DISABLED, NULL);
        }

        /* Free allocated memory */
        free((void *)((Nvm_tstrRecordDispatch *)pvArg)->pstrRecordDesc);
        free((void *)((Nvm_tstrRecordDispatch *)pvArg)->pstrRecord);
        free((void *)(Nvm_tstrRecordDispatch *)pvArg);
    }
}

static void vidKeyAttInputReceived(void *pvArg)
{
    /* If notifications on Key Attribution's Status characteristic are disabled, we don't even
       process user's input */
    if(bAttNotifEnabled)
    {
        /* Make sure valid arguments are passed */
        if(pvArg)
        {
            /* Verify user has signed in */
            if(bAttUserSignedIn)
            {
                /* Extract received data */
                Ble_tstrRxData *pstrInput = (Ble_tstrRxData *)pvArg;

                /* Check for valid input */
                if(APP_KEYATT_ACTIVATION_TOKEN == pstrInput->pu8Data[0])
                {
                    switch(strActiveRecord.enuKeyType)
                    {
                    case App_OneTimeKey:
                    {
                        if(!strActiveRecord.uKeyQuantifier.bOneTimeExpired)
                        {
                            /* One-time key activated. Send notification to peer */
                            uint8_t u8NotificationBuffer[] = "Welcome!";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Attribution,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Grant access */
                            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_GRANT_ACCESS, NULL);

                            /* Invalidate one-time key */
                            strActiveRecord.uKeyQuantifier.bOneTimeExpired = true;

                            /* Update NVM record */
                            (void)enuNVM_UpdateRecord(&strActiveRecordDesc,
                                                      &strActiveRecord,
                                                      Nvm_ExpirableKeys,
                                                      false);
                        }
                        else
                        {
                            /* Key Expired. Send notification to peer */
                            uint8_t u8NotificationBuffer[] = "Key expired";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Attribution,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Display rejection pattern */
                            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_ACCESS_DENIED, NULL);

                            /* Delete user entry from NVM */
                            (void)enuNVM_DeleteRecord(&strActiveRecordDesc);
                        }
                    }
                    break;

                    case App_CountRestrictedKey:
                    {
                        if(strActiveRecord.uKeyQuantifier.strCountRes.u16UsedCount <
                           strActiveRecord.uKeyQuantifier.strCountRes.u16CountLimit)
                        {
                            /* Count-restricted key activated. Send notification to peer */
                            uint8_t u8NotificationBuffer[] = "Welcome!";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Attribution,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Grant access */
                            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_GRANT_ACCESS, NULL);

                            /* Increment use count */
                            strActiveRecord.uKeyQuantifier.strCountRes.u16UsedCount++;

                            /* Update NVM record */
                            (void)enuNVM_UpdateRecord(&strActiveRecordDesc,
                                                      &strActiveRecord,
                                                      Nvm_ExpirableKeys,
                                                      false);
                        }
                        else
                        {
                            /* Key Expired. Send notification to peer */
                            uint8_t u8NotificationBuffer[] = "Key expired";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Attribution,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Display rejection pattern */
                            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_ACCESS_DENIED, NULL);

                            /* Delete user entry from NVM */
                            (void)enuNVM_DeleteRecord(&strActiveRecordDesc);
                        }
                    }
                    break;

                    case App_UnlimitedKey:
                    {
                        /* Unlimited key activated. Send notification to peer */
                        uint8_t u8NotificationBuffer[] = "Welcome!";
                        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                        (void)enuTransferNotification(Ble_Attribution,
                                                      u8NotificationBuffer,
                                                      &u16NotificationSize);
                        /* Grant access */
                        (void)AppMgr_enuDispatchEvent(BLE_KEYATT_GRANT_ACCESS, NULL);
                    }
                    break;

                    case App_TimeRestrictedKey:
                    {
                        /* Request current time */
                        vidBleGetCurrentTime();
                    }
                    break;

                    case App_AdminKey:
                    {
                        /* Admin key activated. Send notification to peer */
                        uint8_t u8NotificationBuffer[] = "Welcome!";
                        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                        (void)enuTransferNotification(Ble_Attribution,
                                                      u8NotificationBuffer,
                                                      &u16NotificationSize);
                        /* Grant access */
                        (void)AppMgr_enuDispatchEvent(BLE_KEYATT_GRANT_ACCESS, NULL);
                    }
                    break;

                    default:
                        /* Nothing to do */
                        break;
                    }
                }
                else
                {
                    /* Notify user of invalid request format */
                    uint8_t u8NotificationBuffer[] = "Invalid! Try again";
                    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                    (void)enuTransferNotification(Ble_Attribution,
                                                  u8NotificationBuffer,
                                                  &u16NotificationSize);
                    /* Display visual cue */
                    (void)AppMgr_enuDispatchEvent(BLE_KEYATT_INVALID_INPUT, NULL);
                }
            }
            else
            {
                /* User hasn't signed in yet. Prompt them to do so */
                uint8_t u8NotificationBuffer[] = "Please sign in first";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                (void)enuTransferNotification(Ble_Attribution,
                                              u8NotificationBuffer,
                                              &u16NotificationSize);
            }

            /* Free allocated memory */
            free((void *)((Ble_tstrRxData *)pvArg)->pu8Data);
            free((void *)(Ble_tstrRxData *)pvArg);
        }
    }
    else
    {
        /* Received input with Key Attribution service's notifications disabled. Give user a visual
           heads-up */
        (void)AppMgr_enuDispatchEvent(BLE_KEYATT_NOTIF_DISABLED, NULL);
    }
}

static void vidCurrentTimeCallback(exact_time_256_t *pstrCurrentTime)
{
    /* Check whether time-restricted key has been activated already */
    if(strActiveRecord.uKeyQuantifier.strTimeRes.bIsKeyActive)
    {
        if((u32TimeToEpoch(pstrCurrentTime) -
           strActiveRecord.uKeyQuantifier.strTimeRes.u32ActivationTime) >=
           APP_KEYATT_MINS_TO_SECS(strActiveRecord.uKeyQuantifier.strTimeRes.u16Timeout))
        {
            /* Key Expired. Send notification to peer */
            uint8_t u8NotificationBuffer[] = "Key expired";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
            (void)enuTransferNotification(Ble_Attribution,
                                          u8NotificationBuffer,
                                          &u16NotificationSize);
            /* Display rejection pattern */
            (void)AppMgr_enuDispatchEvent(BLE_KEYATT_ACCESS_DENIED, NULL);

            /* Delete user entry from NVM */
            (void)enuNVM_DeleteRecord(&strActiveRecordDesc);
        }
    }
    else
    {
        /* Time-restricted key activated. Send notification to peer */
        uint8_t u8NotificationBuffer[] = "Welcome!";
        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
        (void)enuTransferNotification(Ble_Attribution,
                                      u8NotificationBuffer,
                                      &u16NotificationSize);
        /* Grant access */
        (void)AppMgr_enuDispatchEvent(BLE_KEYATT_GRANT_ACCESS, NULL);

        /* Toggle key activation state */
        strActiveRecord.uKeyQuantifier.strTimeRes.bIsKeyActive = true;
        /* Store key activation time */
        strActiveRecord.uKeyQuantifier.strTimeRes.u32ActivationTime = u32TimeToEpoch(pstrCurrentTime);
        /* Update user entry record in NVM */
        (void)enuNVM_UpdateRecord(&strActiveRecordDesc,
                                  &strActiveRecord,
                                  Nvm_ExpirableKeys,
                                  false);
    }
}

static void vidAttributionEvent_Process(uint32_t u32Trigger, void *pvData)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = APP_KEYATT_TRIGGER_COUNT(strKeyAttStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Trigger == (strKeyAttStateMachine + u8Index)->u32Trigger)
        {
            /* Invoke associated action and exit loop */
            (strKeyAttStateMachine + u8Index)->pfAction(pvData);
            break;
        }
        u8Index++;
    }
}

static void vidKeyAttTaskFunction(void *pvArg)
{
    uint32_t u32Event;
    void *pvData;

    /* Register current time data callback */
    vidRegisterCtsCallback(vidCurrentTimeCallback);

    /* Key Attribution task's main polling loop */
    while(1)
    {
        /* Task will remain blocked until an event is set in event group */
        u32Event = xEventGroupWaitBits(pvKeyAttEventGroupHandle,
                                       APP_KEYATT_EVENT_MASK,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
        if(u32Event)
        {
            /* Check whether queue holds any data */
            if(uxQueueMessagesWaiting(pvKeyAttQueueHandle))
            {
                /* We have no tasks of higher priority so we're guaranteed that no other
                   message will be received in the queue until this message is processed */
                xQueueReceive(pvKeyAttQueueHandle, &pvData, APP_KEYATT_POP_IMMEDIATELY);
            }

            /* Process received event */
            vidAttributionEvent_Process(u32Event, pvData);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
App_tenuStatus enuAttribution_Init(void)
{
    App_tenuStatus enuRetVal = Application_Failure;

    /* Create task for Key Attribution application */
    if(pdPASS == xTaskCreate(vidKeyAttTaskFunction,
                             "APP_KeyAtt_Task",
                             APP_KEYATT_TASK_STACK_SIZE,
                             NULL,
                             APP_KEYATT_TASK_PRIORITY,
                             &pvKeyAttTaskHandle))
    {
        /* Create message queue for Key Attribution application */
        pvKeyAttQueueHandle = xQueueCreate(APP_KEYATT_QUEUE_LENGTH, sizeof(void *));

        if(pvKeyAttQueueHandle)
        {
            /* Create event group for Key Attribution application */
            pvKeyAttEventGroupHandle = xEventGroupCreate();
            enuRetVal = (NULL != pvKeyAttEventGroupHandle)?Application_Success:Application_Failure;
        }
    }

    return enuRetVal;
}

App_tenuStatus enuAttribution_GetNotified(uint32_t u32Event, void *pvData)
{
    App_tenuStatus enuRetVal = Application_Success;

    if(pvData)
    {
        /* Push event-related data to local message queue */
        enuRetVal = (pdTRUE == xQueueSend(pvKeyAttQueueHandle,
                                          &pvData,
                                          APP_KEYATT_PUSH_IMMEDIATELY))
                                          ?Application_Success
                                          :Application_Failure;
        if(Application_Failure == enuRetVal)
        {
            if(BLE_ATT_USER_INPUT_RECEIVED == u32Event)
            {
                /* Free allocated memory */
                free((void *)((Ble_tstrRxData *)pvData)->pu8Data);
                free((void *)(Ble_tstrRxData *)pvData);
            }
            else
            {
                /* Free allocated memory */
                free((void *)((Nvm_tstrRecordDispatch *)pvData)->pstrRecordDesc);
                free((void *)((Nvm_tstrRecordDispatch *)pvData)->pstrRecord);
                free((void *)(Nvm_tstrRecordDispatch *)pvData);
            }
        }
    }

    if(Application_Success == enuRetVal)
    {
        /* Unblock Key Attribution task by setting event in local event group */
        enuRetVal = xEventGroupSetBits(pvKeyAttEventGroupHandle, u32Event)
                                       ?Application_Success
                                       :Application_Failure;
    }

    return enuRetVal;
}