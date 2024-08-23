/* -------------------   WiPad Admin User BLE Service for nRF51422   -------------------- */
/*  File      -  WiPad Admin User BLE Service source file                                 */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  July, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "ble_adm.h"
#include "ble_srv_common.h"

/************************************   PRIVATE DEFINES   ****************************************/

#define BLE_ADM_USE_ID_PWD_CHAR_WRITE_REQUEST 1
#define BLE_ADM_USE_ID_PWD_CHAR_WRITE_COMMAND 1
#define BLE_ADM_USE_ID_PWD_CHAR_INIT_OFFSET 0
#define BLE_ADM_USE_ID_PWD_ATT_READ_AUTH 0
#define BLE_ADM_USE_ID_PWD_ATT_WRITE_AUTH 0
#define BLE_ADM_USE_ID_PWD_ATT_VALUE_LENGTH 1
#define BLE_ADM_USE_STATUS_CHAR_NOTIFY 1
#define BLE_ADM_USE_STATUS_CHAR_INIT_OFFSET 0
#define BLE_ADM_USE_STATUS_ATT_READ_AUTH 0
#define BLE_ADM_USE_STATUS_ATT_WRITE_AUTH 0
#define BLE_ADM_USE_STATUS_ATT_VALUE_LENGTH 1
#define BLE_ADM_USE_CCCD_SIZE 2
#define BLE_ADM_USE_NOTIF_EVT_LENGTH 2
#define BLE_ADM_USE_GATTS_EVT_OFFSET 0
#define BLE_ADM_USE_HANDLE_LENGTH 2
#define BLE_ADM_USE_OPCODE_LENGTH 1
#define BLE_ADM_USE_NOTIF_EVT_LENGTH 2
#define BLE_ADM_USE_MAX_DATA_LENGTH (GATT_MTU_SIZE_DEFAULT - BLE_ADM_USE_OPCODE_LENGTH - BLE_ADM_USE_HANDLE_LENGTH)

/*************************************   PRIVATE MACROS   ****************************************/
/* Valid Admin User service instance assertion macro */
#define BLE_ADM_USE_VALID_INSTANCE(INSTANCE) \
    (                                        \
        (INSTANCE) &&                        \
        (BLE_CONN_HANDLE_INVALID != INSTANCE->u16ConnHandle))

/* Characteristic notification enabled assertion macro */
#define BLE_ADM_USE_NOTIF_ENABLED(INSTANCE) \
    (                                       \
        (INSTANCE->bNotificationEnabled))

/* Data transfer assertion macro */
#define BLE_ADM_USE_TX_DATA_ASSERT(DATA, DATA_LENGTH) \
    (                                                 \
        (DATA) &&                                     \
        (DATA_LENGTH) &&                              \
        (*DATA_LENGTH <= BLE_ADM_USE_MAX_DATA_LENGTH))

/* Valid data transfer assertion macro */
#define BLE_ADM_USE_VALID_TRANSFER(INSTANCE, DATA, DATA_LENGTH) \
    (                                                           \
        BLE_ADM_USE_VALID_INSTANCE(INSTANCE) &&                 \
        BLE_ADM_USE_NOTIF_ENABLED(INSTANCE) &&                  \
        BLE_ADM_USE_TX_DATA_ASSERT(DATA, DATA_LENGTH))

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidPeerConnectedCallback(ble_adm_use_t *pstrAdmUseInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if (pstrAdmUseInstance && pstrEvent)
    {
        /* Invoke Admin User service's application-registered event handler */
        pstrAdmUseInstance->u16ConnHandle = pstrEvent->evt.gap_evt.conn_handle;
        BleAdm_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
        strEvent.enuEventType = BLE_ADM_CONNECTED;
        strEvent.pstrAdmUseInstance = pstrAdmUseInstance;
        pstrAdmUseInstance->pfAdmUseEvtHandler(&strEvent);
    }
}

static void vidPeerDisconnectedCallback(ble_adm_use_t *pstrAdmUseInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if (pstrAdmUseInstance && pstrEvent)
    {
        /* Invoke Admin User service's application-registered event handler */
        pstrAdmUseInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        BleAdm_tstrEvent strEvent;
        memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
        strEvent.enuEventType = BLE_ADM_DISCONNECTED;
        strEvent.pstrAdmUseInstance = pstrAdmUseInstance;
        pstrAdmUseInstance->pfAdmUseEvtHandler(&strEvent);
    }
}

