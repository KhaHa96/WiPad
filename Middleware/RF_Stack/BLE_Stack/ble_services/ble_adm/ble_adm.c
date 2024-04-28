/* -------------------------   WiPad Admin BLE Service for nRF52832   -------------------------- */
/*  File      -  WiPad Admin BLE Service source file                                             */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_adm.h"
#include "ble.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define BLE_ADM_COMMAND_CHAR_WRITE_REQUEST 1U
#define BLE_ADM_COMMAND_CHAR_WRITE_COMMAND 1U
#define BLE_ADM_STATUS_CHAR_NOTIFY         1U
#define BLE_ADM_CCCD_SIZE                  2U
#define BLE_ADM_NOTIF_EVT_LENGTH           2U
#define BLE_ADM_GATTS_EVT_OFFSET           0U
#define BLE_ADM_OPCODE_LENGTH              1U
#define BLE_ADM_HANDLE_LENGTH              2U
#define BLE_ADM_MAX_DATA_LENGTH            (NRF_SDH_BLE_GATT_MAX_MTU_SIZE \
                                            - BLE_ADM_OPCODE_LENGTH       \
                                            - BLE_ADM_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid connection handle assertion macro */
#define BLE_ADM_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE)     \
(                                                          \
    ((CLIENT) && (BLE_CONN_HANDLE_INVALID != CONN_HANDLE)) \
)

/* Characteristic notification enabled assertion macro */
#define BLE_ADM_NOTIF_ENABLED(CLIENT) \
(                                     \
    (CLIENT->bNotificationEnabled)    \
)

/* Data transfer length assertion macro */
#define BLE_ADM_TX_LENGTH_ASSERT(DATA_LENGTH) \
(                                             \
    (DATA_LENGTH <= BLE_ADM_MAX_DATA_LENGTH)  \
)

/* Valid data transfer assertion macro */
#define BLE_ADM_VALID_TRANSFER(CLIENT, CONN_HANDLE, DATA_LENGTH) \
(                                                                \
    BLE_ADM_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE) &&            \
    BLE_ADM_NOTIF_ENABLED(CLIENT)                  &&            \
    BLE_ADM_TX_LENGTH_ASSERT(DATA_LENGTH)                        \
)

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_adm_t *pstrAdmInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrAdmInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAdm_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrAdmInstance->pstrLinkCtx,
                                            pstrEvent->evt.gap_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            /* Decode CCCD value to check whether notifications on the Status characteristic are
               already enabled */
            ble_gatts_value_t strGattsValue;
            uint8_t u8CccdValue[BLE_ADM_CCCD_SIZE];

            memset(&strGattsValue, 0, sizeof(ble_gatts_value_t));
            strGattsValue.p_value = u8CccdValue;
            strGattsValue.len = sizeof(u8CccdValue);
            strGattsValue.offset = BLE_ADM_GATTS_EVT_OFFSET;

            if(NRF_SUCCESS == sd_ble_gatts_value_get(pstrEvent->evt.gap_evt.conn_handle,
                                                     pstrAdmInstance->strStatusChar.cccd_handle,
                                                     &strGattsValue))
            {
                if((pstrAdmInstance->pfAdmEvtHandler) &&
                    ble_srv_is_notification_enabled(strGattsValue.p_value))
                {
                    /* Notifications already enabled */
                    if(pstrClient)
                    {
                        /* Set enabled notification flag */
                        pstrClient->bNotificationEnabled = true;
                    }

                    /* Invoke Admin User service's application-registered event handler */
                    BleAdm_tstrEvent strEvent;
                    memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
                    strEvent.enuEventType = BLE_ADM_NOTIF_ENABLED;
                    strEvent.pstrAdmInstance = pstrAdmInstance;
                    strEvent.u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
                    strEvent.pstrLinkCtx = pstrClient;
                    pstrAdmInstance->pfAdmEvtHandler(&strEvent);
                }
            }
        }
    }
}

static void vidCharWrittenCallback(ble_adm_t *pstrAdmInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrAdmInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAdm_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrAdmInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            BleAdm_tstrEvent strEvent;

            /* Set received event */
            memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
            strEvent.pstrAdmInstance = pstrAdmInstance;
            strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
            strEvent.pstrLinkCtx = pstrClient;

            ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
            if((pstrWriteEvent->handle == pstrAdmInstance->strStatusChar.cccd_handle) &&
               (pstrWriteEvent->len == BLE_ADM_NOTIF_EVT_LENGTH))
            {
                /* Gatts write event corresponds to notifications being enabled on the Status
                   characteristic */
                if (pstrClient)
                {
                    /* Decode CCCD value to check whether Peer has enabled notifications on the
                       Status characteristic */
                    if (ble_srv_is_notification_enabled(pstrWriteEvent->data))
                    {
                        pstrClient->bNotificationEnabled = true;
                        strEvent.enuEventType = BLE_ADM_NOTIF_ENABLED;
                    }
                    else
                    {
                        pstrClient->bNotificationEnabled = false;
                        strEvent.enuEventType = BLE_ADM_NOTIF_DISABLED;
                    }

                    /* Invoke Admin User service's application-registered event handler */
                    if (pstrAdmInstance->pfAdmEvtHandler)
                    {
                        pstrAdmInstance->pfAdmEvtHandler(&strEvent);
                    }
                }
            }
            else if((pstrWriteEvent->handle == pstrAdmInstance->strCmdChar.value_handle) &&
                    (pstrAdmInstance->pfAdmEvtHandler))
            {
                /* Gatts write event corresponds to data written to the Command
                   characteristic. Invoke Admin User service's application-registered
                   event handler */
                strEvent.enuEventType = BLE_ADM_CMD_RX;
                strEvent.strRxData.pu8Data = pstrWriteEvent->data;
                strEvent.strRxData.u16Length = pstrWriteEvent->len;
                pstrAdmInstance->pfAdmEvtHandler(&strEvent);
            }
        }
    }
}

