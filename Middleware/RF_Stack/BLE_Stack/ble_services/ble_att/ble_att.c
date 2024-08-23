/* --------------------   WiPad Key Attribution BLE Service for nRF51422   --------------------- */
/*  File      -  WiPad Key Attribution BLE Service source file                                   */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_att.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define BLE_KEY_ATT_ACTIVATION_CHAR_READ        0U
#define BLE_KEY_ATT_ACTIVATION_CHAR_WRITE       1U
#define BLE_KEY_ATT_ACTIVATION_MAX_LENGTH       sizeof(uint8_t)
#define BLE_KEY_ATT_ACTIVATION_CHAR_INIT_OFFSET 0U
#define BLE_KEY_ATT_ACTIVATION_READ_AUTH        0U
#define BLE_KEY_ATT_ACTIVATION_WRITE_AUTH       0U
#define BLE_KEY_ATT_ACTIVATION_VALUE_LENGTH     0U
#define BLE_KEY_ATT_STATUS_CHAR_NOTIFY          1U
#define BLE_KEY_ATT_STATUS_CHAR_INIT_OFFSET     0U
#define BLE_KEY_ATT_STATUS_ATT_READ_AUTH        0U
#define BLE_KEY_ATT_STATUS_ATT_WRITE_AUTH       0U
#define BLE_KEY_ATT_STATUS_ATT_VALUE_LENGTH     1U
#define BLE_KEY_ATT_NOTIF_EVT_LENGTH            2U
#define BLE_KEY_ATT_OPCODE_LENGTH               1U
#define BLE_KEY_ATT_HANDLE_LENGTH               2U
#define BLE_KEY_ATT_MAX_DATA_LENGTH             (GATT_MTU_SIZE_DEFAULT        \
                                                 - BLE_KEY_ATT_OPCODE_LENGTH  \
                                                 - BLE_KEY_ATT_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid Key Attribution service instance assertion macro */
#define BLE_KEY_ATT_VALID_INSTANCE(INSTANCE)                \
(                                                           \
    (INSTANCE)                                           && \
    (BLE_CONN_HANDLE_INVALID != INSTANCE->u16ConnHandle)    \
)

/* Characteristic notification enabled assertion macro */
#define BLE_KEY_ATT_NOTIF_ENABLED(INSTANCE) \
(                                           \
    (INSTANCE->bNotificationEnabled)        \
)

/* Data transfer assertion macro */
#define BLE_KEY_ATT_TX_DATA_ASSERT(DATA, DATA_LENGTH) \
(                                                     \
    (DATA)                                        &&  \
    (DATA_LENGTH)                                 &&  \
    (*DATA_LENGTH <= BLE_KEY_ATT_MAX_DATA_LENGTH)     \
)

/* Valid data transfer assertion macro */
#define BLE_KEY_ATT_VALID_TRANSFER(INSTANCE, DATA, DATA_LENGTH) \
(                                                               \
    BLE_KEY_ATT_VALID_INSTANCE(INSTANCE)          &&            \
    BLE_KEY_ATT_NOTIF_ENABLED(INSTANCE)           &&            \
    BLE_KEY_ATT_TX_DATA_ASSERT(DATA, DATA_LENGTH)               \
)

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        /* Invoke Key Attribution service's application-registered event handler */
        pstrKeyAttInstance->u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
        BleAtt_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
        strEvent.enuEventType = BLE_ATT_CONNECTED;
        strEvent.pstrKeyAttInstance = pstrKeyAttInstance;
        pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
    }
}

static void vidPeerDisconnectedCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        /* Invoke Key Attribution service's application-registered event handler */
        pstrKeyAttInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        BleAtt_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
        strEvent.enuEventType = BLE_ATT_DISCONNECTED;
        strEvent.pstrKeyAttInstance = pstrKeyAttInstance;
        pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
    }
}

