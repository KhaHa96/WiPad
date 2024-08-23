/* -----------------------------   BLE Service for nRF51422   ---------------------------------- */
/*  File      -  BLE Service source file                                                         */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "BLE_Service.h"
#include "NVM_Service.h"
#include "event_groups.h"
#include "semphr.h"
#include "softdevice_handler.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_reg.h"
#include "ble_att.h"
#include "ble_adm.h"
#include "ble_cts_c.h"
#include "ble_db_discovery.h"
#include "peer_manager.h"
#include "AppMgr.h"
#include "App_Types.h"
/*****************************************   DEFINES   *******************************************/
#define BLE_CENTRAL_LINK_COUNT 0
#define BLE_PERIPHERAL_LINK_COUNT 1
#define BLE_ADV_DEVICE_NAME "WiPad"
#define BLE_MIN_CONN_INTERVAL MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define BLE_MAX_CONN_INTERVAL MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define BLE_SLAVE_LATENCY 0
#define BLE_CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)
#define BLE_FIRST_CONN_PARAM_UPDATE_DELAY 5000
#define BLE_REGULAR_CONN_PARAM_UPDATE_DELAY 30000
#define BLE_MAX_NBR_CONN_PARAM_UPDATE_ATTEMPTS 3
#define BLE_ADVERTISING_INTERVAL 64
#define BLE_ADVERTISING_DURATION_IN_SECONDS 180
#define BLE_EVENT_NO_WAIT (TickType_t)0UL
#define BLE_EVENT_MASK (BLE_ADVERTISING_STARTED)

/************************************   PRIVATE MACROS   *****************************************/
#define BLE_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Ble_tstrState))

/* Ble service assert macro */
#define BLE_SERVICE_ASSERT(svc)      \
    (                                \
        (svc == Ble_Registration) || \
        (svc == Ble_Attribution) ||  \
        (svc == Ble_Admin))
/*******************************   PRIVATE FUNCTION PROTOTYPES   *********************************/
static void vidBleStartAdvertising(void);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvBleTaskHandle;
static uint16_t u16ConnHandle = BLE_CONN_HANDLE_INVALID; /* Active connection handle     */
static SemaphoreHandle_t pvBleSemaphoreHandle;
static EventGroupHandle_t pvBleEventGroupHandle;
static ble_use_reg_t strBleUseRegInstance;           /* User registration service's instance*/
static ble_key_att_t strBleKeyAttInstance;          /* Key attribution service's instance */
static ble_adm_use_t strAdmUseInstance;             /* Admin user service's instance   */
static volatile bool bTimeReadingPossible = false; /* Is a CTS reading possible    */
static volatile bool bFirstAdvInCycle = true;      /* Is first time advertising since wake up */
static volatile bool bFlashStorageCleared = false; /* Has flash storage been cleared          */
static vidCtsCallback pfCtsCallback = NULL;        /* Placeholder for CTS callback            */
static ble_cts_c_t BleCtsInstance;                 /* CTS's instance               */

static ble_uuid_t strAdvUuids[] =
    {
        {BLE_USE_REG_UUID_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}};

