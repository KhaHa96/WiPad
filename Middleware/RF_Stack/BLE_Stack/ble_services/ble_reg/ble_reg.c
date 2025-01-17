/* -------------------   WiPad User Registration BLE Service for nRF52832   -------------------- */
/*  File      -  WiPad User Registration BLE Service source file                                 */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_reg.h"
#include "ble.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define BLE_USEREG_ID_PWD_CHAR_WRITE_REQUEST 1U
#define BLE_USEREG_ID_PWD_CHAR_WRITE_COMMAND 1U
#define BLE_USEREG_STATUS_CHAR_NOTIFY        1U
#define BLE_USEREG_CCCD_SIZE                 2U
#define BLE_USEREG_NOTIF_EVT_LENGTH          2U
#define BLE_USEREG_GATTS_EVT_OFFSET          0U
#define BLE_USEREG_OPCODE_LENGTH             1U
#define BLE_USEREG_HANDLE_LENGTH             2U
#define BLE_USEREG_MAX_DATA_LENGTH           (NRF_SDH_BLE_GATT_MAX_MTU_SIZE \
                                              - BLE_USEREG_OPCODE_LENGTH    \
                                              - BLE_USEREG_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid connection handle assertion macro */
#define BLE_USEREG_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE)  \
(                                                          \
    ((CLIENT) && (BLE_CONN_HANDLE_INVALID != CONN_HANDLE)) \
)

/* Characteristic notification enabled assertion macro */
#define BLE_USEREG_NOTIF_ENABLED(CLIENT) \
(                                        \
    (CLIENT->bNotificationEnabled)       \
)

/* Data transfer length assertion macro */
#define BLE_USEREG_TX_LENGTH_ASSERT(DATA_LENGTH) \
(                                                \
    (DATA_LENGTH <= BLE_USEREG_MAX_DATA_LENGTH)  \
)

/* Valid data transfer assertion macro */
#define BLE_USEREG_VALID_TRANSFER(CLIENT, CONN_HANDLE, DATA_LENGTH) \
(                                                                   \
    BLE_USEREG_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE) &&            \
    BLE_USEREG_NOTIF_ENABLED(CLIENT)                  &&            \
    BLE_USEREG_TX_LENGTH_ASSERT(DATA_LENGTH)                        \
)

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleReg_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrUseRegInstance->pstrLinkCtx,
                                            pstrEvent->evt.gap_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            /* Decode CCCD value to check whether notifications on the Status characteristic are
               already enabled */
            ble_gatts_value_t strGattsValue;
            uint8_t u8CccdValue[BLE_USEREG_CCCD_SIZE];

            memset(&strGattsValue, 0, sizeof(ble_gatts_value_t));
            strGattsValue.p_value = u8CccdValue;
            strGattsValue.len = sizeof(u8CccdValue);
            strGattsValue.offset = BLE_USEREG_GATTS_EVT_OFFSET;

            if(NRF_SUCCESS == sd_ble_gatts_value_get(pstrEvent->evt.gap_evt.conn_handle,
                                                     pstrUseRegInstance->strStatusChar.cccd_handle,
                                                     &strGattsValue))
            {
                if((pstrUseRegInstance->pfUseRegEvtHandler) &&
                    ble_srv_is_notification_enabled(strGattsValue.p_value))
                {
                    /* Notifications already enabled */
                    if(pstrClient)
                    {
                        /* Set enabled notification flag */
                        pstrClient->bNotificationEnabled = true;
                    }

                    /* Invoke User Registration service's application-registered event handler */
                    BleReg_tstrEvent strEvent;
                    memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
                    strEvent.enuEventType = BLE_REG_NOTIF_ENABLED;
                    strEvent.pstrUseRegInstance = pstrUseRegInstance;
                    strEvent.u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
                    strEvent.pstrLinkCtx = pstrClient;
                    pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
                }
            }
        }
    }
}

static void vidCharWrittenCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleReg_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrUseRegInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            BleReg_tstrEvent strEvent;

            /* Set received event */
            memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
            strEvent.pstrUseRegInstance = pstrUseRegInstance;
            strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
            strEvent.pstrLinkCtx = pstrClient;

            ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
            if((pstrWriteEvent->handle == pstrUseRegInstance->strStatusChar.cccd_handle) &&
               (pstrWriteEvent->len == BLE_USEREG_NOTIF_EVT_LENGTH))
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
                        strEvent.enuEventType = BLE_REG_NOTIF_ENABLED;
                    }
                    else
                    {
                        pstrClient->bNotificationEnabled = false;
                        strEvent.enuEventType = BLE_REG_NOTIF_DISABLED;
                    }

                    /* Invoke User Registration service's application-registered event handler */
                    if (pstrUseRegInstance->pfUseRegEvtHandler)
                    {
                        pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
                    }
                }
            }
            else if((pstrWriteEvent->handle == pstrUseRegInstance->strIdPwdChar.value_handle) &&
                    (pstrUseRegInstance->pfUseRegEvtHandler))
            {
                /* Gatts write event corresponds to data written to the ID/Password
                   characteristic. Invoke User Registration service's application-registered
                   event handler */
                strEvent.enuEventType = BLE_REG_ID_PWD_RX;
                strEvent.strRxData.pu8Data = pstrWriteEvent->data;
                strEvent.strRxData.u16Length = pstrWriteEvent->len;
                pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
            }
        }
    }
}

