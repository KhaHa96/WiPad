/* -----------------------------   BLE Service for nRF52832   ---------------------------------- */
/*  File      -  BLE Service source file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "BLE_Service.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_att.h"
#include "ble_adm.h"
#include "ble_reg.h"
#include "App_Types.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define BLE_CONN_CFG_TAG                       1U
#define BLE_ADV_DEVICE_NAME                    "WiPad"
#define BLE_MIN_CONN_INTERVAL                  MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define BLE_MAX_CONN_INTERVAL                  MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define BLE_SLAVE_LATENCY                      0U
#define BLE_CONN_SUP_TIMEOUT                   MSEC_TO_UNITS(4000, UNIT_10_MS)
#define BLE_RAM_START_ADDRESS                  0U
#define BLE_FIRST_CONN_PARAM_UPDATE_DELAY      5000U
#define BLE_REGULAR_CONN_PARAM_UPDATE_DELAY    30000U
#define BLE_MAX_NBR_CONN_PARAM_UPDATE_ATTEMPTS 3U
#define BLE_ADVERTISING_INTERVAL               64U
#define BLE_ADVERTISING_DURATION               18000U
#define BLE_EVENT_NO_WAIT                      (TickType_t)0UL
#define BLE_EVENT_MASK                         (BLE_START_ADVERTISING)

/************************************   PRIVATE MACROS   *****************************************/
#define BLE_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Ble_tstrState))

/************************************   GLOBAL VARIABLES   ***************************************/
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/*******************************   PRIVATE FUNCTION PROTOTYPES   *********************************/
static void vidBleStartAdvertising(void);

