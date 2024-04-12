/* --------------------------------- Spi abstraction layer ------------------------------------- */
/*  File      -  Header file for nRF52 Spi driver's abstraction layer                            */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef SPI_AL_H
#define SPI_AL_H

/******************************************   INCLUDES   *****************************************/
#include "system_config.h"
#include "driver_utils.h"

/****************************************   PUBLIC TYPES   ***************************************/
/**
 * SpiAl_tenuRole Enumeration of SpiAl's different roles
*/
typedef enum
{
    SpiAl_Master = 0, /* Spi instance used as Master */
    SpiAl_Slave       /* Spi instance used as Slave  */
}SpiAl_tenuRole;

/**
 * SpiAl_tenuBitOrder Enumeration of the different Spi transfer bit order configurations
*/
typedef enum
{
    SpiAl_MsbFirst = 0,  /* Data transfer starts with Most significant bit  */
    SpiAl_LsbFirst       /* Data transfer starts with least significant bit */
}SpiAl_tenuBitOrder;

/**
 * SpiAl_tenuClockSamplePhase Enumeration of the different Spi data sampling clock
 *                            phase configurations
*/
typedef enum
{
    SpiAl_First_Edge = 0, /* Data sampled on clock's leading edge  */
    SpiAl_Second_Edge     /* Data sampled on clock's trailing edge */
}SpiAl_tenuClockSamplePhase;

/**
 * SpiAl_tenuClockPolarity Enumeration of the different Spi clock polarity configurations
*/
typedef enum
{
    SpiAl_Clk_Active_High = 0, /* Spi clock line is active high */
    SpiAl_Clk_Active_Low       /* Spi clock line is active low  */
}SpiAl_tenuClockPolarity;

/**
 * SpiAl_tenuEvent Enumeration of the different events that could lead to an Spi transfer
 *                 callback invoke in non-blocking mode
*/
typedef enum
{
    SpiAl_Rx = 0, /* Half-duplex Spi read       */
    SpiAl_Tx,     /* Half-duplex Spi write      */
    SpiAl_Tx_Rx   /* Full-duplex Spi read/write */
}SpiAl_tenuEvent;

/**
 * SpiAl_tenuOutcome Enumeration of the different possible outcomes of an SpiAl transfer
*/
typedef enum
{
    SpiAl_Transfer_Success = 0, /* Transfer performed successfully */
    SpiAl_Transfer_Failure      /* Transfer failed                 */
}SpiAl_tenuOutcome;

/**
 * SpiAlCallback End of transfer callback prototype
 *
 * @note This callback function is invoked upon receiving an end of Spi transfer
 *       notification from the nrf_drv_spi event handler. This has 2 important
 *       take-aways:
 *                  - This callback is a mere application-level wrapper for nrf's Spi IRQ
 *                  - This callback is completely overlooked when using Spi_al in Blocking mode
 *
 * @note This callback takes four parameters:
 *  - SpiAl_tenuEvent enuSpiEvent: Spi event that triggered this callback
 *  - SpiAl_tenuRole enuSpiRole: Spi role used for performing transfer
 *  - SpiAl_tenuOutcome enuOutcome: Spi transfer outcome
 */
typedef void (*SpiAlCallback)(SpiAl_tenuEvent enuSpiEvent,
                              SpiAl_tenuRole enuSpiRole,
                              SpiAl_tenuOutcome enuOutcome);

/**
 * SpiAl_tstrXfer Spi transfer structure
*/
typedef struct
{
    uint8_t *pu8RxBuffer;     /* Pointer to Rx buffer */
    uint8_t u8RxBufferLength; /* Rx buffer length     */
    uint8_t *pu8TxBuffer;     /* Pointer to Tx buffer */
    uint8_t u8TxBufferLength; /* Tx buffer length     */
}SpiAl_tstrXfer;

