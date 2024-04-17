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
#include "Attribution.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_KEY_ATT_PUSH_IMMEDIATELY 0
#define APP_KEY_ATT_POP_IMMEDIATELY  0
#define APP_KEY_ATT_EVENT_MASK       (APP_KEY_ATT_TEST_EVENT1 | APP_KEY_ATT_TEST_EVENT2)

/************************************   PRIVATE MACROS   *****************************************/
#define APP_KEY_ATT_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Attribution_tstrState))

/************************************   GLOBAL VARIABLES   ***************************************/
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvKeyAttTaskHandle;
static QueueHandle_t pvKeyAttQueueHandle;
static EventGroupHandle_t pvKeyAttEventGroupHandle;
static void vidKeyAttEvent1Process(void *pvArg);
static void vidKeyAttEvent2Process(void *pvArg);
static const Attribution_tstrState strKeyAttStateMachine[] =
{
    {APP_KEY_ATT_TEST_EVENT1, vidKeyAttEvent1Process},
    {APP_KEY_ATT_TEST_EVENT2, vidKeyAttEvent2Process}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidKeyAttEvent1Process(void *pvArg)
{
    __NOP();
}

static void vidKeyAttEvent2Process(void *pvArg)
{
    __NOP();
}

static void vidAttributionEvent_Process(uint32_t u32Trigger, void *pvData)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = APP_KEY_ATT_TRIGGER_COUNT(strKeyAttStateMachine);
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

    /* Key attribution task's main polling loop */
    while(1)
    {
        /* Task will remain blocked until an event is set in event group */
        u32Event = xEventGroupWaitBits(pvKeyAttEventGroupHandle,
                                       APP_KEY_ATT_EVENT_MASK,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
        if(u32Event)
        {
            /* Check whether queue holds any data */
            if(uxQueueMessagesWaiting(pvKeyAttQueueHandle))
            {
                xQueueReceive(pvKeyAttQueueHandle, pvData, APP_KEY_ATT_POP_IMMEDIATELY);
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

    /* Create task for Key attribution application */
    if(pdPASS == xTaskCreate(vidKeyAttTaskFunction,
                             "APP_KeyAtt_Task",
                             APP_KEYATT_TASK_STACK_SIZE,
                             NULL,
                             APP_KEYATT_TASK_PRIORITY,
                             &pvKeyAttTaskHandle))
    {
        /* Create message queue for Key attribution application */
        pvKeyAttQueueHandle = xQueueCreate(APP_KEYATT_QUEUE_LENGTH, sizeof(uint32_t));

        if(pvKeyAttQueueHandle)
        {
            /* Create event group for Key attribution application */
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
                                          pvData,
                                          APP_KEY_ATT_PUSH_IMMEDIATELY))
                                          ?Application_Success
                                          :Application_Failure;
    }

    if(Application_Success == enuRetVal)
    {
        /* Unblock Key attribution task by setting event in local event group */
        enuRetVal = xEventGroupSetBits(pvKeyAttEventGroupHandle, u32Event)
                                       ?Application_Success
                                       :Application_Failure;
    }
    return enuRetVal;
}