static const Ble_tstrState strBleStateMachine[] =
    {
        {BLE_ADVERTISING_STARTED, vidBleStartAdvertising}};

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
    vidBleAdmUseEventHandler(&strAdmUseInstance, pstrEvent);
    vidBleKeyAttEventHandler(&strBleKeyAttInstance, pstrEvent);
    vidBleUseRegEventHandler(&strBleUseRegInstance, pstrEvent);
}
static void vidUseRegEventHandler(BleReg_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch (pstrEvent->enuEventType)
        {
        case BLE_REG_NOTIF_ENABLED:
        {
            /* User Registration service's notifications enabled. Notify Registration application */
            (void)AppMgr_enuDispatchEvent(BLE_REG_NOTIF_ENABLED_HEADSUP, NULL);
        }
        break;

        case BLE_REG_NOTIF_DISABLED:
        {
            /* User Registration service's notifications disabled. Notify Registration application */
            (void)AppMgr_enuDispatchEvent(BLE_REG_NOTIF_DISABLED_HEADSUP, NULL);
        }
        break;

        case BLE_REG_ID_PWD_RX:
        {
            /* Received user input on Id/Pwd characteristic. Notify Registration application.
               Note: Data must be preserved until the Registration application receives and
               processes it. */
            Ble_tstrRxData *pstrRxData = (Ble_tstrRxData *)malloc(sizeof(Ble_tstrRxData));

            if(pstrRxData)
            {
                pstrRxData->pu8Data = (uint8_t *)malloc(pstrEvent->strRxData.u16Length + 1);
                pstrRxData->u16Length = pstrEvent->strRxData.u16Length;

                /* Successfully allocated memory for data pointer */
                if(NULL == pstrRxData->pu8Data)
                {
                    /* Free allocated memory */
                    free(pstrRxData);
                }

                /* Copy data into buffer and dispatch it to the Registration application */
                memcpy((void *)pstrRxData->pu8Data, pstrEvent->strRxData.pu8Data, pstrRxData->u16Length);
                (void)AppMgr_enuDispatchEvent(BLE_REG_USER_INPUT_RECEIVED, (void *)pstrRxData);
            }
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
            /* Key Activation service's notifications enabled. Notify Attribution application */
            (void)AppMgr_enuDispatchEvent(BLE_ATT_NOTIF_ENABLED_HEADSUP, NULL);
        }
        break;

        case BLE_ATT_NOTIF_DISABLED:
        {
            /* Key Activation service's notifications disabled. Notify Attribution application */
            (void)AppMgr_enuDispatchEvent(BLE_ATT_NOTIF_DISABLED_HEADSUP, NULL);
        }
        break;

        case BLE_ATT_KEY_ACT_RX:
        {
            /* Received user input on Key Activation characteristic. Notify Attribution application.
               Note: Data must be preserved until the Attribution application receives and
               processes it. */
            Ble_tstrRxData *pstrRxData = (Ble_tstrRxData *)malloc(sizeof(Ble_tstrRxData));

            if(pstrRxData)
            {
                pstrRxData->pu8Data = (uint8_t *)malloc(2);
                pstrRxData->u16Length = 1;

                /* Successfully allocated memory for data pointer */
                if(NULL == pstrRxData->pu8Data)
                {
                    /* Free allocated memory */
                    free(pstrRxData);
                }

                /* Copy data into buffer and dispatch it to the Attribution application */
                memcpy((void *)pstrRxData->pu8Data, &pstrEvent->u8RxByte, pstrRxData->u16Length);
                (void)AppMgr_enuDispatchEvent(BLE_ATT_USER_INPUT_RECEIVED, (void *)pstrRxData);
            }
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}


static void vidKeyAdmEventHandler(BleAdm_tstrEvent *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch(pstrEvent->enuEventType)
        {
        case BLE_ADM_NOTIF_ENABLED:
        {
            /* Admin User service's notifications enabled. Notify Registration application */
            (void)AppMgr_enuDispatchEvent(BLE_ADM_NOTIF_ENABLED_HEADSUP, NULL);
        }
        break;

        case BLE_ADM_NOTIF_DISABLED:
        {
            /* Admin User service's notifications disabled. Notify Registration application */
            (void)AppMgr_enuDispatchEvent(BLE_ADM_NOTIF_DISABLED_HEADSUP, NULL);
        }
        break;

        case BLE_ADM_CMD_RX:
        {
            /* Received user input on User Command characteristic. Notify Registration application.
               Note: Data must be preserved until the Registration application receives and
               processes it. */
            Ble_tstrRxData *pstrRxData = (Ble_tstrRxData *)malloc(sizeof(Ble_tstrRxData));

            if (pstrRxData)
            {
                pstrRxData->pu8Data = (uint8_t *)malloc(pstrEvent->strRxData.u16Length + 1);
                pstrRxData->u16Length = pstrEvent->strRxData.u16Length;

                /* Successfully allocated memory for data pointer */
                if (NULL == pstrRxData->pu8Data)
                {
                    /* Free allocated memory */
                    free(pstrRxData);
                }

                /* Copy data into buffer and dispatch it to the Registration application */
                memcpy((void *)pstrRxData->pu8Data, pstrEvent->strRxData.pu8Data, pstrRxData->u16Length);
                (void)AppMgr_enuDispatchEvent(BLE_ADM_USER_INPUT_RECEIVED, (void *)pstrRxData);
            }
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}
static void vidCtsEventHandler(ble_cts_c_t *pstrCtsInstance, ble_cts_c_evt_t *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrCtsInstance && pstrEvent)
    {
        switch(pstrEvent->evt_type)
        {
        case BLE_CTS_C_EVT_DISCOVERY_COMPLETE:
        {
            /* Current Time Service discovered on Central's GATT server and link has been
               established. Associate established link to this instance of the CTS client by
               assigning connection and characteristic handles to it */
            (void)ble_cts_c_handles_assign(&BleCtsInstance,
                                           pstrEvent->conn_handle,
                                           &pstrEvent->params.char_handles);
            /* Set Current Time reading flag */
            bTimeReadingPossible = true;
        }
        break;

        case BLE_CTS_C_EVT_DISCOVERY_FAILED:
        {
            /* Clear Current Time reading flag */
            bTimeReadingPossible = false;
        }
        break;

        case BLE_CTS_C_EVT_CURRENT_TIME:
        {
            /* Invoke Attribution application's current time data callback */
            pfCtsCallback(&pstrEvent->params.current_time.exact_time_256);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidCtsErrorHandler(uint32_t u32Error)
{
    APP_ERROR_HANDLER(u32Error);
}

static void vidDataBaseDiscHandler(ble_db_discovery_evt_t *pstrEvent)
{
    /* Invoke database discovery handler to handle Current Time Service related events */
    ble_cts_c_on_db_disc_evt(&BleCtsInstance, pstrEvent);
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
static void vidBleEventHandler(ble_evt_t const *pstrEvent, void *pvData)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        /* Secure an established connection */
        pm_sm_evt_handler(pstrEvent);

        switch(pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            /* Preserve current connection handle */
            u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
            /* Trigger connection LED pattern */
            (void)AppMgr_enuDispatchEvent(BLE_CONNECTION_EVENT, NULL);
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            /* Clear connection handle placeholder */
            u16ConnHandle = BLE_CONN_HANDLE_INVALID;
            /* Clear connection handle in Current Time Service's instance structure */
            if(BleCtsInstance.conn_handle == pstrEvent->evt.gap_evt.conn_handle)
            {
                BleCtsInstance.conn_handle = BLE_CONN_HANDLE_INVALID;
            }
            /* Trigger disconnection LED pattern */
            (void)AppMgr_enuDispatchEvent(BLE_DISCONNECTION_EVENT, NULL);
        }
        break;

        case BLE_GAP_EVT_TIMEOUT:
        {
            /* Advertising timed out. Prepare wakeup buttons and go to sleep */
            if (NRF_SUCCESS == bsp_btn_ble_sleep_mode_prepare())
            {
                /* Request clearing space in flash storage */
                if(Middleware_Success == enuNVM_ClearFlashStorage())
                {
                    /* Clearing flash storage is an asynchronous operation. Wait for outcome */
                    while(!bFlashStorageCleared)
                    {
                    }
                }

                /* Enter system-off mode. Wakeup will only be possible through a reset */
                (void)sd_power_system_off();
                /* Empty loop to keep CPU busy in debug mode */
                while (1)
                {
                    __NOP();
                }
            }
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}
static void vidSystemEvtDispatch(uint32_t u32SystemEvent)
{
    ble_evt_t ble_evt;
    // populate the ble_evt structure with the system event data
    vidBleEventHandler((ble_evt_t const *)&ble_evt, ((void *)0));
}
static Mid_tenuStatus enuBleStackInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Set Softdevice's low frequency clock source */
    nrf_clock_lf_cfg_t strLfClkCfg = {.source = NRF_CLOCK_LF_SRC_XTAL,
                                      .rc_ctiv = 0,
                                      .rc_temp_ctiv = 0,
                                      .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};

    /* Initialize the SoftDevice handler module */
    SOFTDEVICE_HANDLER_INIT(&strLfClkCfg, SD_EVT_IRQHandler);

    /* Fetch Softdevice's default enable parameters */
    ble_enable_params_t strBleEnableParams;
    if (NRF_SUCCESS == softdevice_enable_get_default_config(BLE_CENTRAL_LINK_COUNT,
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
                                ? Middleware_Success
                                : Middleware_Failure;
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
                        ? Middleware_Success
                        : Middleware_Failure;
    }

    return enuRetVal;
}
static Mid_tenuStatus enuBleServicesInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;
    BleReg_tstrInit strUseRegInit = {0};
    BleAtt_tstrInit strKetAttInit = {0};
    BleAdm_tstrInit strAdmUseInit = {0};
    // ble_cts_c_init_t strCtsInit = {0};

    /* Initialize User Registration service */
    strUseRegInit.pfUseRegEvtHandler = vidUseRegEventHandler;
    if(Middleware_Success == enuBleUseRegInit(&strBleUseRegInstance, &strUseRegInit))
    {
        /* Initialize Key Attribution service */
        strKetAttInit.pfKeyAttEvtHandler = vidKeyAttEventHandler;
        if(Middleware_Success == enuBleKeyAttInit(&strBleKeyAttInstance, &strKetAttInit))
        {
            strAdmUseInit.pfAdmUseEvtHandler = vidKeyAdmEventHandler;
            if(Middleware_Success == enuBleAdmUseInit(&strAdmUseInstance, &strAdmUseInit))
            {
                /* Initialize Current Time service */
                //  strCtsInit.evt_handler = vidCtsEventHandler;
                //  strCtsInit.error_handler = vidCtsErrorHandler;
                // enuRetVal = (NRF_SUCCESS == ble_cts_c_init(&BleCtsInstance, &strCtsInit))
                //                   ? Middleware_Success
                //                 : Middleware_Failure;
            }
        }
    }

    // return enuRetVal;
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
    strAdvData.uuids_complete.p_uuids = strAdvUuids;

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
               ? Middleware_Success
               : Middleware_Failure;
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
               ? Middleware_Success
               : Middleware_Failure;
}

static void vidBleTaskFunction(void *pvArg)
{
    uint32_t u32Event;
    BaseType_t lSemaphoreAvailable;

    /* TODO: Just for testing! Remove later */
    // enuBle_GetNotified(BLE_ADVERTISING_STARTED, NULL);

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

void vidBleGetCurrentTime(void)
{
    if(bTimeReadingPossible)
    {
        /* Get a current time reading from connected peer. Note: This is an asynchronous
           operation. The current time reading obtained from peer's GATT server can be found
           in the vidCtsEventHandler event handler upon receiving a BLE_CTS_C_EVT_CURRENT_TIME
           event. */
        (void)ble_cts_c_current_time_read(&BleCtsInstance);
    }
}

Mid_tenuStatus enuTransferNotification(Ble_tenuServices enuService, uint8_t *pu8Data, uint16_t *pu16Length)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(BLE_SERVICE_ASSERT(enuService) && pu8Data && pu16Length && (*pu16Length > 0))
    {
        switch(enuService)
        {
        case Ble_Registration:
        {
            /* Send notification to ble_reg's Status characteristic */
            enuRetVal = enuBleUseRegTransferData(&strBleUseRegInstance, pu8Data, pu16Length);
        }
        break;

        case Ble_Attribution:
        {
            /* Send notification to ble_att's Status characteristic */
            enuRetVal = enuBleKeyAttTransferData(&strBleKeyAttInstance, pu8Data, pu16Length);
        }
        break;

        case Ble_Admin:
        {
            /* Send notification to ble_adm's Status characteristic */
            enuRetVal = enuBleAdmUseTransferData(&strAdmUseInstance, pu8Data, pu16Length);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
    return enuRetVal;
}

void vidRegisterCtsCallback(vidCtsCallback pfCallback)
{
    /* Register Attribution application's current time data callback */
    pfCtsCallback = pfCallback;
}

void vidFlashStorageClearCallback(void)
{
    /* Set flash storage cleared flag */
    bFlashStorageCleared = true;
}