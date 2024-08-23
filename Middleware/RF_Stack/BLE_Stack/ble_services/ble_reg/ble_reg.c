/* -------------------   WiPad User Registration BLE Service for nRF51422   -------------------- */
/*  File      -  WiPad User Registration BLE Service source file                                 */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_reg.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define BLE_USE_REG_ID_PWD_CHAR_WRITE_REQUEST 1
#define BLE_USE_REG_ID_PWD_CHAR_WRITE_COMMAND 1
#define BLE_USE_REG_ID_PWD_CHAR_INIT_OFFSET   0
#define BLE_USE_REG_ID_PWD_ATT_READ_AUTH      0
#define BLE_USE_REG_ID_PWD_ATT_WRITE_AUTH     0
#define BLE_USE_REG_ID_PWD_ATT_VALUE_LENGTH   1
#define BLE_USE_REG_STATUS_CHAR_NOTIFY        1
#define BLE_USE_REG_STATUS_CHAR_INIT_OFFSET   0
#define BLE_USE_REG_STATUS_ATT_READ_AUTH      0
#define BLE_USE_REG_STATUS_ATT_WRITE_AUTH     0
#define BLE_USE_REG_STATUS_ATT_VALUE_LENGTH   1
#define BLE_USE_REG_CCCD_SIZE                 2
#define BLE_USE_REG_NOTIF_EVT_LENGTH          2
#define BLE_USE_REG_GATTS_EVT_OFFSET          0
#define BLE_USE_REG_OPCODE_LENGTH             1
#define BLE_USE_REG_HANDLE_LENGTH             2
#define BLE_USE_REG_MAX_DATA_LENGTH           (GATT_MTU_SIZE_DEFAULT         \
                                               - BLE_USE_REG_OPCODE_LENGTH   \
                                               - BLE_USE_REG_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid User Registration service instance assertion macro */
#define BLE_USE_REG_VALID_INSTANCE(INSTANCE)                \
(                                                           \
    (INSTANCE)                                           && \
    (BLE_CONN_HANDLE_INVALID != INSTANCE->u16ConnHandle)    \
)

/* Characteristic notification enabled assertion macro */
#define BLE_USE_REG_NOTIF_ENABLED(INSTANCE) \
(                                           \
    (INSTANCE->bNotificationEnabled)        \
)

/* Data transfer assertion macro */
#define BLE_USE_REG_TX_DATA_ASSERT(DATA, DATA_LENGTH) \
(                                                     \
    (DATA)                                        &&  \
    (DATA_LENGTH)                                 &&  \
    (*DATA_LENGTH <= BLE_USE_REG_MAX_DATA_LENGTH)     \
)

/* Valid data transfer assertion macro */
#define BLE_USE_REG_VALID_TRANSFER(INSTANCE, DATA, DATA_LENGTH) \
(                                                               \
    BLE_USE_REG_VALID_INSTANCE(INSTANCE)          &&            \
    BLE_USE_REG_NOTIF_ENABLED(INSTANCE)           &&            \
    BLE_USE_REG_TX_DATA_ASSERT(DATA, DATA_LENGTH)               \
)

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        /* Invoke User Registration service's application-registered event handler */
        pstrUseRegInstance->u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
        BleReg_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
        strEvent.enuEventType = BLE_REG_CONNECTED;
        strEvent.pstrUseRegInstance = pstrUseRegInstance;
        pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
    }
}

static void vidPeerDisconnectedCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        /* Invoke User Registration service's application-registered event handler */
        pstrUseRegInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        BleReg_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
        strEvent.enuEventType = BLE_REG_DISCONNECTED;
        strEvent.pstrUseRegInstance = pstrUseRegInstance;
        pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
    }
}

