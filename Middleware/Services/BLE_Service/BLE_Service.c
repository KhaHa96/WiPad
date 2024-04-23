/* -----------------------------   BLE Service for nRF51422   ---------------------------------- */
/*  File      -  BLE Service source file                                                         */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "BLE_Service.h"
#include "softdevice_handler.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_reg.h"
#include "ble_att.h"

/*****************************************   DEFINES   *******************************************/
#define BLE_CENTRAL_LINK_COUNT                 0
#define BLE_PERIPHERAL_LINK_COUNT              1
#define BLE_ADV_DEVICE_NAME                    "WiPad"
#define BLE_MIN_CONN_INTERVAL                  MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define BLE_MAX_CONN_INTERVAL                  MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define BLE_SLAVE_LATENCY                      0
#define BLE_CONN_SUP_TIMEOUT                   MSEC_TO_UNITS(4000, UNIT_10_MS)
#define BLE_FIRST_CONN_PARAM_UPDATE_DELAY      5000
#define BLE_REGULAR_CONN_PARAM_UPDATE_DELAY    30000
#define BLE_MAX_NBR_CONN_PARAM_UPDATE_ATTEMPTS 3
#define BLE_ADVERTISING_INTERVAL               64
#define BLE_ADVERTISING_DURATION_IN_SECONDS    180
#define BLE_EVENT_NO_WAIT                      (TickType_t)0UL
#define BLE_EVENT_MASK                         (BLE_START_ADVERTISING)

/************************************   PRIVATE MACROS   *****************************************/
#define BLE_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Ble_tstrState))

/*******************************   PRIVATE FUNCTION PROTOTYPES   *********************************/
static void vidBleStartAdvertising(void);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvBleTaskHandle;
static SemaphoreHandle_t pvBleSemaphoreHandle;
static EventGroupHandle_t pvBleEventGroupHandle;
static ble_use_reg_t strBleUseRegInstance;
static ble_key_att_t strBleKeyAttInstance;
static ble_uuid_t strAdvUuids[] =
{
    {BLE_USE_REG_UUID_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}
};
static const Ble_tstrState strBleStateMachine[] =
{
    {BLE_START_ADVERTISING, vidBleStartAdvertising}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidTaskNotify(void)
{
    /* Initialize task yield request to pdFALSE */
    BaseType_t yield_req = pdFALSE;

    /* Notify Softdevice RTOS task of an incoming event from BLE stack by giving the Semaphore */
    xSemaphoreGiveFromISR(pvBleSemaphoreHandle, &yield_req);

    /* If lYieldRequest was set to pdTRUE after the previous call, then a task of higher priority
       than the currently active task has been unblocked as a result of sending out an event
       notification. This requires an immediate context switch to the newly unblocked task. */
    portYIELD_FROM_ISR(yield_req);
}

static uint32_t SD_EVT_IRQHandler(void)
{
    /* Notify Ble task of incoming Softdevice event */
    vidTaskNotify();

    return NRF_SUCCESS;
}

static void vidBleEvent_Process(uint32_t u32Trigger)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = BLE_TRIGGER_COUNT(strBleStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Trigger == (strBleStateMachine + u8Index)->u32Trigger)
        {
            /* Invoke associated action and exit loop */
            (strBleStateMachine + u8Index)->pfAction();
            break;
        }
        u8Index++;
    }
}

static void vidBleStartAdvertising(void)
{
    /* Initiate advertising */
    (void)ble_advertising_start(BLE_ADV_MODE_FAST);
}

static void vidBleEvtDispatch(ble_evt_t *pstrEvent)
{

}

static void vidSystemEvtDispatch(uint32_t u32SystemEvent)
{

}

