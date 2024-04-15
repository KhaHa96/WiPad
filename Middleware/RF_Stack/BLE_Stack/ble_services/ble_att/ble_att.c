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
#define BLE_KEY_ATT_STATUS_CHAR_NOTIFY 1
#define BLE_KEY_ATT_STATUS_MESSAGE_MAX_LENGTH 20U

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleKeyAttEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{

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
            /* Add status characteristic */
            ble_add_char_params_t strCharacteristic;
            memset(&strCharacteristic, 0, sizeof(strCharacteristic));
            strCharacteristic.uuid = BLE_KEY_ATT_STATUS_CHAR_UUID;
            strCharacteristic.uuid_type = pstrKeyAttInstance->u8UuidType;
            strCharacteristic.max_len = BLE_KEY_ATT_STATUS_MESSAGE_MAX_LENGTH;
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

    return enuRetVal;
}
