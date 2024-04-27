/* -------------------------   WiPad Admin BLE Service for nRF52832   -------------------------- */
/*  File      -  WiPad Admin BLE Service header file                                             */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */
 
#ifndef _BLE_ADMIN_H_
#define _BLE_ADMIN_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "ble_gatts.h"
#include "nrf_sdh_ble.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_ADM_BASE_UUID       {0x1D, 0x5D, 0x13, 0xEC, 0x0A, 0xEB, 0x42, 0xAB, \
                                 0xAA, 0xE0, 0xD1, 0x66, 0x00, 0x00, 0xA6, 0x60}
#define BLE_ADM_UUID_SERVICE     0xACDC
#define BLE_ADM_STATUS_CHAR_UUID 0xACDD

/**************************************   PUBLIC MACROS   ****************************************/
#define BLE_ADM_DEF(name)                            \
static ble_adm_t name;                               \
NRF_SDH_BLE_OBSERVER(name ## _obs,                   \
                     BLE_ADM_BLE_OBSERVER_PRIO,      \
                     vidBleAdminEventHandler, &name)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_adm_t Admin user service definition structure type.
*/
typedef struct ble_adm_s ble_adm_t;

/**
 * Admin user service's event structure.
*/
typedef struct
{
    uint8_t u8Test;
}BleAdm_tstrEvent;

/**
 * BleAdmEventHandler Admin user service's event handler function prototype.
 * 
 * @note This prototype is used to define a BLE event handler for the Admin user service.
 * 
 * @note Functions of this type take one parameter:
 *         - BleAdm_tstrEvent *pstrEvent: Pointer to received event structure.
*/
typedef void (*BleAdmEventHandler)(BleAdm_tstrEvent *pstrEvent);

/**
 * Admin user service's initialization structure.
*/
typedef struct
{
    BleAdmEventHandler pfAdmEvtHandler; /* Event handler to be called when..........  */
}BleAdm_tstrInit;

/**
 * Admin user service's definition structure.
*/
struct ble_adm_s
{
    uint16_t u16ServiceHandle; /* Ble_adm's service handle as provided by the BLE stack */
    ble_gatts_char_handles_t strStatusChar; /* Status characteristic handles            */
    uint8_t u8UuidType;        /* Admin user service's UUID type                        */
    BleAdmEventHandler pfAdmEvtHandler;     /* Admin service's event handler            */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuBleAdmInit Initializes WiPad's Admin user BLE service.
 *
 * @param pstrAdmInstance Pointer to WiPad's Admin user service instance structure.
 * @param pstrAdmInit Pointer to WiPad's Admin user service initialization structure.
 * 
 * @return Mid_tenuStatus Middleware_Success if service initialization was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleAdmInit(ble_adm_t *pstrAdmInstance, BleAdm_tstrInit const *pstrAdmInit);

/**
 * @brief vidBleAdminEventHandler Ble event handler invoked upon receiving an Admin service
 *        Ble event.
 * 
 * @note The Admin user service's event handler declared in the definition structure is invoked
 *       from within the context of this handler.
 * 
 * @param pstrEvent Pointer to received event structure.
 * @param pvArg Pointer to parameters passed to handler.
 * 
 * @return nothing.
*/
void vidBleAdminEventHandler(ble_evt_t const *pstrEvent, void *pvArg);

#endif  /* _BLE_ADMIN_H_ */