static void vidUseRegEventHandler(BleReg_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch(pstrEvent->enuEventType)
        {
        case BLE_REG_CONNECTED:
        {

        }
        break;

        case BLE_REG_DISCONNECTED:
        {

        }
        break;

        case BLE_REG_ID_PWD_RX:
        {

        }
        break;

        case BLE_REG_NOTIF_ENABLED:
        {

        }
        break;

        case BLE_REG_NOTIF_DISABLED:
        {

        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidKeyAttEventHandler(BleAtt_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch(pstrEvent->enuEventType)
        {
        case BLE_ATT_CONNECTED:
        {

        }
        break;

        case BLE_ATT_DISCONNECTED:
        {

        }
        break;

        case BLE_ATT_KEY_ACT_RX:
        {

        }
        break;

        case BLE_ATT_NOTIF_ENABLED:
        {

        }
        break;

        case BLE_ATT_NOTIF_DISABLED:
        {

        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidAdvEventHandler(ble_adv_evt_t enuEvent)
{

}

static void vidConnParamEventHandler(ble_conn_params_evt_t *pstrEvent)
{

}

static void vidConnParamErrorHandler(uint32_t u32Error)
{

}

static Mid_tenuStatus enuBleStackInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Set Softdevice's low frequency clock source */
    nrf_clock_lf_cfg_t strLfClkCfg = {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                      .rc_ctiv       = 0,                                \
                                      .rc_temp_ctiv  = 0,                                \
                                      .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};

    /* Initialize the SoftDevice handler module */
    SOFTDEVICE_HANDLER_INIT(&strLfClkCfg, SD_EVT_IRQHandler);

    /* Fetch Softdevice's default enable parameters */
    ble_enable_params_t strBleEnableParams;
    if(NRF_SUCCESS == softdevice_enable_get_default_config(BLE_CENTRAL_LINK_COUNT,
                                                           BLE_PERIPHERAL_LINK_COUNT,
                                                           &strBleEnableParams))
    {
        /* Enable BLE Softdevice */
        if(NRF_SUCCESS == softdevice_enable(&strBleEnableParams))
        {
            /* Register a BLE stack event dispatcher function to be invoked upon receiving
               a BLE stack event */
            if(NRF_SUCCESS == softdevice_ble_evt_handler_set(vidBleEvtDispatch))
            {
                /* Register a system event dispatcher to be invoked upon receiving a system event */
                enuRetVal = (NRF_SUCCESS == softdevice_sys_evt_handler_set(vidSystemEvtDispatch))
                                            ?Middleware_Success
                                            :Middleware_Failure;
            }
        }
    }

    return enuRetVal;
}

static Mid_tenuStatus enuBleGapInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;
    ble_gap_conn_sec_mode_t strGapSecurityMode;
    ble_gap_conn_params_t strGapConnParams;

    /* Set an open link, no protection required security mode */
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strGapSecurityMode);

    /* Set device's GAP advertised name */
    if(NRF_SUCCESS == sd_ble_gap_device_name_set(&strGapSecurityMode,
                                                 (const uint8_t *)BLE_ADV_DEVICE_NAME,
                                                 strlen(BLE_ADV_DEVICE_NAME)))
    {
        /* Apply GAP connection settings */
        memset(&strGapConnParams, 0, sizeof(strGapConnParams));
        strGapConnParams.min_conn_interval = BLE_MIN_CONN_INTERVAL;
        strGapConnParams.max_conn_interval = BLE_MAX_CONN_INTERVAL;
        strGapConnParams.slave_latency = BLE_SLAVE_LATENCY;
        strGapConnParams.conn_sup_timeout = BLE_CONN_SUP_TIMEOUT;

        /* Set GAP's connection parameters */
        enuRetVal = (NRF_SUCCESS == sd_ble_gap_ppcp_set(&strGapConnParams))
                                                        ?Middleware_Success
                                                        :Middleware_Failure;
    }

    return enuRetVal;
}

static Mid_tenuStatus enuBleServicesInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;
    BleReg_tstrInit strUseRegInit = {0};
    BleAtt_tstrInit strKetAttInit = {0};

    /* Initialize User Registration service */
    strUseRegInit.pfUseRegEvtHandler = vidUseRegEventHandler;
    if(Middleware_Success == enuBleUseRegInit(&strBleUseRegInstance, &strUseRegInit))
    {
        /* Initialize Key Attribution service */
        strKetAttInit.pfKeyAttEvtHandler = vidKeyAttEventHandler;
        if(Middleware_Success == enuBleKeyAttInit(&strBleKeyAttInstance, &strKetAttInit))
        {
            /* Initialize Admin User service */
            __NOP();
        }
    }

    //return enuRetVal;
    return Middleware_Success;
}

static Mid_tenuStatus enuAdvertisingInit(void)
{
    ble_advdata_t strAdvData;
    ble_adv_modes_config_t strAdvConfig;

    /* Apply advertising module's settings */
    memset(&strAdvData, 0, sizeof(strAdvData));
    strAdvData.name_type = BLE_ADVDATA_FULL_NAME;
    strAdvData.include_appearance = false;
    strAdvData.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    strAdvData.uuids_complete.uuid_cnt = sizeof(strAdvUuids) / sizeof(strAdvUuids[0]);
    strAdvData.uuids_complete.p_uuids  = strAdvUuids;

    /* Apply advertising module's configuration */
    memset(&strAdvConfig, 0, sizeof(strAdvConfig));
    strAdvConfig.ble_adv_fast_enabled = true;
    strAdvConfig.ble_adv_fast_interval = BLE_ADVERTISING_INTERVAL;
    strAdvConfig.ble_adv_fast_timeout = BLE_ADVERTISING_DURATION_IN_SECONDS;

    /* Initialize advertising module */
    return (NRF_SUCCESS == ble_advertising_init(&strAdvData,
                                                NULL,
                                                &strAdvConfig,
                                                vidAdvEventHandler,
                                                NULL))
                                                ?Middleware_Success
                                                :Middleware_Failure;
}

static Mid_tenuStatus enuBleConnParamsInit(void)
{
    ble_conn_params_init_t strConnParams;

    /* Apply Connection parameters negotiation module's settings */
    memset(&strConnParams, 0, sizeof(strConnParams));
    strConnParams.p_conn_params = NULL;
    strConnParams.first_conn_params_update_delay = BLE_FIRST_CONN_PARAM_UPDATE_DELAY;
    strConnParams.next_conn_params_update_delay = BLE_REGULAR_CONN_PARAM_UPDATE_DELAY;
    strConnParams.max_conn_params_update_count = BLE_MAX_NBR_CONN_PARAM_UPDATE_ATTEMPTS;
    strConnParams.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    strConnParams.disconnect_on_fail = false;
    strConnParams.evt_handler = vidConnParamEventHandler;
    strConnParams.error_handler = vidConnParamErrorHandler;

    /* Initialize Connection parameters negotiation module */
    return (NRF_SUCCESS == ble_conn_params_init(&strConnParams))
                                                ?Middleware_Success
                                                :Middleware_Failure;
}

static void vidBleTaskFunction(void *pvArg)
{
    uint32_t u32Event;
    BaseType_t lSemaphoreAvailable;

    /* TODO: Just for testing! Remove later */
    enuBle_GetNotified(BLE_START_ADVERTISING, NULL);

    /* Ble task's main loop */
    while(1)
    {
        /* Wait for event from SoftDevice */
        lSemaphoreAvailable = xSemaphoreTake(pvBleSemaphoreHandle, portMAX_DELAY);

        if(pdTRUE == lSemaphoreAvailable)
        {
            /* Process Ble stack incoming events by invoking the BLE stack event dispatcher
               registered by the enuBleStackInit function */
            intern_softdevice_events_execute();

            /* Retrieve event if any from event group */
            u32Event = xEventGroupWaitBits(pvBleEventGroupHandle,
                                           BLE_EVENT_MASK,
                                           pdTRUE,
                                           pdFALSE,
                                           BLE_EVENT_NO_WAIT);
            if(u32Event)
            {
                /* Process received event */
                vidBleEvent_Process(u32Event);
            }
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
Mid_tenuStatus enuBle_Init(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Create task for Ble stack */
    if(pdPASS == xTaskCreate(vidBleTaskFunction,
                             "Ble_Task",
                             MID_BLE_TASK_STACK_SIZE,
                             NULL,
                             MID_BLE_TASK_PRIORITY,
                             &pvBleTaskHandle))
    {
        /* Create synchronization semaphore for Ble task */
        pvBleSemaphoreHandle = xSemaphoreCreateBinary();

        if(pvBleSemaphoreHandle)
        {
            /* Create event group for Ble middleware service */
            pvBleEventGroupHandle = xEventGroupCreate();

            if(pvBleEventGroupHandle)
            {
                /* Initialize BLE stack */
                if(Middleware_Success == enuBleStackInit())
                {
                    /* Initialize GAP */
                    if(Middleware_Success == enuBleGapInit())
                    {
                        /* Initialize BLE services */
                        if(Middleware_Success == enuBleServicesInit())
                        {
                            /* Initialize advertising module */
                            if(Middleware_Success == enuAdvertisingInit())
                            {
                                /* Initialize Connection Parameters module */
                                enuRetVal = enuBleConnParamsInit();
                            }
                        }
                    }
                }
            }
        }
    }

    return enuRetVal;
}

Mid_tenuStatus enuBle_GetNotified(uint32_t u32Event, void *pvData)
{
    /* Unblock Ble task */
    vidTaskNotify();
    /* Set event in local event group */
    return (xEventGroupSetBits(pvBleEventGroupHandle, u32Event))
                               ?Middleware_Success
                               :Middleware_Failure;
}