/**
 * SpiAl_tstrConfig Spi instance configuration structure
*/
typedef struct
{
    SpiAl_tenuRole enuSpiRole;              /* Spi role (Master/Slave)                    */
    SpiAl_tenuBitOrder enuBitOrder;         /* Msb/Lsb mounted first on Spi transfer line */
    SpiAl_tenuClockSamplePhase enuClkPhase; /* Spi data sampling clock phase              */
    SpiAl_tenuClockPolarity enuClkPolarity; /* Spi clock line polarity                    */
    uint32_t u32Baudrate;                   /* Spi datarate in Mbps                       */
    uint8_t u8SpiInstance;                  /* Spi instance                               */
}SpiAl_tstrConfig;

/**
 * SpiAl_tstrHandle Spi handle structure
*/
typedef struct
{
    bool bIsInitialized;           /* SPI instance initialization state indicator          */
    bool bTransferInProgress;      /* Flag indicating whether an SPI trasfer is ongoing    */
    SpiAl_tstrConfig *pstrSpiConfig; /* Spi instance configuration                           */
    SpiAlCallback pfCallback;        /* Application-level non-blocking Spi transfer callback */
}SpiAl_tstrHandle;

/***************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuSpiAl_Init Initializes an SPI instance with a user
 *        configuration and sets an application-level end-of-transfer callback.
 *
 * @note Passing a NULL callback function pointer to enuSpiAl_Init will automatically
 *       configure SpiAl's instance to perform transfers in blocking mode. This is only
 *       applicable to Master SpiAl instances.
 *
 * @note SpiAl instances configured as Master use nrf_drv_spi driver which does not rely on EDMA. SpiAl
 *       instances configured as Slave however, use the nrf_drv_spis driver which does make use of EDMA.
 *
 * @pre This function requires no prerequisites.
 *
 * @warning This function should be called before calling any SpiAl API and it should be called once for
 *          every Spi instance.
 *
 * @param pstrConfig Pointer to user configuration structure
 * @param pfCallback Pointer to application-level end-of-transfer callback
 *
 * @return Drivers_tenuStatus Driver_Success if initialization was performed successfully,
 *         Driver_Failure otherwise
 */
Drivers_tenuStatus enuSpiAl_Init(SpiAl_tstrConfig *pstrConfig, SpiAlCallback pfCallback);

/**
 * @brief vidSpiAl_Uninit Uninitializes an SPI instance when. If an Spi transfer is ongoing, the operation
 * is first cancelled then the SpiAl instance is uninitialized
 *
 * @pre Uninitialization of an SpiAl instance is only possible if it's already initialized
 *
 * @warning Blocking transfers cannot be aborted
 *
 * @param enuRole SpiAl instance role
 * @param u8SpiInstance SpiAl instance
 *
 * @return nothing
 */
void vidSpiAl_Uninit(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance);

/**
 * @brief bSpiAl_TransferInProgress Returns transfer status of the provided instance.
 *
 * @pre The SpiAl instance identified by its role and index must have already been initialized
 *      using enuSpiAl_Init by the time this function is invoked.
 *
 * @warning This function shouldn't be used for blocking Spi transfers
 *
 * @param enuRole SpiAl instance role
 * @param u8SpiInstance SpiAl instance
 *
 * @return bool true if transfer is ongoing, false otherwise
 */
bool bSpiAl_TransferInProgress(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance);

/**
 * @brief enuSpiAl_Transfer Performs an SpiAl half-duplex/full-duplex data transfer.
 *
 * @pre The SpiAl instance identified by its role and index must have already been initialized
 *      using enuSpiAl_Init by the time this function is invoked.
 *
 * @warning Passing NULL Tx and Rx buffer pointers will yield a failed transfer. Similarly, a
 *          non NULL buffer pointer must be provided alongside a non-zero buffer length.
 *
 * @param enuRole SpiAl instance role
 * @param u8SpiInstance SpiAl instance
 * @param pstrXfer Pointer to SpiAl transfer structure
 *
 * @return Drivers_tenuStatus Driver_Success if SpiAl transfer was performed successfully,
 *         Driver_Failure otherwise
 */
Drivers_tenuStatus enuSpiAl_Transfer(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance, SpiAl_tstrXfer *pstrXfer);

#endif /* SPI_AL_H */