static void vidCharWrittenCallback(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        BleAtt_tstrEvent strEvent;

        /* Set received event */
        memset(&strEvent, 0, sizeof(BleAtt_tstrEvent));
        strEvent.pstrKeyAttInstance = pstrKeyAttInstance;

        ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
        if((pstrWriteEvent->handle == pstrKeyAttInstance->strStatusChar.cccd_handle) &&
           (pstrWriteEvent->len == BLE_KEY_ATT_NOTIF_EVT_LENGTH))
        {
            /* Decode CCCD value to check whether Peer has enabled notifications on the
               Status characteristic */
            if (ble_srv_is_notification_enabled(pstrWriteEvent->data))
            {
                pstrKeyAttInstance->bNotificationEnabled = true;
                strEvent.enuEventType = BLE_ATT_NOTIF_ENABLED;
            }
            else
            {
                pstrKeyAttInstance->bNotificationEnabled = false;
                strEvent.enuEventType = BLE_ATT_NOTIF_DISABLED;
            }

            /* Invoke Key Attribution service's application-registered event handler */
            if (pstrKeyAttInstance->pfKeyAttEvtHandler)
            {
                pstrKeyAttInstance->pfKeyAttEvtHandler(&strEvent);
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

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleKeyAttEventHandler(ble_key_att_t *pstrKeyAttInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrEvent)
    {
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            vidPeerDisconnectedCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrKeyAttInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleKeyAttTransferData(ble_key_att_t *pstrKeyAttInstance, uint8_t *pu8Data, uint16_t *pu16DataLength)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(BLE_KEY_ATT_VALID_TRANSFER(pstrKeyAttInstance, pu8Data, pu16DataLength))
    {
        ble_gatts_hvx_params_t strHvxParams;

        /* Send notification to Status characteristic */
        memset(&strHvxParams, 0, sizeof(strHvxParams));
        strHvxParams.handle = pstrKeyAttInstance->strStatusChar.value_handle;
        strHvxParams.p_data = pu8Data;
        strHvxParams.p_len = pu16DataLength;
        strHvxParams.type = BLE_GATT_HVX_NOTIFICATION;
        enuRetVal = (NRF_SUCCESS == sd_ble_gatts_hvx(pstrKeyAttInstance->u16ConnHandle,
                                                     &strHvxParams))
                                                     ?Middleware_Success
                                                     :Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuBleKeyAttInit(ble_key_att_t *pstrKeyAttInstance, BleAtt_tstrInit const *pstrKeyAttInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrKeyAttInstance && pstrKeyAttInit)
    {
        /* Initialize Key Attribution service's definition structure */
        pstrKeyAttInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        pstrKeyAttInstance->bNotificationEnabled = false;
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
                /* Set Key Attribution characteristic metadata */
                ble_gatts_char_md_t strCharMetaData;
                memset(&strCharMetaData, 0, sizeof(strCharMetaData));
                strCharMetaData.char_props.read = BLE_KEY_ATT_ACTIVATION_CHAR_READ;
                strCharMetaData.char_props.write = BLE_KEY_ATT_ACTIVATION_CHAR_WRITE;
                strCharMetaData.p_char_user_desc = NULL;
                strCharMetaData.p_char_pf = NULL;
                strCharMetaData.p_user_desc_md = NULL;
                strCharMetaData.p_cccd_md = NULL;
                strCharMetaData.p_sccd_md = NULL;

                /* Set Key Attribution characteristic UUID */
                ble_uuid_t strCharUuid;
                strCharUuid.type = pstrKeyAttInstance->u8UuidType;
                strCharUuid.uuid = BLE_KEY_ATT_KEY_CHAR_UUID;

                /* Set Key Attribution attribute metadata */
                ble_gatts_attr_md_t strAttMetaData;
                memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                strAttMetaData.rd_auth = BLE_KEY_ATT_ACTIVATION_READ_AUTH;
                strAttMetaData.wr_auth = BLE_KEY_ATT_ACTIVATION_WRITE_AUTH;
                strAttMetaData.vlen = BLE_KEY_ATT_ACTIVATION_VALUE_LENGTH;

                /* Apply Key Attribution attribute settings */
                ble_gatts_attr_t strAttCharValue;
                memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                strAttCharValue.p_uuid = &strCharUuid;
                strAttCharValue.p_attr_md = &strAttMetaData;
                strAttCharValue.init_len = sizeof(uint8_t);
                strAttCharValue.init_offs = BLE_KEY_ATT_ACTIVATION_CHAR_INIT_OFFSET;
                strAttCharValue.max_len = BLE_KEY_ATT_ACTIVATION_MAX_LENGTH;
                strAttCharValue.p_value = NULL;

                /* Add Key Attribution characteristic */
                if(NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrKeyAttInstance->u16ServiceHandle,
                                                                  &strCharMetaData,
                                                                  &strAttCharValue,
                                                                  &pstrKeyAttInstance->strKeyActChar))
                {
                    /* Set Status characteristic's CCCD attribute's metadata */
                    ble_gatts_attr_md_t strCccdMetaData;
                    memset(&strCccdMetaData, 0, sizeof(strCccdMetaData));
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strCccdMetaData.read_perm);
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strCccdMetaData.write_perm);
                    strCccdMetaData.vloc = BLE_GATTS_VLOC_STACK;

                    /* Set Status characteristic metadata */
                    ble_gatts_char_md_t strCharMetaData;
                    memset(&strCharMetaData, 0, sizeof(strCharMetaData));
                    strCharMetaData.char_props.notify = BLE_KEY_ATT_STATUS_CHAR_NOTIFY;
                    strCharMetaData.p_char_user_desc = NULL;
                    strCharMetaData.p_char_pf = NULL;
                    strCharMetaData.p_user_desc_md = NULL;
                    strCharMetaData.p_cccd_md = &strCccdMetaData;
                    strCharMetaData.p_sccd_md = NULL;

                    /* Set Status characteristic UUID */
                    ble_uuid_t strCharUuid;
                    strCharUuid.type = pstrKeyAttInstance->u8UuidType;
                    strCharUuid.uuid = BLE_KEY_ATT_STATUS_CHAR_UUID;

                    /* Set Status attribute metadata */
                    ble_gatts_attr_md_t strAttMetaData;
                    memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                    strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                    strAttMetaData.rd_auth = BLE_KEY_ATT_STATUS_ATT_READ_AUTH;
                    strAttMetaData.wr_auth = BLE_KEY_ATT_STATUS_ATT_WRITE_AUTH;
                    strAttMetaData.vlen = BLE_KEY_ATT_STATUS_ATT_VALUE_LENGTH;

                    /* Apply Status attribute settings */
                    ble_gatts_attr_t strAttCharValue;
                    memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                    strAttCharValue.p_uuid = &strCharUuid;
                    strAttCharValue.p_attr_md = &strAttMetaData;
                    strAttCharValue.init_len = sizeof(uint8_t);
                    strAttCharValue.init_offs = BLE_KEY_ATT_STATUS_CHAR_INIT_OFFSET;
                    strAttCharValue.max_len = BLE_KEY_ATT_MAX_DATA_LENGTH;

                    /* Add Status characteristic */
                    enuRetVal = (NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrKeyAttInstance->u16ServiceHandle,
                                                                                &strCharMetaData,
                                                                                &strAttCharValue,
                                                                                &pstrKeyAttInstance->strStatusChar))
                                                                                ?Middleware_Success
                                                                                :Middleware_Failure;
                }
            }
        }
    }

    return enuRetVal;
}