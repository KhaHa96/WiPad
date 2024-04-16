/* --------------------------------- Twi Master abstraction layer ------------------------------------- */
/*  File      -  Header file for nRF52 Twi Master driver's abstraction layer                            */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/******************************************   INCLUDES   *****************************************/
#include "system_config.h"
#include "driver_utils.h"

/****************************************   PUBLIC TYPES   ***************************************/
/**
 * TwiAl_tenuEvent Enumeration of the different events that could lead to a Twi Master transfer
 *                 callback invoke in non-blocking mode
*/
typedef enum
{
    TwiAl_XFER_RX = 0, /* Half-duplex Twi read       */
    TwiAl_XFER_TX,     /* Half-duplex Twi write      */
    TwiAl_XFER_TXRX,   /* Full-duplex Twi read/write : TX transfer followed by RX transfer with repeated start. */
    TwiAl_XFER_TXTX    /* Half-duplex Twi write : TX transfer followed by TX transfer with repeated start. */
}TwiAl_tenuEvent;

/**
 * TwiAl_tenuOutcome Enumeration of the different possible outcomes of an TwiAl transfer
*/
typedef enum
{
    TwiAl_Transfer_Success = 0, /* Transfer performed successfully */
    TwiAl_Transfer_Failure      /* Transfer failed                 */
}TwiAl_tenuOutcome;


/**
 * TwiAlCallback End of transfer callback prototype
 *
 * @note This callback function is invoked upon receiving an end of Twi transfer
 *       notification from the nrf_drv_twi event handler. This has 2 important
 *       take-aways:
 *                  - This callback is a mere application-level wrapper for nrf's Twi IRQ
 *                  - This callback is completely overlooked when using Twi_al in Blocking mode
 *
 * @note This callback takes four parameters:
 *  - TwiAl_tenuEvent enuTwiEvent: Twi event that triggered this callback
 *  - TwiAl_tenuRole enuTwiRole: Twi role used for performing transfer
 *  - TwiAl_tenuOutcome enuOutcome: Twi transfer outcome
 */
typedef void (*TwiAlCallback)(TwiAl_tenuEvent enuTwiEvent,
                              TwiAl_tenuOutcome enuOutcome);

/**
 * TwiAl_tstrXfer TWI transfer descriptor Structure
 *
 *  @note To succesfully make a TX transfer, the Pointer to TX transfer buffer pu8TxBuffer should not
 *        be passed as a null value, the TX buffer length u8TxBufferlength should be different from 0 and the
 *        Pointer to RX transfer buffer pu8RxBuffer should be passed as a NULL value.
 *
 * @note To succesfully make a RX transfer, the Pointer to RX transfer buffer pu8RxBuffer should not
 *        be passed as a null value, the RX buffer length u8RxBufferlength should be different from 0 and the
 *        Pointer to TX transfer buffer pu8TxBuffer should be passed as a NULL value.
*/
typedef struct
{
    uint8_t                 u8Address;           ///< Slave address.
    uint8_t                 u8TxBufferlength;     ///< TX Transfer buffer length.
    uint8_t const *               pu8TxBuffer;       ///< Pointer to TX transfer buffer
    uint8_t                 u8RxBufferlength;     ///< RX Transfer buffer length.
    uint8_t *               pu8RxBuffer;       ///< Pointer to RX transfer buffer
}TwiAl_tstrXfer;

/**
 * TwiAl_tstrConfig Twi instance configuration Structure.
 */
typedef struct
{
    uint8_t                    u8InterruptPriority;  ///< Interrupt priority.
    bool                       bClearBusInit;      ///< Clear bus during init.
    bool                       bHoldBusUninit;     ///< Hold pull up state on gpio pins after uninit.
    uint8_t                    u8TwiInstance;      /* Twi instance */
} TwiAl_tstrConfig;

/**
 * TwiAlMaster_tstrHandle Twi master handle structure
*/
typedef struct
{
    bool bIsInitialized;           /* TWI instance initialization state indicator          */
    bool bTransferInProgress;      /* Flag indicating whether an TWI trasfer is ongoing    */
    TwiAl_tstrConfig *pstrTwiConfig; /* Twi instance configuration                           */
    TwiAlCallback pfCallback;        /* Application-level non-blocking TWI transfer callback */
}TwiAlMaster_tstrHandle;

/***************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuTwiAl_Init Initializes an TwI Master instance with a user
 *        configuration and sets an application-level end-of-transfer callback.
 *
 * @note Passing a NULL callback function pointer to enuTwiAl_Init will automatically
 *       configure TwiAl's instance to perform transfers in blocking mode.
 *
 * @note TwiAl instances configured as Master use nrf_drv_twi driver which does not rely on EDMA.
 *
 * @pre This function requires no prerequisites.
 *
 * @warning This function should be called before calling any TwiAl API and it should be called once for
 *          every Twi Master instance.
 *
 * @param pstrConfig Pointer to user configuration structure
 * @param pfCallback Pointer to application-level end-of-transfer callback
 *
 * @return Drivers_tenuStatus Driver_Success if initialization was performed successfully,
 *         Driver_Failure otherwise
 */
Drivers_tenuStatus enuTwiAlMaster_Init(TwiAl_tstrConfig *pstrConfig, TwiAlCallback pfCallback, uint32_t u32Baudrate);

/**
 * @brief vidTwiAl_Uninit Uninitializes an TWI instance when. If an TWI transfer is ongoing, the operation
 * is first cancelled then the TwiAl instance is uninitialized
 *
 * @pre Uninitialization of an TwiAl instance is only possible if it's already initialized
 *
 * @warning Blocking transfers cannot be aborted
 *
 * @param u8TwiInstance TwiAl instance
 *
 * @return nothing
 */
void vidTwiAl_Uninit(uint8_t u8TwiInstance);

/**
 * @brief bTwiAl_TransferInProgress Returns transfer status of the provided instance.
 *
 * @pre The TwiAl instance identified by its index must have already been initialized
 *      using enuTwiAl_Init by the time this function is invoked.
 *
 * @warning This function shouldn't be used for blocking Twi transfers
 *
 * @param u8TwiInstance TwiiAl instance
 *
 * @return bool true if transfer is ongoing, false otherwise
 */
bool bTwiAl_TransferInProgress(uint8_t u8TwiInstance);

/**
 * @brief enuTwiAl_Transfer Performs an TwiAl half-duplex/full-duplex data transfer.
 *
 * @pre The TwiAl instance identified by its index must have already been initialized
 *      using enuTwiAl_Init by the time this function is invoked.
 *
 * @warning Passing NULL Tx and Rx buffer pointers will yield a failed transfer. Similarly, a
 *          non NULL buffer pointer must be provided alongside a non-zero buffer length.
 *
 * @param u8TwiInstance TwiAl instance
 * @param pstrXfer Pointer to TwiAl transfer structure
 *
 * @return Drivers_tenuStatus Driver_Success if TwiAl transfer was performed successfully,
 *         Driver_Failure otherwise
 */
Drivers_tenuStatus enuTwiAl_Transfer(uint8_t u8TwiInstance, TwiAl_tstrXfer *pstrXfer);