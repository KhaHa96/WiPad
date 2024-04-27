/* --------------------   WiPad Key Attribution BLE Service for nRF52832   --------------------- */
/*  File      -  WiPad Key Attribution BLE Service source file                                   */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_att.h"
#include "ble.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/

#define BLE_KEY_ATT_ACTIVATION_CHAR_READ      1
#define BLE_KEY_ATT_ACTIVATION_CHAR_WRITE     1
#define BLE_KEY_ATT_ACTIVATION_MAX_LENGTH     sizeof(uint8_t)
#define BLE_KEY_ATT_STATUS_CHAR_NOTIFY        1
#define BLE_KEY_ATT_CCCD_SIZE                 2
#define BLE_KEY_ATT_NOTIF_EVT_LENGTH          2
#define BLE_KEY_ATT_GATTS_EVT_OFFSET          0
#define BLE_KEY_ATT_OPCODE_LENGTH             1
#define BLE_KEY_ATT_HANDLE_LENGTH             2
#define BLE_KEY_ATT_MAX_DATA_LENGTH           (NRF_SDH_BLE_GATT_MAX_MTU_SIZE \
                                               - BLE_KEY_ATT_OPCODE_LENGTH   \
                                               - BLE_KEY_ATT_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid connection handle assertion macro */
#define BLE_KEY_ATT_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE) \
(                                                          \
    ((CLIENT) && (BLE_CONN_HANDLE_INVALID != CONN_HANDLE)) \
)

/* Charactristic notification enabled assertion macro */
#define BLE_KEY_ATT_NOTIF_ENABLED(CLIENT) \
(                                         \
    (CLIENT->bNotificationEnabled)        \
)

/* Data transfer length assertion macro */
#define BLE_KEY_ATT_TX_LENGTH_ASSERT(DATA_LENGTH) \
(                                                 \
    (DATA_LENGTH <= BLE_KEY_ATT_MAX_DATA_LENGTH)  \
)

/* Valid data transfer assertion macro */
#define BLE_KEY_ATT_VALID_TRANSFER(CLIENT, CONN_HANDLE, DATA_LENGTH) \
(                                                                    \
    BLE_KEY_ATT_VALID_CONN_HANDLE(CLIENT, CONN_HANDLE) &&            \
    BLE_KEY_ATT_NOTIF_ENABLED(CLIENT)                  &&            \
    BLE_KEY_ATT_TX_LENGTH_ASSERT(DATA_LENGTH)                        \
)

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAtt_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrKeyAttInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            /* Decode CCCD value to check whether notifications on the Status characteristic are
               already enabled */
            ble_gatts_value_t strGattsValue;
            uint8_t u8CccdValue[BLE_KEY_ATT_CCCD_SIZE];

            memset(&strGattsValue, 0, sizeof(ble_gatts_value_t));
            strGattsValue.p_value = u8CccdValue;
            strGattsValue.len = sizeof(u8CccdValue);
            strGattsValue.offset = BLE_KEY_ATT_GATTS_EVT_OFFSET;

            if(NRF_SUCCESS == sd_ble_gatts_value_get(pstrEvent->evt.gap_evt.conn_handle,
                                                     pstrKeyAttInstance->strStatusChar.cccd_handle,
                                                     &strGattsValue))
            {
                if((pstrKeyAttInstance->pfKeyAttEvtHandler) &&
                    ble_srv_is_notification_enabled(strGattsValue.p_value))
                {
                    if(pstrClient)
                    {
                        /* Set enabled notification flag */
                        pstrClient->bNotificationEnabled = true;
                    }

                    /* Invoke Key Attribution service's application-registered event handler */
                    BleAtt_tstrEvent strEvent;
                    memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
                    strEvent.enuEventType = BLE_ATT_NOTIF_ENABLED;
                    strEvent.pstrKeyAttInstance = pstrKeyAttInstance;
                    strEvent.u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
                    strEvent.pstrLinkCtx = pstrClient;
                    pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
                }
            }
        }
    }
}

static void vidCharWrittenCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAtt_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrKeyAttInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            BleAtt_tstrEvent strEvent;

            /* Set received event */
            memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
            strEvent.pstrKeyAttInstance = pstrKeyAttInstance;
            strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
            strEvent.pstrLinkCtx = pstrClient;

            ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
            if((pstrWriteEvent->handle == pstrKeyAttInstance->strStatusChar.cccd_handle) &&
               (pstrWriteEvent->len == BLE_KEY_ATT_NOTIF_EVT_LENGTH))
            {
                if (pstrClient)
                {
                    /* Decode CCCD value to check whether Peer has enabled notifications on the
                       Status characteristic */
                    if (ble_srv_is_notification_enabled(pstrWriteEvent->data))
                    {
                        pstrClient->bNotificationEnabled = true;
                        strEvent.enuEventType = BLE_ATT_NOTIF_ENABLED;
                    }
                    else
                    {
                        pstrClient->bNotificationEnabled = false;
                        strEvent.enuEventType = BLE_ATT_NOTIF_DISABLED;
                    }

                    /* Invoke Key Attribution service's application-registered event handler */
                    if (pstrKeyAttInstance->pfKeyAttEvtHandler)
                    {
                        pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
                    }
                }
            }
            else if((pstrWriteEvent->handle == pstrKeyAttInstance->strKeyActChar.value_handle) &&
                    (pstrKeyAttInstance->pfKeyAttEvtHandler))
            {
                /* Invoke Key Attribution service's application-registered event handler */
                strEvent.enuEventType = BLE_ATT_KEY_ACT_RX;
                strEvent.u8RxByte = pstrWriteEvent->data[0];
                pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
            }
        }
    }
}

