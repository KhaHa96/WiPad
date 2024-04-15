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
#define BLE_USE_REG_STATUS_CHAR_WRITE_REQUEST 1
#define BLE_USE_REG_STATUS_CHAR_WRITE_COMMAND 1
#define BLE_USE_REG_STATUS_MESSAGE_MAX_LENGTH 20U

/************************************   PUBLIC FUNCTIONS   ***************************************/
void vidBleUseRegEventHandler(ble_evt_t const *pstrEvent, void *pvArg)
{

}

Mid_tenuStatus enuBleUseRegInit(ble_use_reg_t *pstrUseRegInstance, BleReg_tstrInit const *pstrUseRegInit)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Map User Registration service's event handler to its placeholder in the service definition
       structure */
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
            /* Add status characteristic */
            ble_add_char_params_t strCharacteristic;
            memset(&strCharacteristic, 0, sizeof(strCharacteristic));
            strCharacteristic.uuid = BLE_USE_REG_ID_CHAR_UUID;
            strCharacteristic.uuid_type = pstrUseRegInstance->u8UuidType;
            strCharacteristic.max_len = BLE_USE_REG_STATUS_MESSAGE_MAX_LENGTH;
            strCharacteristic.init_len = sizeof(uint8_t);
            strCharacteristic.is_var_len = true;
            strCharacteristic.char_props.write = BLE_USE_REG_STATUS_CHAR_WRITE_REQUEST;
            strCharacteristic.char_props.write_wo_resp = BLE_USE_REG_STATUS_CHAR_WRITE_COMMAND;
            strCharacteristic.read_access = SEC_OPEN;
            strCharacteristic.write_access = SEC_OPEN;
            enuRetVal = (NRF_SUCCESS == characteristic_add(pstrUseRegInstance->u16ServiceHandle,
                                                           &strCharacteristic,
                                                           &pstrUseRegInstance->strStatusChar))
                                                           ?Middleware_Success
                                                           :Middleware_Failure;
        }
    }

    return enuRetVal;
}