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
#include "ble_link_ctx_manager.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_ADM_BASE_UUID        {0x1D, 0x5D, 0x13, 0xEC, 0x0A, 0xEB, 0x42, 0xAB, \
                                  0xAA, 0xE0, 0xD1, 0x66, 0x00, 0x00, 0xA6, 0x60}
#define BLE_ADM_UUID_SERVICE      0xACDC
#define BLE_ADM_COMMAND_CHAR_UUID 0xACDD
#define BLE_ADM_STATUS_CHAR_UUID  0xACDE

/**************************************   PUBLIC MACROS   ****************************************/
#define BLE_ADM_DEF(name, max_clients)                          \
BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(name, _link_ctx_storage),     \
                  (max_clients), sizeof(BleAdm_tstrClientCtx)); \
static ble_adm_t name =                                         \
{                                                               \
    .pstrLinkCtx = &CONCAT_2(name, _link_ctx_storage)           \
};                                                              \
static ble_adm_t name;                                          \
NRF_SDH_BLE_OBSERVER(name ## _obs,                              \
                     BLE_ADM_BLE_OBSERVER_PRIO,                 \
                     vidBleAdmEventHandler, &name)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_adm_t Admin user service definition structure type.
*/
typedef struct ble_adm_s ble_adm_t;

/**
 * Admin user service's events dispatched back to application-registered callback.
*/
typedef enum
{
    BLE_ADM_NOTIF_ENABLED = 0, /* Peer enabled notifications on Status characteristic   */
    BLE_ADM_NOTIF_DISABLED,    /* Peer disabled notifications on Status characteristic  */
    BLE_ADM_STATUS_TX,         /* Peer notified of service status                       */
    BLE_ADM_CMD_RX             /* Received data from peer on the command characteristic */
}BleAdm_tenuEventType;

/**
 * Admin User service client context structure.
*/
typedef struct
{
    bool bNotificationEnabled; /* Indicates whether peer has enabled notification on Status characteristic */
}BleAdm_tstrClientCtx;

/**
 * Rx data structure upon being on the receiving end of a GATT client write event.
*/
typedef struct
{
    uint8_t const *pu8Data; /* Pointer to Rx buffer    */
    uint16_t u16Length;     /* Length of received data */
}BleAdm_tstrRxData;

/**
 * User Registration service's event structure.
*/
typedef struct
{
    BleAdm_tenuEventType enuEventType; /* Event type                                   */
    ble_adm_t *pstrAdmInstance;        /* Pointer to Admin User service instance       */
    uint16_t u16ConnHandle;            /* Connection Handle                            */
    BleAdm_tstrClientCtx *pstrLinkCtx; /* Pointer to link context                      */
    BleAdm_tstrRxData strRxData;       /* Received data upon a GATT client write event */
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
    BleAdmEventHandler pfAdmEvtHandler; /* Event handler to be called when peer enables/disables
                                           notifications on the Status characteristic,
                                           notification is sent on the Status characteristic or
                                           data is received on the Command characteristic  */
}BleAdm_tstrInit;

/**
 * Admin user service's definition structure.
*/
struct ble_adm_s
{
    uint8_t u8UuidType;        /* Admin User service's UUID type                           */
    uint16_t u16ServiceHandle; /* Admin User service's handle as provided by the BLE stack */
    ble_gatts_char_handles_t strCmdChar;        /* Command characteristic handles          */
    ble_gatts_char_handles_t strStatusChar;     /* Status characteristic handles           */
    blcm_link_ctx_storage_t *const pstrLinkCtx; /* Pointer to link context storage         */
    BleAdmEventHandler pfAdmEvtHandler;         /* Admin User service's event handler      */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief vidBleAdmEventHandler Ble event handler invoked upon receiving an Admin service
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
void vidBleAdmEventHandler(ble_evt_t const *pstrEvent, void *pvArg);

/**
 * @brief enuBleAdmTransferData Initiates data transfer to peer over BLE.
 *
 * @note  This function sends data as a notification to the Admin User
 *        service's Status characteristic
 *
 * @param pstrAdmInstance Pointer to the Admin User instance structure.
 * @param pu8Data Pointer to data buffer.
 * @param pu16DataLength Pointer to data length in bytes.
 * @param u16ConnHandle Connection Handle of the destination client.
 *
 * @return Mid_tenuStatus Middleware_Success if transfer was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleAdmTransferData(ble_adm_t *pstrAdmInstance, uint8_t *pu8Data, uint16_t *pu16DataLength, uint16_t u16ConnHandle);

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

#endif  /* _BLE_ADMIN_H_ */