/************************************   PRIVATE VARIABLES   **************************************/
NRF_BLE_GATT_DEF(BleGattInstance);
NRF_BLE_QWR_DEF(BleQwrInstance);
BLE_USEREG_DEF(BleUseRegInstance, NRF_SDH_BLE_TOTAL_LINK_COUNT);
BLE_KEYATT_DEF(BleKeyAttInstance, NRF_SDH_BLE_TOTAL_LINK_COUNT);
BLE_ADM_DEF(BleAdminInstance, NRF_SDH_BLE_TOTAL_LINK_COUNT);
BLE_ADVERTISING_DEF(BleAdvInstance);
static TaskHandle_t pvBLETaskHandle;
static EventGroupHandle_t pvBleEventGroupHandle;
static uint16_t u16ConnHandle = BLE_CONN_HANDLE_INVALID;
static ble_uuid_t strAdvUuids[] =
{
    {BLE_KEYATT_UUID_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}
};
static const Ble_tstrState strBleStateMachine[] =
{
    {BLE_START_ADVERTISING, vidBleStartAdvertising}
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidTaskNotify(void)
{
    /* Initialize task yield request to pdFALSE */
    BaseType_t lYieldRequest = pdFALSE;

    /* Notify Softdevice RTOS task of an incoming event from BLE stack */
    vTaskNotifyGiveFromISR(pvBLETaskHandle, &lYieldRequest);

    /* If lYieldRequest was set to pdTRUE after the previous call, then a task of higher priority
       than the currently active task has been unblocked as a result of sending out an event
       notification. This requires an immediate context switch to the newly unblocked task. */
    portYIELD_FROM_ISR(lYieldRequest);
}

void SD_EVT_IRQHandler(void)
{
    /* Notify Ble task of incoming Softdevice event */
    vidTaskNotify();
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
    (void)ble_advertising_start(&BleAdvInstance, BLE_ADV_MODE_FAST);
}

static void vidConnParamEventHandler(ble_conn_params_evt_t *pstrEvent)
{

}

static void vidConnParamErrorHandler(uint32_t u32Error)
{

}

static void vidBleEventHandler(ble_evt_t const *pstrEvent, void *pvData)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            /* Preserve current connection handle */
            u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
            /* Assign current connection handle to Queued Writes module's instance */
            (void)nrf_ble_qwr_conn_handle_assign(&BleQwrInstance, u16ConnHandle);
            /* Trigger connection LED pattern */
            (void)AppMgr_enuDispatchEvent(BLE_CONNECTION_EVENT, NULL);
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            /* Clear connection handle placeholder */
            u16ConnHandle = BLE_CONN_HANDLE_INVALID;
            /* Trigger disconnection LED pattern */
            (void)AppMgr_enuDispatchEvent(BLE_DISCONNECTION_EVENT, NULL);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidUseRegEventHandler(BleReg_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch(pstrEvent->enuEventType)
        {
        case BLE_REG_NOTIF_ENABLED:
        {

        }
        break;

        case BLE_REG_NOTIF_DISABLED:
        {

        }
        break;

        case BLE_REG_STATUS_TX:
        {

        }
        break;

        case BLE_REG_ID_PWD_RX:
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
        case BLE_ATT_NOTIF_ENABLED:
        {

        }
        break;

        case BLE_ATT_NOTIF_DISABLED:
        {

        }
        break;

        case BLE_ATT_STATUS_TX:
        {

        }
        break;

        case BLE_ATT_KEY_ACT_RX:
        {

        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidAdminEventHandler(BleAdm_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch(pstrEvent->enuEventType)
        {
        case BLE_ADM_NOTIF_ENABLED:
        {

        }
        break;

        case BLE_ADM_NOTIF_DISABLED:
        {

        }
        break;

        case BLE_ADM_STATUS_TX:
        {

        }
        break;

        case BLE_ADM_CMD_RX:
        {

        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidQwrErrorHandler(uint32_t u32Error)
{
    APP_ERROR_HANDLER(u32Error);
}

static void vidAdvEventHandler(ble_adv_evt_t enuEvent)
{

}

static Mid_tenuStatus enuBleStackInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Dispatch Softdevice enable request */
    if(NRF_SUCCESS == nrf_sdh_enable_request())
    {
        /* Apply Softdevice's default configuration */
        uint32_t u32RamStart = BLE_RAM_START_ADDRESS;
        if(NRF_SUCCESS == nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &u32RamStart))
        {
            /* Enable BLE Softdevice */
            enuRetVal = (NRF_SUCCESS == nrf_sdh_ble_enable(&u32RamStart))
                                        ?Middleware_Success
                                        :Middleware_Failure;

            if(Middleware_Success == enuRetVal)
            {
                /* Register a handler to process BLE events received from Softdevice */
                NRF_SDH_BLE_OBSERVER(BleObserver,
                                     BLE_OBSERVER_PRIO,
                                     vidBleEventHandler,
                                     NULL);
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

static Mid_tenuStatus enuBleGattInit(void)
{
    return (NRF_SUCCESS == nrf_ble_gatt_init(&BleGattInstance, NULL))
                                             ?Middleware_Success
                                             :Middleware_Failure;
}

static Mid_tenuStatus enuBleServicesInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;
    BleReg_tstrInit strUseRegInit = {0};
    BleAtt_tstrInit strKeyAttInit = {0};
    BleAdm_tstrInit strAdmInit = {0};
    nrf_ble_qwr_init_t strQwrInit = {0};

    /* Initialize Queued Write Module */
    strQwrInit.error_handler = vidQwrErrorHandler;
    if(NRF_SUCCESS == nrf_ble_qwr_init(&BleQwrInstance, &strQwrInit))
    {
        /* Initialize User Registration service */
        strUseRegInit.pfUseRegEvtHandler = vidUseRegEventHandler;
        if(Middleware_Success == enuBleUseRegInit(&BleUseRegInstance, &strUseRegInit))
        {
            /* Initialize Key Attribution service */
            strKeyAttInit.pfKeyAttEvtHandler = vidKeyAttEventHandler;
            if(Middleware_Success == enuBleKeyAttInit(&BleKeyAttInstance, &strKeyAttInit))
            {
                /* Initialize Admin User service */
                strAdmInit.pfAdmEvtHandler = vidAdminEventHandler;
                enuRetVal = enuBleAdmInit(&BleAdminInstance, &strAdmInit);
            }
        }
    }

    return enuRetVal;
}

static Mid_tenuStatus enuAdvertisingInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;
    ble_advertising_init_t strAdvertisingInit;

    /* Apply advertising module's settings */
    memset(&strAdvertisingInit, 0, sizeof(strAdvertisingInit));
    strAdvertisingInit.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    strAdvertisingInit.advdata.include_appearance = false;
    strAdvertisingInit.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    strAdvertisingInit.srdata.uuids_complete.uuid_cnt = sizeof(strAdvUuids) / sizeof(strAdvUuids[0]);
    strAdvertisingInit.srdata.uuids_complete.p_uuids = strAdvUuids;
    strAdvertisingInit.config.ble_adv_fast_enabled  = true;
    strAdvertisingInit.config.ble_adv_fast_interval = BLE_ADVERTISING_INTERVAL;
    strAdvertisingInit.config.ble_adv_fast_timeout  = BLE_ADVERTISING_DURATION;
    strAdvertisingInit.evt_handler = vidAdvEventHandler;

    /* Initialize advertising module */
    enuRetVal = (NRF_SUCCESS == ble_advertising_init(&BleAdvInstance,
                                                     &strAdvertisingInit))
                                                     ?Middleware_Success
                                                     :Middleware_Failure;

    if(Middleware_Success == enuRetVal)
    {
        /* Set connection settings tag */
        ble_advertising_conn_cfg_tag_set(&BleAdvInstance, BLE_CONN_CFG_TAG);
    }

    return enuRetVal;
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

    /* Start advertising */
    enuBle_GetNotified(BLE_START_ADVERTISING, NULL);

    /* Ble task's main polling loop */
    while(1)
    {
        /* Process events originating from Ble Stack */
        nrf_sdh_evts_poll();
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

        /* Clear notifications after they've been processed and put task in blocked state */
        (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
Mid_tenuStatus enuBle_Init(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Create task for BLE service */
    if (pdTRUE == xTaskCreate(vidBleTaskFunction,
                              "BLE_Task",
                              MID_BLE_TASK_STACK_SIZE,
                              NULL,
                              MID_BLE_TASK_PRIORITY,
                              &pvBLETaskHandle))
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
                    /* Initialize GATT */
                    if(Middleware_Success == enuBleGattInit())
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