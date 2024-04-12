/* ------------------------------   Display app for nRF52832   --------------------------------- */
/*  File      -  Display application source file                                                 */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Display.h"
#include "nrf_gpio.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_DISPLAY_LED_COUNT           4
#define APP_DISPLAY_DEFAULT_CYCLE_COUNT 4
#define APP_DISPLAY_LED_SWITCH_ON       0
#define APP_DISPLAY_LED_SWITCH_OFF      1
#define APP_DISPLAY_TIMER_NO_WAIT       0
#define APP_DISPLAY_EVENT_MASK          (APP_DISPLAY_ID_VERIF_SUCCESS | \
                                         APP_DISPLAY_ID_VERIF_FAILURE )

/************************************   PRIVATE MACROS   *****************************************/
#define APP_DISPLAY_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Display_tstrState))
#define APP_DISPLAY_ALIGN_EVENT(arg) (arg - 1)
#define APP_DISPLAY_PENULTIMATE_LED(arg) ((arg-(arg/LED_2)+3*(LED_1/arg)))

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvDisplayTaskHandle;
static QueueHandle_t pvDisplayQueueHandle;
static EventGroupHandle_t pvDisplayEventGroupHandle;
static TimerHandle_t pvDisplayTimerHandle;
static volatile uint8_t u8LedCounter;
static volatile uint8_t u8CycleCounter;
static uint32_t u32CurrentEvent;
static void vidDisplayIdVerifSuccess(void);
static void vidDisplayIdVerifFailure(void);
static const Display_tstrState strDisplayStateMachine[] =
{
    {APP_DISPLAY_ID_VERIF_SUCCESS, vidDisplayIdVerifSuccess},
    {APP_DISPLAY_ID_VERIF_FAILURE, vidDisplayIdVerifFailure}
};
static const Display_tstrLedPattern strDisplayPatterns[] =
{
    {APP_DISPLAY_ID_VERIF_SUCCESS, u32LedPattern1243},
    {APP_DISPLAY_ID_VERIF_FAILURE, u32LedPattern1423}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidDisplayIdVerifSuccess(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ID_VERIF_SUCCESS;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayIdVerifFailure(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ID_VERIF_FAILURE;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayTimerCallback(TimerHandle_t pvTimerHandle)
{
    if(pvTimerHandle != NULL)
    {
        if((LED_1 == u8LedCounter) && (!(--u8CycleCounter)))
        {
            /* Switch off last Led in the pattern */
            nrf_gpio_pin_write(strDisplayPatterns[APP_DISPLAY_ALIGN_EVENT(u32CurrentEvent)].pfArrangement(LED_4),
                               APP_DISPLAY_LED_SWITCH_OFF);
            /* Stop timer */
            xTimerStop(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
        }
        else
        {
            /* Toggle LEDs sequentially */
            nrf_gpio_pin_write(strDisplayPatterns[APP_DISPLAY_ALIGN_EVENT(u32CurrentEvent)].pfArrangement(APP_DISPLAY_PENULTIMATE_LED(u8LedCounter)),
                               APP_DISPLAY_LED_SWITCH_OFF);
            nrf_gpio_pin_write(strDisplayPatterns[APP_DISPLAY_ALIGN_EVENT(u32CurrentEvent)].pfArrangement(u8LedCounter),
                               APP_DISPLAY_LED_SWITCH_ON);
            /* Increment Led counter in cycles of 4 */
            u8LedCounter = LED_1 + (((u8LedCounter+1)%LED_1)%APP_DISPLAY_LED_COUNT);
        }
    }
}

static void vidDisplayEvent_Process(uint32_t u32Trigger)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = APP_DISPLAY_TRIGGER_COUNT(strDisplayStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Trigger == (strDisplayStateMachine + u8Index)->u32Trigger)
        {
            /* Invoke associated action */
            (strDisplayStateMachine + u8Index)->pfAction();
        }
        u8Index++;
    }
}

static void vidDisplayTaskFunction(void *pvArg)
{
    uint32_t u32Event;

    /* Display task's main polling loop */
    while(1)
    {
        /* Task will remain blocked until an event is set in event group */
        u32Event = xEventGroupWaitBits(pvDisplayEventGroupHandle,
                                       APP_DISPLAY_EVENT_MASK,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
        if(u32Event)
        {
            /* Reset LED counter */
            u8LedCounter = LED_1;
            /* Reset cycle counter */
            u8CycleCounter = APP_DISPLAY_DEFAULT_CYCLE_COUNT;
            /* Process received event */
            vidDisplayEvent_Process(u32Event);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
App_tenuStatus enuDisplay_Init(void)
{
    App_tenuStatus enuRetVal = Application_Failure;

    /* Configure LED pins as output */
    nrf_gpio_range_cfg_output(LED_1, LED_4);

    /* Turn all LEDs off */
    for(uint8_t u8Index = LED_1; u8Index <= LED_4; u8Index++)
    {
        nrf_gpio_pin_write((uint32_t)u8Index, APP_DISPLAY_LED_SWITCH_OFF);
    }

    /* Create task for LED application */
    if(pdTRUE == xTaskCreate(vidDisplayTaskFunction,
                             "APP_Display_Task",
                             APP_DISPLAY_TASK_STACK_SIZE,
                             NULL,
                             APP_DISPLAY_TASK_PRIORITY,
                             &pvDisplayTaskHandle))
    {
        /* Create message queue for Display application */
        pvDisplayQueueHandle = xQueueCreate(APP_DISPLAY_QUEUE_LENGTH, sizeof(uint32_t));

        if(pvDisplayQueueHandle)
        {
            /* Create event group for Display application */
            pvDisplayEventGroupHandle = xEventGroupCreate();

            if(pvDisplayEventGroupHandle)
            {
                pvDisplayTimerHandle = xTimerCreate("APP_Display_Timer",
                                                    pdMS_TO_TICKS(400),
                                                    pdTRUE,
                                                    NULL,
                                                    vidDisplayTimerCallback);

                enuRetVal = (pvDisplayTimerHandle)?Application_Success:Application_Failure;
            }
        }
    }

    return enuRetVal;
}

App_tenuStatus enuDisplay_GetNotified(uint32_t u32Event, void *pvData)
{
    /* Unblock Display task by setting event in local event group */
    return (xEventGroupSetBits(pvDisplayEventGroupHandle, u32Event))
                               ?Application_Success
                               :Application_Failure;
}