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
#include "ble_link_ctx_manager.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_USE_REG_BASE_UUID       {0xE6, 0xF8, 0xFC, 0x18, 0x9B, 0x41, 0x4C, 0xDF, \
                                     0xB9, 0x21, 0xD9, 0x00, 0x00, 0x00, 0x59, 0x5F}
#define BLE_USE_REG_UUID_SERVICE     0x2345
#define BLE_USE_REG_ID_PWD_CHAR_UUID 0x2346
#define BLE_USE_REG_STATUS_CHAR_UUID 0x2347

/**************************************   PUBLIC MACROS   ****************************************/
#define BLE_USE_REG_DEF(name, max_clients)                      \
BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(name, _link_ctx_storage),     \
                  (max_clients), sizeof(BleReg_tstrClientCtx)); \
static ble_use_reg_t name =                                     \
{                                                               \
    .pstrLinkCtx = &CONCAT_2(name, _link_ctx_storage)           \
};                                                              \
static ble_use_reg_t name;                                      \
NRF_SDH_BLE_OBSERVER(name ## _obs,                              \
                     BLE_USE_REG_BLE_OBSERVER_PRIO,             \
                     vidBleUseRegEventHandler, &name)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_use_reg_t User Registration service definition structure type.
*/
typedef struct ble_use_reg_s ble_use_reg_t;

/**
 * User Registration event types dispatched back to application-registered callback.
*/
typedef enum
{
    BLE_REG_CONNECTED = 0, /* Peer connected                                       */
    BLE_REG_STATUS_TX,     /* Peer notified of service status                      */
    BLE_REG_ID_PWD_RX      /* Received data from peer on the Id/Pwd characteristic */
}BleReg_tenuEventType;

/**
 * User Registration service client context structure.
*/
typedef struct
{
    bool bNotificationEnabled; /* Indicates whether peer has enabled notification of the Status characteristic */
}BleReg_tstrClientCtx;

/**
 * Rx data structure upon being on the receiving end of a GATT client write event.
*/
typedef struct
{
    uint8_t const *pu8Data; /* Pointer to Rx buffer    */
    uint16_t u16Length;     /* Length of received data */
}BleReg_tstrRxData;

/**
 * User Registration service's event structure.
*/
typedef struct
{
    BleReg_tenuEventType enuEventType;      /* Event type                                        */
    ble_use_reg_t *pstrUseRegInstance;      /* Pointer to the User Registration service instance */
    uint16_t u16ConnHandle;                 /* Connection Handle                                 */
    BleReg_tstrClientCtx *pstrLinkCtx;      /* Pointer to the link context                       */
    BleReg_tstrRxData strRxData;            /* Received data upon a GATT client write event      */
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
    uint8_t u8UuidType;        /* User Registration service's UUID type                           */
    uint16_t u16ServiceHandle; /* User Registration service's handle as provided by the BLE stack */
    ble_gatts_char_handles_t strIdPwdChar;      /* Id/Password characteristic handles             */
    ble_gatts_char_handles_t strStatusChar;     /* Status characteristic handles                  */
    blcm_link_ctx_storage_t *const pstrLinkCtx; /* Pointer to link context storage                */
    BleUseRegEventHandler pfUseRegEvtHandler;   /* User Registration service's event handler      */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
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

/**
 * @brief enuBleUseRegTransferData Initiates data transfer to peer over BLE.
 *
 * @note  This function sends data as a notification to the User Registration
 *        service's Status characteristic
 *
 * @param pstrUseRegInstance Pointer to the User Registration instance structure.
 * @param pu8Data Pointer to data buffer.
 * @param pu16DataLength Pointer to data length in bytes.
 * @param u16ConnHandle Connection Handle of the destination client.
 *
 * @return Mid_tenuStatus Middleware_Success if transfer was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleUseRegTransferData(ble_use_reg_t *pstrUseRegInstance, uint8_t *pu8Data, uint16_t *pu16DataLength, uint16_t u16ConnHandle);

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

#endif  /* _BLE_REG_H_ */