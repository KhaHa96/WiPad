/* --------------------   WiPad Key Attribution BLE Service for nRF52832   --------------------- */
/*  File      -  WiPad Key Attribution BLE Service header file                                   */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  April, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */
 
#ifndef _BLE_ATT_H_
#define _BLE_ATT_H_

/****************************************   INCLUDES   *******************************************/
#include "Middleware_types.h"
#include "ble_gatts.h"
#include "nrf_sdh_ble.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_KEY_ATT_BASE_UUID       {0xA3, 0xF1, 0x2C, 0xF6, 0x5A, 0x38, 0x48, 0xC2, \
                                     0xBF, 0xED, 0x57, 0x15, 0x00, 0x00, 0x05, 0x8C}
#define BLE_KEY_ATT_UUID_SERVICE     0x1234
#define BLE_KEY_ATT_STATUS_CHAR_UUID 0x1235

/**************************************   PUBLIC MACROS   ****************************************/
#define BLE_KEY_ATT_DEF(name)                         \
static ble_key_att_t name;                            \
NRF_SDH_BLE_OBSERVER(name ## _obs,                    \
                     BLE_KEY_ATT_BLE_OBSERVER_PRIO,   \
                     vidBleKeyAttEventHandler, &name)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_key_att_t Key Attribution service definition structure type.
*/
typedef struct ble_key_att_s ble_key_att_t;

/**
 * Key Attribution service's event structure.
*/
typedef struct
{
    uint8_t u8Test;
}BleAtt_tstrEvent;

/**
 * BleKeyAttEventHandler Key Attribution service's event handler function prototype.
 * 
 * @note This prototype is used to define a BLE event handler for the Key Attribution service.
 * 
 * @note Functions of this type take one parameter:
 *         - BleAtt_tstrEvent *pstrEvent: Pointer to received event structure.
*/
typedef void (*BleKeyAttEventHandler)(BleAtt_tstrEvent *pstrEvent);

/**
 * Key Attribution service's initialization structure.
*/
typedef struct
{
    BleKeyAttEventHandler pfKeyAttEvtHandler; /* Event handler to be called when..........  */
}BleAtt_tstrInit;

/**
 * Key Attribution service's definition structure.
*/
struct ble_key_att_s
{
    uint16_t u16ServiceHandle; /* Key Attribution service's handle as provided by the BLE stack */
    ble_gatts_char_handles_t strStatusChar;   /* Status characteristic handles                  */
    uint8_t u8UuidType;        /* Key Attribution service's UUID type                           */
    BleKeyAttEventHandler pfKeyAttEvtHandler; /* Key Attribution service's event handler        */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuBleKeyAttInit Initializes WiPad's Key Attribution BLE service.
 *
 * @param pstrKeyAttInstance Pointer to WiPad's Key Attribution service instance structure.
 * @param pstrKeyAttInit Pointer to WiPad's Key Attribution service initialization structure.
 * 
 * @return Mid_tenuStatus Middleware_Success if service initialization was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleKeyAttInit(ble_key_att_t *pstrKeyAttInstance, BleAtt_tstrInit const *pstrKeyAttInit);

/**
 * @brief vidBleKeyAttEventHandler Ble event handler invoked upon receiving a Key Attribution
 *        service Ble event.
 * 
 * @note The Key Attribution service's event handler declared in the definition structure is invoked
 *       from within the context of this handler.
 * 
 * @param pstrEvent Pointer to received event structure.
 * @param pvArg Pointer to parameters passed to handler.
 * 
 * @return nothing.
*/
void vidBleKeyAttEventHandler(ble_evt_t const *pstrEvent, void *pvArg);

#endif  /* _BLE_ATT_H_ */