/* -------------------   WiPad Admin User BLE Service for nRF51422   -------------------- */
/*  File      -  WiPad Admin User service BLE Service header file                                 */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  July, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _BLE_ADM_H_
#define _BLE_ADM_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "ble.h"
#include "ble_gatts.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define BLE_ADM_USE_BASE_UUID                              \
    {                                                      \
        0x1D, 0x5D, 0x13, 0xEC, 0x0A, 0xEB, 0x42, 0xAB,    \
            0xAA, 0xE0, 0xD1, 0x66, 0x00, 0x00, 0xA6, 0x60 \
    }
#define BLE_ADM_USE_UUID_SERVICE 0xACDC
#define BLE_ADM_USE_ID_PWD_CHAR_UUID 0xACDD
#define BLE_ADM_USE_STATUS_CHAR_UUID 0xACDE

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Forward declaration of the ble_adm_use_t Admin User service definition structure type.
 */
typedef struct ble_adm_use_s ble_adm_use_t;

/**
 * Admin User service event types dispatched back to application-registered callback.
 */
typedef enum
{
    BLE_ADM_CONNECTED = 0, /* Connection session with peer established             */
    BLE_ADM_DISCONNECTED,  /* Disconnected from an established connection session  */
    BLE_ADM_ID_PWD_RX,     /* Received data from peer on the Id/Pwd characteristic */
    BLE_ADM_NOTIF_ENABLED, /* Peer enabled notifications on Status characteristic  */
    BLE_ADM_NOTIF_DISABLED, /* Peer disabled notifications on Status characteristic */
    BLE_ADM_CMD_RX             /* Received data from peer on the command characteristic */

} BleAdm_tenuEventType;

/**
 * Rx data structure upon being on the receiving end of a GATT client write event.
 */
typedef struct
{
    uint8_t const *pu8Data; /* Pointer to Rx buffer    */
    uint16_t u16Length;     /* Length of received data */
} BleAdm_tstrRxData;

/**
 * Admin User service service's event structure.
 */
typedef struct
{
    BleAdm_tenuEventType enuEventType; /* Event type                                        */
    ble_adm_use_t *pstrAdmUseInstance; /* Pointer to the Admin User service instance */
    BleAdm_tstrRxData strRxData;       /* Received data upon a GATT client write event      */
} BleAdm_tstrEvent;

/**
 * BleUseRegEventHandler Admin User service's event handler function prototype.
 *
 * @note This prototype is used to define a BLE event handler for the Admin User service.
 *
 * @note Functions of this type take one parameter:
 *         - BleAdm_tstrEvent *pstrEvent: Pointer to received event structure.
 */
typedef void (*BleAdmUseEventHandler)(BleAdm_tstrEvent *pstrEvent);

/**
 * Admin User  service's initialization structure.
 */
typedef struct
{
    BleAdmUseEventHandler pfAdmUseEvtHandler; /* Event handler to be called when connection with peer
                                                 is established, notification is sent on the Status
                                                 characteristic or data is received on the Id/Password
                                                 characteristic */
} BleAdm_tstrInit;

/**
 * Admin User  service's definition structure.
 */
struct ble_adm_use_s
{
    uint8_t u8UuidType;                       /* Admin User  service's UUID type                              */
    uint16_t u16ServiceHandle;                /* Admin User service's handle as provided by the BLE stack    */
    ble_gatts_char_handles_t strIdPwdChar;    /* Id/Password characteristic handles                */
    ble_gatts_char_handles_t strStatusChar;   /* Status characteristic handles                     */
    uint16_t u16ConnHandle;                   /* Handle of the active connection session provided by Softdevice     */
    bool bNotificationEnabled;                /* Indicates whether peer has enabled notification of the Status char */
    BleAdmUseEventHandler pfAdmUseEvtHandler; /* Admin User service's event handler         */
};

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief vidBleUseRegEventHandler Ble event handler invoked upon receiving a Admin User
 *        service Ble event.
 *
 * @note The Admin User service's event handler declared in the definition structure is invoked
 *       from within the context of this handler.
 *
 * @param pstrAdmUseInstance Pointer to WiPad's Admin User service instance structure.
 * @param pstrEvent Pointer to received event structure.
 *
 * @return nothing.
 */
void vidBleAdmUseEventHandler(ble_adm_use_t *pstrAdmUseInstance, ble_evt_t const *pstrEvent);

/**
 * @brief enuBleUseRegTransferData Initiates data transfer to peer over BLE.
 *
 * @note  This function sends data as a notification to the Admin User
 *        service's Status characteristic
 *
 * @param pstrAdmUseInstance Pointer to the Admin User instance structure.
 * @param pu8Data Pointer to data buffer.
 * @param pu16DataLength Pointer to data length in bytes.
 *
 * @return Mid_tenuStatus Middleware_Success if transfer was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleAdmUseTransferData(ble_adm_use_t *pstrAdmUseInstance, uint8_t *pu8Data, uint16_t *pu16DataLength);

/**
 * @brief enuBleUseRegInit Initializes WiPad's Admin User BLE service.
 *
 * @param pstrAdmUseInstance Pointer to WiPad's Admin User service instance structure.
 * @param pstrUseRegInit Pointer to WiPad's Admin User service initialization structure.
 *
 * @return Mid_tenuStatus Middleware_Success if service initialization was performed successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuBleAdmUseInit(ble_adm_use_t *pstrAdmUseInstance, BleAdm_tstrInit const *pstrAdmUseInit);

#endif /* _BLE_ADM_H_ */