static void vidNotificationSentCallback(ble_adm_t *pstrAdmInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrAdmInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAdm_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrAdmInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            if((pstrClient->bNotificationEnabled) && (pstrAdmInstance->pfAdmEvtHandler))
            {
                /* Invoke Admin User service's application-registered event handler */
                BleAdm_tstrEvent strEvent;
                memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
                strEvent.enuEventType = BLE_ADM_STATUS_TX;
                strEvent.pstrAdmInstance = pstrAdmInstance;
                strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
                strEvent.pstrLinkCtx = pstrClient;
                pstrAdmInstance->pfAdmEvtHandler(&strEvent);
            }
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleAdmEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent && pvArg)
    {
        /* Retrieve Admin User service instance */
        ble_adm_t *pstrAdmInstance = (ble_adm_t *)pvArg;
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrAdmInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrAdmInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        {
            vidNotificationSentCallback(pstrAdmInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleAdmTransferData(ble_adm_t *pstrAdmInstance, uint8_t *pu8Data, uint16_t *pu16DataLength, uint16_t u16ConnHandle)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrAdmInstance && pu8Data && pu16DataLength)
    {
        BleAdm_tstrClientCtx *pstrClient;
        ble_gatts_hvx_params_t strHvxParams;

        /* Fetch link context from link registry based on connection handle */
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrAdmInstance->pstrLinkCtx, u16ConnHandle, (void *)&pstrClient))
        {
            /* Ensure connection handle and data validity */
            if(BLE_ADM_VALID_TRANSFER(pstrClient, u16ConnHandle, *pu16DataLength))
            {
               /* Send notification to Status characteristic */
                memset(&strHvxParams, 0, sizeof(strHvxParams));
                strHvxParams.handle = pstrAdmInstance->strStatusChar.value_handle;
                strHvxParams.p_data = pu8Data;
                strHvxParams.p_len = pu16DataLength;
                strHvxParams.type = BLE_GATT_HVX_NOTIFICATION;
                enuRetVal = (NRF_SUCCESS == sd_ble_gatts_hvx(u16ConnHandle,
                                                             &strHvxParams))
                                                             ?Middleware_Success
                                                             :Middleware_Failure;
            }
        }
    }

    return enuRetVal;
}

Mid_tenuStatus enuBleAdmInit(ble_adm_t *pstrAdmInstance, BleAdm_tstrInit const *pstrAdmInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrAdmInstance && pstrAdmInit)
    {
        /* Map Admin User service's event handler to its placeholder in the service definition
           structure */
        pstrAdmInstance->pfAdmEvtHandler = pstrAdmInit->pfAdmEvtHandler;

        /* Add Admin User service's custom base UUID to Softdevice's service database */
        ble_uuid128_t strBaseUuid = BLE_ADM_BASE_UUID;
        if(NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrAdmInstance->u8UuidType))
        {
            /* Add Admin User service to Softdevice's BLE service database */
            ble_uuid_t strAdmUuid;
            strAdmUuid.type = pstrAdmInstance->u8UuidType;
            strAdmUuid.uuid = BLE_ADM_UUID_SERVICE;
            if(NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                       &strAdmUuid,
                                                       &pstrAdmInstance->u16ServiceHandle))
            {
                /* Add Command characteristic */
                ble_add_char_params_t strCharacteristic;
                memset(&strCharacteristic, 0, sizeof(strCharacteristic));
                strCharacteristic.uuid = BLE_ADM_COMMAND_CHAR_UUID;
                strCharacteristic.uuid_type = pstrAdmInstance->u8UuidType;
                strCharacteristic.max_len = BLE_ADM_MAX_DATA_LENGTH;
                strCharacteristic.init_len = sizeof(uint8_t);
                strCharacteristic.is_var_len = true;
                strCharacteristic.char_props.write = BLE_ADM_COMMAND_CHAR_WRITE_REQUEST;
                strCharacteristic.char_props.write_wo_resp = BLE_ADM_COMMAND_CHAR_WRITE_COMMAND;
                strCharacteristic.read_access = SEC_OPEN;
                strCharacteristic.write_access = SEC_OPEN;
                if(NRF_SUCCESS == characteristic_add(pstrAdmInstance->u16ServiceHandle,
                                                     &strCharacteristic,
                                                     &pstrAdmInstance->strCmdChar))
                {
                    /* Add Status characteristic */
                    memset(&strCharacteristic, 0, sizeof(strCharacteristic));
                    strCharacteristic.uuid = BLE_ADM_STATUS_CHAR_UUID;
                    strCharacteristic.uuid_type = pstrAdmInstance->u8UuidType;
                    strCharacteristic.max_len = BLE_ADM_MAX_DATA_LENGTH;
                    strCharacteristic.init_len = sizeof(uint8_t);
                    strCharacteristic.is_var_len = true;
                    strCharacteristic.char_props.notify = BLE_ADM_STATUS_CHAR_NOTIFY;
                    strCharacteristic.read_access = SEC_OPEN;
                    strCharacteristic.write_access = SEC_OPEN;
                    strCharacteristic.cccd_write_access = SEC_OPEN;
                    enuRetVal = (NRF_SUCCESS == characteristic_add(pstrAdmInstance->u16ServiceHandle,
                                                                   &strCharacteristic,
                                                                   &pstrAdmInstance->strStatusChar))
                                                                   ?Middleware_Success
                                                                   :Middleware_Failure;
                }
            }
        }
    }

    return enuRetVal;
}