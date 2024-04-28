/* -------------------------   User registration app for nRF52832   ---------------------------- */
/*  File      -  User registration application source file                                       */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Registration.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_USEREG_POWER_BASE       2U
#define APP_USEREG_PUSH_IMMEDIATELY 0U
#define APP_USEREG_POP_IMMEDIATELY  0U
#define APP_USEREG_EVENT_MASK       (APP_USEREG_TEST_EVENT1 | APP_USEREG_TEST_EVENT2)

/************************************   PRIVATE MACROS   *****************************************/
#define APP_USEREG_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Registration_tstrState))

/************************************   GLOBAL VARIABLES   ***************************************/
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvUseRegTaskHandle;
static QueueHandle_t pvUseRegQueueHandle;
static EventGroupHandle_t pvUseRegEventGroupHandle;
static void vidUseRegEvent1Process(void *pvArg);
static void vidUseRegEvent2Process(void *pvArg);
static const Registration_tstrState strUseRegStateMachine[] =
{
    {APP_USEREG_TEST_EVENT1, vidUseRegEvent1Process},
    {APP_USEREG_TEST_EVENT2, vidUseRegEvent2Process}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidUseRegEvent1Process(void *pvArg)
{
    __NOP();
}

static void vidUseRegEvent2Process(void *pvArg)
{
    __NOP();
}

static void vidRegistrationEvent_Process(uint32_t u32Trigger, void *pvData)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint32_t u32Event = (uint32_t)s32Power(APP_USEREG_POWER_BASE, u32Trigger - 1);
    uint8_t u8TriggerCount = APP_USEREG_TRIGGER_COUNT(strUseRegStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Event == (strUseRegStateMachine + u8Index)->u32Trigger)
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
                xQueueReceive(pvUseRegQueueHandle, pvData, APP_USEREG_POP_IMMEDIATELY);
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

    /* Create task for User registration application */
    if(pdPASS == xTaskCreate(vidUseRegTaskFunction,
                             "APP_UseReg_Task",
                             APP_USEREG_TASK_STACK_SIZE,
                             NULL,
                             APP_USEREG_TASK_PRIORITY,
                             &pvUseRegTaskHandle))
    {
        /* Create message queue for User registration application */
        pvUseRegQueueHandle = xQueueCreate(APP_USEREG_QUEUE_LENGTH, sizeof(uint32_t));

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
                                          pvData,
                                          APP_USEREG_PUSH_IMMEDIATELY))
                                          ?Application_Success
                                          :Application_Failure;
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