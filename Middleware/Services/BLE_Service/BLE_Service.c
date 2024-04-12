/* -----------------------------   BLE Service for nRF52832   ---------------------------------- */
/*  File      -  BLE Service source file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "BLE_Service.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "nrf_ble_gatt.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define WIPAD_MOCK_EVENT      (1 << 0)
#define BLE_CONN_CFG_TAG      1
#define BLE_OBSERVER_PRIO     3
#define BLE_ADV_DEVICE_NAME   "WiPad"
#define BLE_MIN_CONN_INTERVAL MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define BLE_MAX_CONN_INTERVAL MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define BLE_SLAVE_LATENCY     0
#define BLE_CONN_SUP_TIMEOUT  MSEC_TO_UNITS(4000, UNIT_10_MS)
#define BLE_ADV_INTERVAL      300
#define BLE_ADV_DURATION      18000

/************************************   PRIVATE VARIABLES   **************************************/
NRF_BLE_GATT_DEF(BleGattInstance);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvBLETaskHandle;
static EventGroupHandle_t pvBLEEventGroupHandle;

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidBleEventHandler(ble_evt_t const *pstrEvent, void *pvData)
{
    __NOP();
}

static Mid_tenuStatus enuBleStackInit(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Dispatch Softdevice enable request */
    if(NRF_SUCCESS == nrf_sdh_enable_request())
    {
        /* Apply Softdevice's default configuration */
        uint32_t u32RamStart = 0;
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

static void vidBleTaskFunction(void *pvArg)
{
    uint32_t u32Event;

    enuBleStackInit();
    enuBleGapInit();
    enuBleGattInit();

    /* Task function's main busy loop */
    while (1)
    {
        /* Pull for synch events set in event group */
        u32Event = xEventGroupWaitBits(pvBLEEventGroupHandle, WIPAD_MOCK_EVENT, pdTRUE, pdTRUE, portMAX_DELAY);

        if (u32Event)
        {
            __NOP();
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
Mid_tenuStatus enuBle_Init(void)
{
    /* Create task for BLE service */
    xTaskCreate(vidBleTaskFunction,
                "BLE_Task",
                MID_BLE_TASK_STACK_SIZE,
                NULL,
                MID_BLE_TASK_PRIORITY,
                &pvBLETaskHandle);

    pvBLEEventGroupHandle = xEventGroupCreate();

    return (pvBLEEventGroupHandle)?Middleware_Success:Middleware_Failure;
}