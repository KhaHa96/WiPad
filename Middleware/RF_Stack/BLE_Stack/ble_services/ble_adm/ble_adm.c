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
#define BLE_ADM_STATUS_CHAR_NOTIFY 1
#define BLE_ADM_STATUS_MESSAGE_MAX_LENGTH 20U

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleAdminEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{

}

Mid_tenuStatus enuBleAdmInit(ble_adm_t *pstrAdmInstance, BleAdm_tstrInit const *pstrAdmInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Map Admin service's event handler to its placeholder in the service definition structure */
    pstrAdmInstance->pfAdmEvtHandler = pstrAdmInit->pfAdmEvtHandler;

    /* Add Admin service's custom base UUID to Softdevice's service database */
    ble_uuid128_t strBaseUuid = BLE_ADM_BASE_UUID;
    if(NRF_SUCCESS == sd_ble_uuid_vs_add(&strBaseUuid, &pstrAdmInstance->u8UuidType))
    {
        /* Add Admin service to Softdevice's BLE service database */
        ble_uuid_t strAdmUuid;
        strAdmUuid.type = pstrAdmInstance->u8UuidType;
        strAdmUuid.uuid = BLE_ADM_UUID_SERVICE;
        if(NRF_SUCCESS == sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                                   &strAdmUuid,
                                                   &pstrAdmInstance->u16ServiceHandle))
        {
            /* Add status characteristic */
            ble_add_char_params_t strCharacteristic;
            memset(&strCharacteristic, 0, sizeof(strCharacteristic));
            strCharacteristic.uuid = BLE_ADM_STATUS_CHAR_UUID;
            strCharacteristic.uuid_type = pstrAdmInstance->u8UuidType;
            strCharacteristic.max_len = BLE_ADM_STATUS_MESSAGE_MAX_LENGTH;
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

    return enuRetVal;
}