static void vidNotificationSentCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleReg_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrUseRegInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            if((pstrClient->bNotificationEnabled) && (pstrUseRegInstance->pfUseRegEvtHandler))
            {
                /* Invoke User Registration service's application-registered event handler */
                BleReg_tstrEvent strEvent;
                memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
                strEvent.enuEventType = BLE_REG_STATUS_TX;
                strEvent.pstrUseRegInstance = pstrUseRegInstance;
                strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
                strEvent.pstrLinkCtx = pstrClient;
                pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
            }
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleUseRegEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent && pvArg)
    {
        /* Retrieve User Registration service instance */
        ble_use_reg_t *pstrUseRegInstance = (ble_use_reg_t *)pvArg;
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        {
            vidNotificationSentCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleUseRegTransferData(ble_use_reg_t *pstrUseRegInstance, uint8_t *pu8Data, uint16_t *pu16DataLength, uint16_t u16ConnHandle)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pu8Data && pu16DataLength)
    {
        BleReg_tstrClientCtx *pstrClient;
        ble_gatts_hvx_params_t strHvxParams;

        /* Fetch link context from link registry based on connection handle */
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrUseRegInstance->pstrLinkCtx, u16ConnHandle, (void *)&pstrClient))
        {
            /* Ensure connection handle and data validity */
            if(BLE_USEREG_VALID_TRANSFER(pstrClient, u16ConnHandle, *pu16DataLength))
            {
               /* Send notification to Status characteristic */
                memset(&strHvxParams, 0, sizeof(strHvxParams));
                strHvxParams.handle = pstrUseRegInstance->strStatusChar.value_handle;
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

Mid_tenuStatus enuBleUseRegInit(ble_use_reg_t *pstrUseRegInstance, BleReg_tstrInit const *pstrUseRegInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrUseRegInit)
    {
        /* Map User Registration service's event handler to its placeholder in the service definition
           structure */
        pstrUseRegInstance->pfUseRegEvtHandler = pstrUseRegInit->pfUseRegEvtHandler;

        /* Add User Registration service's custom base UUID to Softdevice's service database */
        ble_uuid128_t strBaseUuid = BLE_USEREG_BASE_UUID;
        if(NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrUseRegInstance->u8UuidType))
        {
            /* Add User Registration service to Softdevice's BLE service database */
            ble_uuid_t strUseRegUuid;
            strUseRegUuid.type = pstrUseRegInstance->u8UuidType;
            strUseRegUuid.uuid = BLE_USEREG_UUID_SERVICE;
            if(NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                       &strUseRegUuid,
                                                       &pstrUseRegInstance->u16ServiceHandle))
            {
                /* Add User Id/Password characteristic */
                ble_add_char_params_t strCharacteristic;
                memset(&strCharacteristic, 0, sizeof(strCharacteristic));
                strCharacteristic.uuid = BLE_USEREG_ID_PWD_CHAR_UUID;
                strCharacteristic.uuid_type = pstrUseRegInstance->u8UuidType;
                strCharacteristic.max_len = BLE_USEREG_MAX_DATA_LENGTH;
                strCharacteristic.init_len = sizeof(uint8_t);
                strCharacteristic.is_var_len = true;
                strCharacteristic.char_props.write = BLE_USEREG_ID_PWD_CHAR_WRITE_REQUEST;
                strCharacteristic.char_props.write_wo_resp = BLE_USEREG_ID_PWD_CHAR_WRITE_COMMAND;
                strCharacteristic.read_access = SEC_OPEN;
                strCharacteristic.write_access = SEC_OPEN;
                if(NRF_SUCCESS == characteristic_add(pstrUseRegInstance->u16ServiceHandle,
                                                     &strCharacteristic,
                                                     &pstrUseRegInstance->strIdPwdChar))
                {
                    /* Add Status characteristic */
                    memset(&strCharacteristic, 0, sizeof(strCharacteristic));
                    strCharacteristic.uuid = BLE_USEREG_STATUS_CHAR_UUID;
                    strCharacteristic.uuid_type = pstrUseRegInstance->u8UuidType;
                    strCharacteristic.max_len = BLE_USEREG_MAX_DATA_LENGTH;
                    strCharacteristic.init_len = sizeof(uint8_t);
                    strCharacteristic.is_var_len = true;
                    strCharacteristic.char_props.notify = BLE_USEREG_STATUS_CHAR_NOTIFY;
                    strCharacteristic.read_access = SEC_OPEN;
                    strCharacteristic.write_access = SEC_OPEN;
                    strCharacteristic.cccd_write_access = SEC_OPEN;
                    enuRetVal = (NRF_SUCCESS == characteristic_add(pstrUseRegInstance->u16ServiceHandle,
                                                                   &strCharacteristic,
                                                                   &pstrUseRegInstance->strStatusChar))
                                                                   ?Middleware_Success
                                                                   :Middleware_Failure;
                }
            }
        }
    }

    return enuRetVal;
}