static void vidCharWrittenCallback(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        BleReg_tstrEvent strEvent;

        /* Set received event */
        memset(&strEvent, 0, sizeof(BleReg_tstrEvent));
        strEvent.pstrUseRegInstance = pstrUseRegInstance;

        ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
        if((pstrWriteEvent->handle == pstrUseRegInstance->strStatusChar.cccd_handle) &&
           (pstrWriteEvent->len == BLE_USE_REG_NOTIF_EVT_LENGTH))
        {
            /* Decode CCCD value to check whether Peer has enabled notifications on the
               Status characteristic */
            if(ble_srv_is_notification_enabled(pstrWriteEvent->data))
            {
                pstrUseRegInstance->bNotificationEnabled = true;
                strEvent.enuEventType = BLE_REG_NOTIF_ENABLED;
            }
            else
            {
                pstrUseRegInstance->bNotificationEnabled = false;
                strEvent.enuEventType = BLE_REG_NOTIF_DISABLED;
            }

            /* Invoke User Registration service's application-registered event handler */
            if(pstrUseRegInstance->pfUseRegEvtHandler)
            {
                pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
            }
        }
        else if((pstrWriteEvent->handle == pstrUseRegInstance->strIdPwdChar.value_handle) &&
                (pstrUseRegInstance->pfUseRegEvtHandler))
        {
            /* Invoke User Registration service's application-registered event handler */
            strEvent.enuEventType = BLE_REG_ID_PWD_RX;
            strEvent.strRxData.pu8Data = pstrWriteEvent->data;
            strEvent.strRxData.u16Length = pstrWriteEvent->len;
            pstrUseRegInstance->pfUseRegEvtHandler(&strEvent);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleUseRegEventHandler(ble_use_reg_t *pstrUseRegInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrEvent)
    {
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            vidPeerDisconnectedCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrUseRegInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleUseRegTransferData(ble_use_reg_t *pstrUseRegInstance, uint8_t *pu8Data, uint16_t *pu16DataLength)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(BLE_USE_REG_VALID_TRANSFER(pstrUseRegInstance, pu8Data, pu16DataLength))
    {
        ble_gatts_hvx_params_t strHvxParams;

        /* Send notification to Status characteristic */
        memset(&strHvxParams, 0, sizeof(strHvxParams));
        strHvxParams.handle = pstrUseRegInstance->strStatusChar.value_handle;
        strHvxParams.p_data = pu8Data;
        strHvxParams.p_len = pu16DataLength;
        strHvxParams.type = BLE_GATT_HVX_NOTIFICATION;
        enuRetVal = (NRF_SUCCESS == sd_ble_gatts_hvx(pstrUseRegInstance->u16ConnHandle,
                                                     &strHvxParams))
                                                     ?Middleware_Success
                                                     :Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuBleUseRegInit(ble_use_reg_t *pstrUseRegInstance, BleReg_tstrInit const *pstrUseRegInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if(pstrUseRegInstance && pstrUseRegInit)
    {
        /* Initialize User Registration service's definition structure */
        pstrUseRegInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        pstrUseRegInstance->bNotificationEnabled = false;
        pstrUseRegInstance->pfUseRegEvtHandler = pstrUseRegInit->pfUseRegEvtHandler;

        /* Add User Registration service's custom base UUID to Softdevice's service database */
        ble_uuid128_t strBaseUuid = BLE_USE_REG_BASE_UUID;
        if(NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrUseRegInstance->u8UuidType))
        {
            /* Add User Registration service to Softdevice's BLE service database */
            ble_uuid_t strUseRegUuid;
            strUseRegUuid.type = pstrUseRegInstance->u8UuidType;
            strUseRegUuid.uuid = BLE_USE_REG_UUID_SERVICE;
            if(NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                       &strUseRegUuid,
                                                       &pstrUseRegInstance->u16ServiceHandle))
            {
                /* Set User Id/Password characteristic metadata */
                ble_gatts_char_md_t strCharMetaData;
                memset(&strCharMetaData, 0, sizeof(strCharMetaData));
                strCharMetaData.char_props.write = BLE_USE_REG_ID_PWD_CHAR_WRITE_REQUEST;
                strCharMetaData.char_props.write_wo_resp = BLE_USE_REG_ID_PWD_CHAR_WRITE_COMMAND;
                strCharMetaData.p_char_user_desc = NULL;
                strCharMetaData.p_char_pf = NULL;
                strCharMetaData.p_user_desc_md = NULL;
                strCharMetaData.p_cccd_md = NULL;
                strCharMetaData.p_sccd_md = NULL;

                /* Set User Id/Password characteristic UUID */
                ble_uuid_t strCharUuid;
                strCharUuid.type = pstrUseRegInstance->u8UuidType;
                strCharUuid.uuid = BLE_USE_REG_ID_PWD_CHAR_UUID;

                /* Set User Id/Password attribute metadata */
                ble_gatts_attr_md_t strAttMetaData;
                memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                strAttMetaData.rd_auth = BLE_USE_REG_ID_PWD_ATT_READ_AUTH;
                strAttMetaData.wr_auth = BLE_USE_REG_ID_PWD_ATT_WRITE_AUTH;
                strAttMetaData.vlen = BLE_USE_REG_ID_PWD_ATT_VALUE_LENGTH;

                /* Apply User Id/Password attribute settings */
                ble_gatts_attr_t strAttCharValue;
                memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                strAttCharValue.p_uuid = &strCharUuid;
                strAttCharValue.p_attr_md = &strAttMetaData;
                strAttCharValue.init_len = sizeof(uint8_t);
                strAttCharValue.init_offs = BLE_USE_REG_ID_PWD_CHAR_INIT_OFFSET;
                strAttCharValue.max_len = BLE_USE_REG_MAX_DATA_LENGTH;

                /* Add User Id/Password characteristic */
                if(NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrUseRegInstance->u16ServiceHandle,
                                                                  &strCharMetaData,
                                                                  &strAttCharValue,
                                                                  &pstrUseRegInstance->strIdPwdChar))
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
                    strCharMetaData.char_props.notify = BLE_USE_REG_STATUS_CHAR_NOTIFY;
                    strCharMetaData.p_char_user_desc = NULL;
                    strCharMetaData.p_char_pf = NULL;
                    strCharMetaData.p_user_desc_md = NULL;
                    strCharMetaData.p_cccd_md = &strCccdMetaData;
                    strCharMetaData.p_sccd_md = NULL;

                    /* Set Status characteristic UUID */
                    ble_uuid_t strCharUuid;
                    strCharUuid.type = pstrUseRegInstance->u8UuidType;
                    strCharUuid.uuid = BLE_USE_REG_STATUS_CHAR_UUID;

                    /* Set Status attribute metadata */
                    ble_gatts_attr_md_t strAttMetaData;
                    memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                    strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                    strAttMetaData.rd_auth = BLE_USE_REG_STATUS_ATT_READ_AUTH;
                    strAttMetaData.wr_auth = BLE_USE_REG_STATUS_ATT_WRITE_AUTH;
                    strAttMetaData.vlen = BLE_USE_REG_STATUS_ATT_VALUE_LENGTH;

                    /* Apply Status attribute settings */
                    ble_gatts_attr_t strAttCharValue;
                    memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                    strAttCharValue.p_uuid = &strCharUuid;
                    strAttCharValue.p_attr_md = &strAttMetaData;
                    strAttCharValue.init_len = sizeof(uint8_t);
                    strAttCharValue.init_offs = BLE_USE_REG_STATUS_CHAR_INIT_OFFSET;
                    strAttCharValue.max_len = BLE_USE_REG_MAX_DATA_LENGTH;

                    /* Add Status characteristic */
                    enuRetVal = (NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrUseRegInstance->u16ServiceHandle,
                                                                                &strCharMetaData,
                                                                                &strAttCharValue,
                                                                                &pstrUseRegInstance->strStatusChar))
                                                                                ?Middleware_Success
                                                                                :Middleware_Failure;
                }
            }
        }
    }

    return enuRetVal;
}