/* -------------------------   User registration app for nRF52832   ---------------------------- */
/*  File      -  User registration application source file                                       */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Registration.h"
#include "BLE_Service.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_USEREG_POWER_BASE           2U
#define APP_USEREG_PUSH_IMMEDIATELY     0U
#define APP_USEREG_POP_IMMEDIATELY      0U
#define APP_ADMUSR_ID_LENGTH            8U
#define AP_MAX_COMMAND_LENGTH           20U
#define APP_USEREG_EVENT_MASK           (APP_USEREG_NOTIF_ENABLED  | \
                                         APP_USEREG_NOTIF_DISABLED | \
                                         APP_USEREG_USR_INPUT_RX   | \
                                         APP_USEADM_NOTIF_ENABLED  | \
                                         APP_USEADM_NOTIF_DISABLED | \
                                         APP_USEADM_USR_INPUT_RX)

/************************************   PRIVATE MACROS   *****************************************/
#define APP_USEREG_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Registration_tstrState))

/************************************   GLOBAL VARIABLES   ***************************************/
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvUseRegTaskHandle;
static QueueHandle_t pvUseRegQueueHandle;
static EventGroupHandle_t pvUseRegEventGroupHandle;
static volatile bool bRegNotifEnabled = false;
static volatile bool bAdmNotifEnabled = false;
static volatile bool bExpectingPwd = false;
static volatile bool bAdmSignedIn = false;
static void vidUseRegNotifEnabled(void *pvArg);
static void vidUseRegNotifDisabled(void *pvArg);
static void vidUseRegInputReceived(void *pvArg);
static void vidUseAdmNotifEnabled(void *pvArg);
static void vidUseAdmNotifDisabled(void *pvArg);
static void vidUseAdmInputReceived(void *pvArg);
static uint8_t u8AddUsrCmd[] = "mkusi";
static uint8_t u8UsrDataCmd[] = "mkud -i ";
static const Registration_tstrState strUseRegStateMachine[] =
{
    {APP_USEREG_NOTIF_ENABLED , vidUseRegNotifEnabled },
    {APP_USEREG_NOTIF_DISABLED, vidUseRegNotifDisabled},
    {APP_USEREG_USR_INPUT_RX  , vidUseRegInputReceived},
    {APP_USEADM_NOTIF_ENABLED , vidUseAdmNotifEnabled },
    {APP_USEADM_NOTIF_DISABLED, vidUseAdmNotifDisabled},
    {APP_USEADM_USR_INPUT_RX  , vidUseAdmInputReceived}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidUseRegNotifEnabled(void *pvArg)
{
    /* User Registration service's notifications enabled. Toggle its notifications enabled flag */
    bRegNotifEnabled = true;
    /* Prompt user to input their Id */
    uint8_t u8NotificationBuffer[] = "Please input your Id";
    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
    (void)enuTransferNotification(Ble_Registration, u8NotificationBuffer, &u16NotificationSize);
}

static void vidUseRegNotifDisabled(void *pvArg)
{
    /* User Registration service's notifications disabled. Toggle its notifications enabled flag */
    bRegNotifEnabled = false;
}

static void vidUseRegInputReceived(void *pvArg)
{
    /* If notifications on User Registration's Status characteristic are disabled, we don't even
       process user's input */
    if(bRegNotifEnabled)
    {
        /* Make sure valid parameters are passed */
        if(pvArg)
        {
            /* Check whether we're expecting a user Id or a user password */
            if(bExpectingPwd)
            {
                
            }
            else
            {

            }
        }
    }
    else
    {
        /* Received user input while notifications are disabled. Give user a visual heads-up */
        (void)AppMgr_enuDispatchEvent(BLE_USEREG_NOTIF_DISABLED, NULL);
    }
}

static void vidUseAdmNotifEnabled(void *pvArg)
{
    /* Admin User service's notifications enabled. Toggle its notifications enabled flag */
    bAdmNotifEnabled = true;

    /* Prompt Admin user to sign in if they haven't already, otherwise display a simple greeting */
    const char *pchNotification = bAdmSignedIn?"Hi there Admin":"Please input your Id";
    uint16_t u16NotificationSize = strlen(pchNotification);
    (void)enuTransferNotification(Ble_Admin, (uint8_t *)pchNotification, &u16NotificationSize);
}

static void vidUseAdmNotifDisabled(void *pvArg)
{
    /* Admin User service's notifications disabled. Toggle its notifications enabled flag */
    bAdmNotifEnabled = false;
}

static Registration_tenuAdmCmdType enuExtractCommandType(const uint8_t *pu8Data, uint8_t u8Length)
{
    Registration_tenuAdmCmdType enuRetVal = Adm_InvalidCmd;

    /* Make sure valid parameters are passed */
    if(pu8Data && (u8Length > 0) && (u8Length <= AP_MAX_COMMAND_LENGTH))
    {
        /* Compare received input with Add user command base */
        if(0 == s8StringCompare(pu8Data, u8AddUsrCmd, 5))
        {
            if((15 == u8Length) || (20 == u8Length))
            {
                /* Add user command received. Check whether input contains a valid user Id */
                if(bIsAllNumerals(&pu8Data[5], APP_ADMUSR_ID_LENGTH))
                {
                    if(0 == s8StringCompare(&pu8Data[13], "k1", 2))
                    {
                        enuRetVal = Adm_AddUser;
                    }
                    else if(0 == s8StringCompare(&pu8Data[13], "k2", 2))
                    {
                        if('c' == pu8Data[15])
                        {
                            if(bIsAllNumerals(&pu8Data[16], 4))
                            {
                                enuRetVal = Adm_AddUser;
                            }
                        }
                        else
                        {
                            enuRetVal = Adm_AddUser;
                        }                        
                    }
                    else if(0 == s8StringCompare(&pu8Data[13], "k3", 2))
                    {
                        if('\0' == pu8Data[15])
                        {
                            enuRetVal = Adm_AddUser;
                        }
                    }
                    else if(0 == s8StringCompare(&pu8Data[13], "k4", 2))
                    {
                        if('t' == pu8Data[15])
                        {
                            if(bIsAllNumerals(&pu8Data[16], 4))
                            {
                                enuRetVal = Adm_AddUser;
                            }
                        }
                    }
                }
            }
        }
        else if(s8StringCompare(pu8Data, u8UsrDataCmd, 8))
        {
            if(16 == u8Length)
            {
                /* User data command received. Check whether input contains a valid user Id */
                if(bIsAllNumerals(&pu8Data[8], APP_ADMUSR_ID_LENGTH))
                {
                    if('\0' == pu8Data[9])
                    {
                        enuRetVal = Adm_UserData;
                    }
                }
            }
        }
    }

    return enuRetVal;
}

static App_tenuKeyTypes enuDecodeAddCommand(const uint8_t *pu8Data, uint16_t *pu16Arg)
{
    App_tenuKeyTypes enuRetVal = App_KeyLowerBound;

    /* Make sure valid parameters are passed */
    if(pu8Data && pu16Arg)
    {
        switch(pu8Data[15])
        {
        case '\0':
        {
            enuRetVal = (App_tenuKeyTypes)atoi((char *)(&pu8Data[16]));
            pu16Arg = NULL;
        }
        break;

        case 'c':
        {
            /* Count-restricted key */
            enuRetVal = App_CountRestrictedKey;
            /* Extract key count */
            *pu16Arg = (uint16_t)atoi((char *)(&pu8Data[16]));
        }
        break;

        case 't':
        {
            /* Time-restricted key */
            enuRetVal = App_TimeRestrictedKey;
            /* Extract timeout */
            *pu16Arg = (uint16_t)atoi((char *)(&pu8Data[16]));
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }

    return enuRetVal;
}

static void vidUseAdmInputReceived(void *pvArg)
{
    /* If notifications on Admin User's Status characteristic are disabled, we don't even
       process user's input */
    if(bAdmNotifEnabled)
    {
        /* Make sure valid parameters are passed */
        if(pvArg)
        {
            /* Check whether user has been through the Admin authentication process */
            if(bAdmSignedIn)
            {
                /* Extract command from received data */
                Ble_tstrRxData *pstrCommand = (Ble_tstrRxData *)pvArg;
                /* Determine command type */
                Registration_tenuAdmCmdType enuCmdType = enuExtractCommandType(pstrCommand->pu8Data,
                                                                               pstrCommand->u16Length);

                switch(enuCmdType)
                {
                case Adm_AddUser:
                {
                    /* Extract user Id */

                    /* Decode Add user command to extract key type */
                    uint16_t *pu16Argument;
                    App_tenuKeyTypes enuKeyType = enuDecodeAddCommand(pstrCommand->pu8Data, pu16Argument);

                    /* Add new NVM entry */
                }
                break;

                case Adm_UserData:
                {
                    /* Extract user Id */

                    /* Add new NVM entry */
                }
                break;

                case Adm_InvalidCmd:
                {
                    /* Notify user of invalid input */
                    uint8_t u8NotificationBuffer[] = "Invalid! Try again";
                    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                    (void)enuTransferNotification(Ble_Admin,
                                                  u8NotificationBuffer,
                                                  &u16NotificationSize);
                }
                break;

                default:
                    /* Nothing to do */
                    break;
                }
            }
            else
            {
                /* User hasn't signed in as Admin. Prompt them to do so */
                uint8_t u8NotificationBuffer[] = "Please sign in first";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
            }
        }
    }
    else
    {
        /* Received user input while notifications are disabled. Give user a visual heads-up */
        (void)AppMgr_enuDispatchEvent(BLE_USEREG_NOTIF_DISABLED, NULL);
    }
}

static void vidRegistrationEvent_Process(uint32_t u32Trigger, void *pvData)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = APP_USEREG_TRIGGER_COUNT(strUseRegStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Trigger == (strUseRegStateMachine + u8Index)->u32Trigger)
        {
            /* Invoke associated action and exit loop */
            (strUseRegStateMachine + u8Index)->pfAction(pvData);
            break;
        }
        u8Index++;
    }
}

static void vidUseRegTaskFunction(void *pvArg)
{
    uint32_t u32Event;
    void *pvData;

    /* User registration task's main polling loop */
    while(1)
    {
        /* Task will remain blocked until an event is set in event group */
        u32Event = xEventGroupWaitBits(pvUseRegEventGroupHandle,
                                       APP_USEREG_EVENT_MASK,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
        if(u32Event)
        {
            /* Check whether queue holds any data */
            if(uxQueueMessagesWaiting(pvUseRegQueueHandle))
            {
                /* We have no tasks of higher priority so we're guaranteed that no other
                   message will be received in the queue until this message is processed */
                xQueueReceive(pvUseRegQueueHandle, &pvData, APP_USEREG_POP_IMMEDIATELY);
            }

            /* Process received event */
            vidRegistrationEvent_Process(u32Event, pvData);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
App_tenuStatus enuRegistration_Init(void)
{
    App_tenuStatus enuRetVal = Application_Failure;
    bAdmSignedIn=true; /* TODO: remove!!!*/

    /* Create task for User registration application */
    if(pdPASS == xTaskCreate(vidUseRegTaskFunction,
                             "APP_UseReg_Task",
                             APP_USEREG_TASK_STACK_SIZE,
                             NULL,
                             APP_USEREG_TASK_PRIORITY,
                             &pvUseRegTaskHandle))
    {
        /* Create message queue for User registration application */
        pvUseRegQueueHandle = xQueueCreate(APP_USEREG_QUEUE_LENGTH, sizeof(void *));

        if(pvUseRegQueueHandle)
        {
            /* Create event group for User registration application */
            pvUseRegEventGroupHandle = xEventGroupCreate();
            enuRetVal = (NULL != pvUseRegEventGroupHandle)?Application_Success:Application_Failure;
        }
    }

    return enuRetVal;
}

App_tenuStatus enuRegistration_GetNotified(uint32_t u32Event, void *pvData)
{
    App_tenuStatus enuRetVal = Application_Success;

    if(pvData)
    {
        /* Push event-related data to local message queue */
        enuRetVal = (pdTRUE == xQueueSend(pvUseRegQueueHandle,
                                          &pvData,
                                          APP_USEREG_PUSH_IMMEDIATELY))
                                          ?Application_Success
                                          :Application_Failure;
        if(Application_Failure == enuRetVal)
        {
            /* Free allocated memory */
            free((void *)((Ble_tstrRxData *)pvData)->pu8Data);
        }
    }

    if(Application_Success == enuRetVal)
    {
        /* Unblock Registration task by setting event in local event group */
        enuRetVal = xEventGroupSetBits(pvUseRegEventGroupHandle, u32Event)
                                       ?Application_Success
                                       :Application_Failure;
    }

    return enuRetVal;
}