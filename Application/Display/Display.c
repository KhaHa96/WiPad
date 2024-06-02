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
#define APP_DISPLAY_LED_COUNT           4U
#define APP_DISPLAY_DEFAULT_CYCLE_COUNT 4U
#define APP_DISPLAY_LED_SWITCH_ON       0U
#define APP_DISPLAY_LED_SWITCH_OFF      1U
#define APP_DISPLAY_TIMER_NO_WAIT       0U
#define APP_DISPLAY_EVENT_MASK          (APP_DISPLAY_ADVERTISING_START         | \
                                         APP_DISPLAY_PEER_CONNECTION           | \
                                         APP_DISPLAY_PEER_DISCONNECTION        | \
                                         APP_DISPLAY_VALID_USER_INPUT          | \
                                         APP_DISPLAY_INVALID_USER_INPUT        | \
                                         APP_DISPLAY_ACCESS_GRANTED            | \
                                         APP_DISPLAY_ACCESS_DENIED             | \
                                         APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP   | \
                                         APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP | \
                                         APP_DISPLAY_DISABLED_NOTIFICATIONS      )

/************************************   PRIVATE MACROS   *****************************************/
/* Compute state machine trigger count */
#define APP_DISPLAY_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Display_tstrState))

/* Event and associated display pattern aligning macro */
#define APP_DISPLAY_ALIGN_EVENT(arg) (arg - 1)

/* Macro to get previoud LED pin index in pattern */
#define APP_DISPLAY_PENULTIMATE_LED(arg) ((arg-(arg/LED_2)+3*(LED_1/arg)))

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvDisplayTaskHandle;             /* Display task handle                      */
static QueueHandle_t pvDisplayQueueHandle;           /* Display queue handle                     */
static EventGroupHandle_t pvDisplayEventGroupHandle; /* Display event group handle               */
static TimerHandle_t pvDisplayTimerHandle;           /* Display timer handle                     */
static volatile uint8_t u8LedCounter;                /* Current LED's rank in pattern            */
static volatile uint8_t u8CycleCounter;              /* Current cycle in display pattern         */
static uint32_t u32CurrentEvent;                     /* Current event whose pattern is displayed */
static uint8_t u8LedsState = 1;                      /* LED state in current half cycle          */
static void vidDisplayAdvertisingStart(void);        /* Advertising started func prototype       */
static void vidDisplayPeerConnected(void);           /* Connection established func prototype    */
static void vidDisplayPeerDisonnected(void);         /* Disconnected from peer func prototype    */
static void vidDisplayInputVerifSuccess(void);       /* Valid input received func prototype      */
static void vidDisplayInputVerifFailure(void);       /* Invalid input received func prototype    */
static void vidDisplayAccessGranted(void);           /* Access granted func prototype            */
static void vidDisplayAccessDenied(void);            /* Access denied func prototype             */
static void vidDisplaySuccessfulAddOp(void);         /* New user added func prototype            */
static void vidDisplaySuccessfulCheckOp(void);       /* User data requested func prototype       */
static void vidDisplayDisabledNotifs(void);          /* Notifications disabled func prototype    */

/* Display state machine's entry list */
static const Display_tstrState strDisplayStateMachine[] =
{
    {APP_DISPLAY_ADVERTISING_START        , vidDisplayAdvertisingStart }, /* Advertising started */
    {APP_DISPLAY_PEER_CONNECTION          , vidDisplayPeerConnected    }, /* Connected to peer   */
    {APP_DISPLAY_PEER_DISCONNECTION       , vidDisplayPeerDisonnected  }, /* Peer disconnection  */
    {APP_DISPLAY_VALID_USER_INPUT         , vidDisplayInputVerifSuccess}, /* Valid input         */
    {APP_DISPLAY_INVALID_USER_INPUT       , vidDisplayInputVerifFailure}, /* Invalid input       */
    {APP_DISPLAY_ACCESS_GRANTED           , vidDisplayAccessGranted    }, /* Access granted      */
    {APP_DISPLAY_ACCESS_DENIED            , vidDisplayAccessDenied     }, /* Access denied       */
    {APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP  , vidDisplaySuccessfulAddOp  }, /* New user added      */
    {APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP, vidDisplaySuccessfulCheckOp}, /* User data requested */
    {APP_DISPLAY_DISABLED_NOTIFICATIONS   , vidDisplayDisabledNotifs   }  /* Notifs disabled     */
};

/* Display patterns function list */
static const Display_tstrLedPattern strDisplayPatterns[] =
{
    {APP_DISPLAY_ADVERTISING_START        , u32LedPattern1342},       /* 1,3,4,2 display pattern */
    {APP_DISPLAY_PEER_CONNECTION          , u32LedPattern1324},       /* 1,3,2,4 display pattern */
    {APP_DISPLAY_PEER_DISCONNECTION       , u32LedPattern4231},       /* 4,2,3,1 display pattern */
    {APP_DISPLAY_VALID_USER_INPUT         , u32LedPattern2341},       /* 2,3,4,1 display pattern */
    {APP_DISPLAY_INVALID_USER_INPUT       , u32LedPattern1432},       /* 1,4,3,2 display pattern */
    {APP_DISPLAY_ACCESS_GRANTED           , u32LedPattern1243},       /* 1,2,4,3 display pattern */
    {APP_DISPLAY_ACCESS_DENIED            , u32LedPattern1423},       /* 1,4,2,3 display pattern */
    {APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP  , u32LedPattern1234},       /* 1,2,3,4 display pattern */
    {APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP, u32LedPattern4321}        /* 4,3,2,1 display pattern */
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidDisplayAdvertisingStart(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ADVERTISING_START_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayPeerConnected(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_PEER_CONNECTION_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayPeerDisonnected(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_PEER_DISCONNECTION_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayInputVerifSuccess(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_VALID_USER_INPUT_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayInputVerifFailure(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_INVALID_USER_INPUT_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayAccessGranted(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ACCESS_GRANTED_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayAccessDenied(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ACCESS_DENIED_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplaySuccessfulAddOp(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplaySuccessfulCheckOp(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayDisabledNotifs(void)
{
    /* Set current event */
    u32CurrentEvent = APP_DISPLAY_DISABLED_NOTIFICATIONS_RANK;
    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
}

static void vidDisplayTimerCallback(TimerHandle_t pvTimerHandle)
{
    if(pvTimerHandle)
    {
        if(APP_DISPLAY_DISABLED_NOTIFICATIONS_RANK == u32CurrentEvent)
        {
            if((u8LedsState) && (!(--u8CycleCounter)))
            {
                /* Stop timer */
                xTimerStop(pvDisplayTimerHandle, APP_DISPLAY_TIMER_NO_WAIT);
            }
            else
            {
                /* Toggle all LEDs */
                u8LedsState = !u8LedsState;
                for(uint8_t u8Index = LED_1; u8Index <= LED_4; u8Index++)
                {
                    nrf_gpio_pin_write((uint32_t)u8Index, u8LedsState);
                }
            }
        }
        else
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
            /* Invoke associated action and exit loop */
            (strDisplayStateMachine + u8Index)->pfAction();
            break;
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
                /* Create software timer for Display application */
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