static void vidNotificationSentCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        /* Fetch link context from link registry based on connection handle */
        BleAtt_tstrClientCtx *pstrClient;
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrKeyAttInstance->pstrLinkCtx,
                                            pstrEvent->evt.gatts_evt.conn_handle,
                                            (void *)&pstrClient))
        {
            if((pstrClient->bNotificationEnabled) && (pstrKeyAttInstance->pfKeyAttEvtHandler))
            {
                /* Invoke Key Attribution service's application-registered event handler */
                BleAtt_tstrEvent strEvent;
                memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
                strEvent.enuEventType = BLE_ATT_STATUS_TX;
                strEvent.pstrKeyAttInstance = pstrKeyAttInstance;
                strEvent.u16ConnHandle = pstrEvent->evt.gatts_evt.conn_handle;
                strEvent.pstrLinkCtx = pstrClient;
                pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
            }
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleKeyAttEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent && pvArg)
    {
        ble_key_att_t *pstrKeyAttInstance = (ble_key_att_t *)pvArg;
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        {
            vidNotificationSentCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleKeyAttTransferData(ble_key_att_t *pstrKeyAttInstance, uint8_t *pu8Data, uint16_t *pu16DataLength, uint16_t u16ConnHandle)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pu8Data && pu16DataLength)
    {
        BleAtt_tstrClientCtx *pstrClient;
        ble_gatts_hvx_params_t strHvxParams;

        /* Fetch link context from link registry based on connection handle */
        if(NRF_SUCCESS == blcm_link_ctx_get(pstrKeyAttInstance->pstrLinkCtx, u16ConnHandle, (void *)&pstrClient))
        {
            /* Ensure connection handle and data validity */
            if(BLE_KEY_ATT_VALID_TRANSFER(pstrClient, u16ConnHandle, *pu16DataLength))
            {
                /* Send notification to Status characteristic */
                memset(&strHvxParams, 0, sizeof(strHvxParams));
                strHvxParams.handle = pstrKeyAttInstance->strStatusChar.value_handle;
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

Mid_tenuStatus enuBleKeyAttInit(ble_key_att_t *pstrKeyAttInstance, BleAtt_tstrInit const *pstrKeyAttInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Map Key Attribution service's event handler to its placeholder in the service definition
       structure */
    pstrKeyAttInstance->pfKeyAttEvtHandler = pstrKeyAttInit->pfKeyAttEvtHandler;

    /* Add Key Attribution service's custom base UUID to Softdevice's service database */
    ble_uuid128_t strBaseUuid = BLE_KEY_ATT_BASE_UUID;
    if(NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrKeyAttInstance->u8UuidType))
    {
        /* Add Key Attribution service to Softdevice's BLE service database */
        ble_uuid_t strKeyAttUuid;
        strKeyAttUuid.type = pstrKeyAttInstance->u8UuidType;
        strKeyAttUuid.uuid = BLE_KEY_ATT_UUID_SERVICE;
        if(NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                   &strKeyAttUuid,
                                                   &pstrKeyAttInstance->u16ServiceHandle))
        {
            /* Add Key Activation characteristic */
            ble_add_char_params_t strCharacteristic;
            memset(&strCharacteristic, 0, sizeof(strCharacteristic));
            strCharacteristic.uuid = BLE_KEY_ATT_KEY_CHAR_UUID;
            strCharacteristic.uuid_type = pstrKeyAttInstance->u8UuidType;
            strCharacteristic.init_len = sizeof(uint8_t);
            strCharacteristic.max_len = BLE_KEY_ATT_ACTIVATION_MAX_LENGTH;
            strCharacteristic.char_props.read = BLE_KEY_ATT_ACTIVATION_CHAR_READ;
            strCharacteristic.char_props.write = BLE_KEY_ATT_ACTIVATION_CHAR_WRITE;
            strCharacteristic.read_access  = SEC_OPEN;
            strCharacteristic.write_access = SEC_OPEN;
            if(NRF_SUCCESS == characteristic_add(pstrKeyAttInstance->u16ServiceHandle,
                                                 &strCharacteristic,
                                                 &pstrKeyAttInstance->strKeyActChar))
            {
                /* Add Status characteristic */
                memset(&strCharacteristic, 0, sizeof(strCharacteristic));
                strCharacteristic.uuid = BLE_KEY_ATT_STATUS_CHAR_UUID;
                strCharacteristic.uuid_type = pstrKeyAttInstance->u8UuidType;
                strCharacteristic.max_len = BLE_KEY_ATT_MAX_DATA_LENGTH;
                strCharacteristic.init_len = sizeof(uint8_t);
                strCharacteristic.is_var_len = true;
                strCharacteristic.char_props.notify = BLE_KEY_ATT_STATUS_CHAR_NOTIFY;
                strCharacteristic.read_access = SEC_OPEN;
                strCharacteristic.write_access = SEC_OPEN;
                strCharacteristic.cccd_write_access = SEC_OPEN;
                enuRetVal = (NRF_SUCCESS == characteristic_add(pstrKeyAttInstance->u16ServiceHandle,
                                                               &strCharacteristic,
                                                               &pstrKeyAttInstance->strStatusChar))
                                                               ?Middleware_Success
                                                               :Middleware_Failure;
            }
        }
    }

    return enuRetVal;
}