static void vidCharWrittenCallback(ble_adm_use_t *pstrAdmUseInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if (pstrAdmUseInstance && pstrEvent)
    {
        BleAdm_tstrEvent strEvent;

        /* Set received event */
        memset(&strEvent, 0, sizeof(BleAdm_tstrEvent));
        strEvent.pstrAdmUseInstance = pstrAdmUseInstance;

        ble_gatts_evt_write_t const *pstrWriteEvent = &pstrEvent->evt.gatts_evt.params.write;
        if ((pstrWriteEvent->handle == pstrAdmUseInstance->strStatusChar.cccd_handle) &&
            (pstrWriteEvent->len == BLE_ADM_USE_NOTIF_EVT_LENGTH))
        {
            /* Decode CCCD value to check whether Peer has enabled notifications on the
               Status characteristic */
            if (ble_srv_is_notification_enabled(pstrWriteEvent->data))
            {
                pstrAdmUseInstance->bNotificationEnabled = true;
                strEvent.enuEventType = BLE_ADM_NOTIF_ENABLED;
            }
            else
            {
                pstrAdmUseInstance->bNotificationEnabled = false;
                strEvent.enuEventType = BLE_ADM_NOTIF_DISABLED;
            }

            /* Invoke Admin User service's application-registered event handler */
            if (pstrAdmUseInstance->pfAdmUseEvtHandler)
            {
                pstrAdmUseInstance->pfAdmUseEvtHandler(&strEvent);
            }
        }
        else if ((pstrWriteEvent->handle == pstrAdmUseInstance->strIdPwdChar.value_handle) &&
                 (pstrAdmUseInstance->pfAdmUseEvtHandler))
        {
            /* Invoke Admin User service's application-registered event handler */
            strEvent.enuEventType = BLE_ADM_ID_PWD_RX;
            strEvent.strRxData.pu8Data = pstrWriteEvent->data;
            strEvent.strRxData.u16Length = pstrWriteEvent->len;
            pstrAdmUseInstance->pfAdmUseEvtHandler(&strEvent);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleAdmUseEventHandler(ble_adm_use_t *pstrAdmUseInstance, ble_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if (pstrAdmUseInstance && pstrEvent)
    {
        switch (pstrEvent->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
        {
            vidPeerConnectedCallback(pstrAdmUseInstance, pstrEvent);
        }
        break;

        case BLE_GAP_EVT_DISCONNECTED:
        {
            vidPeerDisconnectedCallback(pstrAdmUseInstance, pstrEvent);
        }
        break;

        case BLE_GATTS_EVT_WRITE:
        {
            vidCharWrittenCallback(pstrAdmUseInstance, pstrEvent);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

Mid_tenuStatus enuBleAdmUseTransferData(ble_adm_use_t *pstrAdmUseInstance, uint8_t *pu8Data, uint16_t *pu16DataLength)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if (BLE_ADM_USE_VALID_TRANSFER(pstrAdmUseInstance, pu8Data, pu16DataLength))
    {
        ble_gatts_hvx_params_t strHvxParams;

        /* Send notification to Status characteristic */
        memset(&strHvxParams, 0, sizeof(strHvxParams));
        strHvxParams.handle = pstrAdmUseInstance->strStatusChar.value_handle;
        strHvxParams.p_data = pu8Data;
        strHvxParams.p_len = pu16DataLength;
        strHvxParams.type = BLE_GATT_HVX_NOTIFICATION;
        enuRetVal = (NRF_SUCCESS == sd_ble_gatts_hvx(pstrAdmUseInstance->u16ConnHandle,
                                                     &strHvxParams))
                        ? Middleware_Success
                        : Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuBleAdmUseInit(ble_adm_use_t *pstrAdmUseInstance, BleAdm_tstrInit const *pstrAdmUseInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed */
    if (pstrAdmUseInstance && pstrAdmUseInit)
    {
        /* Initialize Admin User service's definition structure */
        pstrAdmUseInstance->u16ConnHandle = BLE_CONN_HANDLE_INVALID;
        pstrAdmUseInstance->bNotificationEnabled = false;
        pstrAdmUseInstance->pfAdmUseEvtHandler = pstrAdmUseInit->pfAdmUseEvtHandler;

        /* Add Admin User service's custom base UUID to Softdevice's service database */
        ble_uuid128_t strBaseUuid = BLE_ADM_USE_BASE_UUID;
        if (NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrAdmUseInstance->u8UuidType))
        {
            /* Add Admin User service to Softdevice's BLE service database */
            ble_uuid_t strUseRegUuid;
            strUseRegUuid.type = pstrAdmUseInstance->u8UuidType;
            strUseRegUuid.uuid = BLE_ADM_USE_UUID_SERVICE;
            if (NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                        &strUseRegUuid,
                                                        &pstrAdmUseInstance->u16ServiceHandle))
            {
                /* Set User Id/Password characteristic metadata */
                ble_gatts_char_md_t strCharMetaData;
                memset(&strCharMetaData, 0, sizeof(strCharMetaData));
                strCharMetaData.char_props.write = BLE_ADM_USE_ID_PWD_CHAR_WRITE_REQUEST;
                strCharMetaData.char_props.write_wo_resp = BLE_ADM_USE_ID_PWD_CHAR_WRITE_COMMAND;
                strCharMetaData.p_char_user_desc = NULL;
                strCharMetaData.p_char_pf = NULL;
                strCharMetaData.p_user_desc_md = NULL;
                strCharMetaData.p_cccd_md = NULL;
                strCharMetaData.p_sccd_md = NULL;

                /* Set User Id/Password characteristic UUID */
                ble_uuid_t strCharUuid;
                strCharUuid.type = pstrAdmUseInstance->u8UuidType;
                strCharUuid.uuid = BLE_ADM_USE_ID_PWD_CHAR_UUID;

                /* Set User Id/Password attribute metadata */
                ble_gatts_attr_md_t strAttMetaData;
                memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                strAttMetaData.rd_auth = BLE_ADM_USE_ID_PWD_ATT_READ_AUTH;
                strAttMetaData.wr_auth = BLE_ADM_USE_ID_PWD_ATT_WRITE_AUTH;
                strAttMetaData.vlen = BLE_ADM_USE_ID_PWD_ATT_VALUE_LENGTH;

                /* Apply User Id/Password attribute settings */
                ble_gatts_attr_t strAttCharValue;
                memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                strAttCharValue.p_uuid = &strCharUuid;
                strAttCharValue.p_attr_md = &strAttMetaData;
                strAttCharValue.init_len = sizeof(uint8_t);
                strAttCharValue.init_offs = BLE_ADM_USE_ID_PWD_CHAR_INIT_OFFSET;
                strAttCharValue.max_len = BLE_ADM_USE_MAX_DATA_LENGTH;

                /* Add User Id/Password characteristic */
                if (NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrAdmUseInstance->u16ServiceHandle,
                                                                   &strCharMetaData,
                                                                   &strAttCharValue,
                                                                   &pstrAdmUseInstance->strIdPwdChar))
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
                    strCharMetaData.char_props.notify = BLE_ADM_USE_STATUS_CHAR_NOTIFY;
                    strCharMetaData.p_char_user_desc = NULL;
                    strCharMetaData.p_char_pf = NULL;
                    strCharMetaData.p_user_desc_md = NULL;
                    strCharMetaData.p_cccd_md = &strCccdMetaData;
                    strCharMetaData.p_sccd_md = NULL;

                    /* Set Status characteristic UUID */
                    ble_uuid_t strCharUuid;
                    strCharUuid.type = pstrAdmUseInstance->u8UuidType;
                    strCharUuid.uuid = BLE_ADM_USE_STATUS_CHAR_UUID;

                    /* Set Status attribute metadata */
                    ble_gatts_attr_md_t strAttMetaData;
                    memset(&strAttMetaData, 0, sizeof(strAttMetaData));
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.read_perm);
                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&strAttMetaData.write_perm);
                    strAttMetaData.vloc = BLE_GATTS_VLOC_STACK;
                    strAttMetaData.rd_auth = BLE_ADM_USE_STATUS_ATT_READ_AUTH;
                    strAttMetaData.wr_auth = BLE_ADM_USE_STATUS_ATT_WRITE_AUTH;
                    strAttMetaData.vlen = BLE_ADM_USE_STATUS_ATT_VALUE_LENGTH;

                    /* Apply Status attribute settings */
                    ble_gatts_attr_t strAttCharValue;
                    memset(&strAttCharValue, 0, sizeof(strAttCharValue));
                    strAttCharValue.p_uuid = &strCharUuid;
                    strAttCharValue.p_attr_md = &strAttMetaData;
                    strAttCharValue.init_len = sizeof(uint8_t);
                    strAttCharValue.init_offs = BLE_ADM_USE_STATUS_CHAR_INIT_OFFSET;
                    strAttCharValue.max_len = BLE_ADM_USE_MAX_DATA_LENGTH;

                    /* Add Status characteristic */
                    enuRetVal = (NRF_SUCCESS == sd_ble_gatts_characteristic_add(pstrAdmUseInstance->u16ServiceHandle,
                                                                                &strCharMetaData,
                                                                                &strAttCharValue,
                                                                                &pstrAdmUseInstance->strStatusChar))
                                    ? Middleware_Success
                                    : Middleware_Failure;
                }
            }
        }
    }

    return enuRetVal;
}