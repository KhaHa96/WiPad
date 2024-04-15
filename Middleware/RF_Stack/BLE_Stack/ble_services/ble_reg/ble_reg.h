/* -------------------   WiPad User Registration BLE Service for nRF52832   -------------------- */
/*  File      -  WiPad User Registration BLE Service header file                                 */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */
 
#ifndef _BLE_REG_H_
#define _BLE_REG_H_

/****************************************   INCLUDES   *******************************************/
#include "Middleware_types.h"
#include "ble_gatts.h"
#include "nrf_sdh_ble.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_USE_REG_BASE_UUID       {0xE6, 0xF8, 0xFC, 0x18, 0x9B, 0x41, 0x4C, 0xDF, \
                                     0xB9, 0x21, 0xD9, 0x00, 0x00, 0x00, 0x59, 0x5F}
#define BLE_USE_REG_UUID_SERVICE     0x2345
#define BLE_USE_REG_ID_CHAR_UUID     0x2346

/**************************************   PUBLIC MACROS   ****************************************/
#define BLE_USE_REG_DEF(name)                         \
static ble_use_reg_t name;                            \
NRF_SDH_BLE_OBSERVER(name ## _obs,                    \
                     BLE_USE_REG_BLE_OBSERVER_PRIO,   \
                     vidBleUseRegEventHandler, &name)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_use_reg_t User Registration service definition structure type.
*/
typedef struct ble_use_reg_s ble_use_reg_t;

/**
 * User Registration service's event structure.
*/
typedef struct
{
    uint8_t u8Test;
}BleReg_tstrEvent;

/**
 * BleUseRegEventHandler User Registration service's event handler function prototype.
 * 
 * @note This prototype is used to define a BLE event handler for the User Registration service.
 * 
 * @note Functions of this type take one parameter:
 *         - BleReg_tstrEvent *pstrEvent: Pointer to received event structure.
*/
typedef void (*BleUseRegEventHandler)(BleReg_tstrEvent *pstrEvent);

/**
 * User Registration service's initialization structure.
*/
typedef struct
{
    BleUseRegEventHandler pfUseRegEvtHandler; /* Event handler to be called when..........  */
}BleReg_tstrInit;

/**
 * User Registration service's definition structure.
*/
struct ble_use_reg_s
{
    uint16_t u16ServiceHandle; /* User Registration service's handle as provided by the BLE stack */
    ble_gatts_char_handles_t strStatusChar;   /* Status characteristic handles                    */
    uint8_t u8UuidType;        /* User Registration service's UUID type                           */
    BleUseRegEventHandler pfUseRegEvtHandler; /* User Registration service's event handler        */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuBleUseRegInit Initializes WiPad's User Registration BLE service.
 *
 * @param pstrUseRegInstance Pointer to WiPad's User Registration service instance structure.
 * @param pstrUseRegInit Pointer to WiPad's User Registration service initialization structure.
 * 
 * @return Mid_tenuStatus Middleware_Success if service initialization was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleUseRegInit(ble_use_reg_t *pstrUseRegInstance, BleReg_tstrInit const *pstrUseRegInit);

/**
 * @brief vidBleUseRegEventHandler Ble event handler invoked upon receiving a User Registration
 *        service Ble event.
 * 
 * @note The User Registration service's event handler declared in the definition structure is invoked
 *       from within the context of this handler.
 * 
 * @param pstrEvent Pointer to received event structure.
 * @param pvArg Pointer to parameters passed to handler.
 * 
 * @return nothing.
*/
void vidBleUseRegEventHandler(ble_evt_t const *pstrEvent, void *pvArg);

#endif  /* _BLE_REG_H_ */