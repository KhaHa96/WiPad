/* --------------------------------- Twi abstraction layer ------------------------------------- */
/*  File      -  Header file for nRF52 Twi driver's abstraction layer                            */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/******************************************   INCLUDES   *****************************************/
#include "TwiMaster_al.h"
#include "nrf_drv_twi.h"
#include "nrfx_twi.h"

/**************************************   PRIVATE DEFINES   **************************************/
#define TWI_AL_MASTER_FREQ_COUNT 3U
#define TWI_AL_MASTER_INSTANCE_0 0
#define TWI_AL_MASTER_INSTANCE_1 1


#define NO_STOP_CONDITION_SET    1
#define NO_STOP_CONDITION_UNSET  0

/***************************************   PRIVATE MACROS   **************************************/

/* Twi configuration assertion macro */
#define TWI_AL_ASSERT_CONFIGURATION(PARAM)                         \
(                                                                  \
    (PARAM->u8TwiInstance < TWI_AL_MAX_MASTER_INSTANCE_COUNT)  &&  \
    (PARAM->u8InterruptPriority != NULL)                           \
)

/* Twi TX transfer assertion macro */
#define TWI_AL_ASSERT_TX_TRANSFER(PARAM)                            \
(                                                                   \
    ((PARAM->pu8TxBuffer != NULL) && (PARAM->u8TxBufferlength))&&   \
    (PARAM->pu8RxBuffer == NULL)                                     \
)

/* Twi RX transfer assertion macro */
#define TWI_AL_ASSERT_RX_TRANSFER(PARAM)                            \
(                                                                   \
    ((PARAM->pu8RxBuffer != NULL) && (PARAM->u8RxBufferlength)) &&  \
    (PARAM->pu8TxBuffer == NULL)                                     \
)

/**************************************   PRIVATE TYPES   ****************************************/
typedef void (*TwiMasterEventHandler)(nrf_drv_twi_evt_t const *pstrEvent, void *pvArg);

/************************************   PRIVATE VARIABLES   **************************************/
static TwiAlMaster_tstrHandle strTwiMasterHandleArray[TWI_AL_MAX_MASTER_INSTANCE_COUNT] = {{0}};
static nrf_drv_twi_t strTwiMasterInstanceArray[TWI_AL_MAX_MASTER_INSTANCE_COUNT] = {NRF_DRV_TWI_INSTANCE(TWI_AL_MASTER_INSTANCE_0),
                                                                                    NRF_DRV_TWI_INSTANCE(TWI_AL_MASTER_INSTANCE_1)};

static uint32_t u32MasterTwiFreqPool[TWI_AL_MASTER_FREQ_COUNT] = {100000,250000,400000};


/**************************************   PRIVATE FUNCTIONS   ************************************/
static nrf_drv_twi_frequency_t enuSetMasterFrequency(uint32_t u32Baudrate)
{
    /* Nrf's Twi Master supports 3 different frequency settings, the smallest of which being 100kbps
       Note: Refer to nRF52832 Product Specification v1.4 page 315 for more details */
    nrf_drv_twi_frequency_t enuRetValue = NRF_DRV_TWI_FREQ_100K;
    uint32_t u32MinDiff = MODULUS((int32_t)u32Baudrate - (int32_t)u32MasterTwiFreqPool[0]);

    /* Go through possible frequency values and find closest setting to baudrate */
    for(uint8_t u8Index = 1; u8Index < TWI_AL_MASTER_FREQ_COUNT; u8Index++)
    {
        if(MODULUS((int32_t)u32Baudrate - (int32_t)u32MasterTwiFreqPool[u8Index]) < u32MinDiff)
        {
            u32MinDiff = u32Baudrate - u32MasterTwiFreqPool[u8Index];
            enuRetValue = (nrf_drv_twi_frequency_t)u8Index;
        }
    }

    return enuRetValue;
}

static void vidTwiMasterEventHandler(nrf_drv_twi_evt_t const *pstrEvent, void *pvArg)
{
    /* Make sure Twi event structure pointer is not NULL */
    if(pstrEvent && pvArg)
    {
        uint8_t u8TwiInstance = *((uint8_t *)pvArg);

        /* Clear bTransferInProgress flag */
        strTwiMasterHandleArray[u8TwiInstance].bTransferInProgress = false;

        TwiAlCallback pfCallback = strTwiMasterHandleArray[u8TwiInstance].pfCallback;

        /* Application-level callback has to be non-NULL */
        if(pfCallback)
        {
            TwiAl_tenuOutcome enuXferStatus = (NRF_DRV_TWI_EVT_DONE == pstrEvent->type)
                                              ?TwiAl_Transfer_Success
                                              :TwiAl_Transfer_Failure;

            /* Call application-level callback */
            if((pstrEvent->xfer_desc.type== NRF_DRV_TWI_XFER_TX) && (pstrEvent->xfer_desc.p_primary_buf) && (!pstrEvent->xfer_desc.p_secondary_buf))
            {
                pfCallback(TwiAl_XFER_TX, enuXferStatus);
            }
            else if((pstrEvent->xfer_desc.type== NRF_DRV_TWI_XFER_RX) && (pstrEvent->xfer_desc.p_primary_buf) && (!pstrEvent->xfer_desc.p_secondary_buf))
            {
                pfCallback(TwiAl_XFER_RX, enuXferStatus);
            }
            else if ((pstrEvent->xfer_desc.type== NRF_DRV_TWI_XFER_TXRX) && (pstrEvent->xfer_desc.p_primary_buf) && (pstrEvent->xfer_desc.p_secondary_buf))
            {
                pfCallback(TwiAl_XFER_TXRX, enuXferStatus);
            }
            else
            {
                pfCallback(TwiAl_XFER_TXTX, enuXferStatus);
            }
        }
    }
}

/**************************************   PUBLIC FUNCTIONS   *************************************/
Drivers_tenuStatus enuTwiAlMaster_Init(TwiAl_tstrConfig *pstrConfig, TwiAlCallback pfCallback, uint32_t u32Baudrate)
{
    Drivers_tenuStatus enuRetVal = Driver_Failure;

    if(pstrConfig)
    {
        uint8_t u8TwiInstance = pstrConfig->u8TwiInstance;

        /* Make sure user passed a valid configuration */
        if((TWI_AL_ASSERT_CONFIGURATION(pstrConfig)) &&
           ((pfCallback != NULL) && (pstrConfig->u8TwiInstance < TWI_AL_MAX_MASTER_INSTANCE_COUNT)))
        {
            TwiAlMaster_tstrHandle *pstrTwiHandle = &strTwiMasterHandleArray[u8TwiInstance];

            /* Store user configuration in TwiAl's handle instance */
            pstrTwiHandle->pstrTwiConfig = malloc(sizeof(TwiAl_tstrConfig));
            pstrTwiHandle->pstrTwiConfig->u8TwiInstance = u8TwiInstance;
            pstrTwiHandle->pstrTwiConfig->u8InterruptPriority = pstrConfig->u8InterruptPriority;
            pstrTwiHandle->pstrTwiConfig->bClearBusInit = pstrConfig->bClearBusInit;
            pstrTwiHandle->pstrTwiConfig->bHoldBusUninit = pstrConfig->bHoldBusUninit;
            pstrTwiHandle->bIsInitialized = true;
            pstrTwiHandle->pfCallback = pfCallback;

            /* Populate nRF's Master Twi driver configuration structure */
            nrf_drv_twi_config_t strNrfTwiCfg = NRF_DRV_TWI_DEFAULT_CONFIG;
            strNrfTwiCfg.frequency = enuSetMasterFrequency(u32Baudrate);
            strNrfTwiCfg.interrupt_priority = pstrConfig->u8InterruptPriority;
            strNrfTwiCfg.clear_bus_init = pstrConfig->bClearBusInit;
            strNrfTwiCfg.hold_bus_uninit = pstrConfig->bHoldBusUninit;
            strNrfTwiCfg.scl = TWI_SCL_PIN;
            strNrfTwiCfg.sda = TWI_SDA_PIN;
            /* Map Twi event handler to a local placeholder */
            TwiMasterEventHandler pfEventHandler = (pfCallback != NULL)?vidTwiMasterEventHandler:NULL;

            /* Initialize Master Twi */
            enuRetVal = (NRF_SUCCESS == nrf_drv_twi_init(&strTwiMasterInstanceArray[u8TwiInstance],
                                                            &strNrfTwiCfg, pfEventHandler,
                                                            (void *)&pstrTwiHandle->pstrTwiConfig->u8TwiInstance))
                                                            ?Driver_Success:Driver_Failure;
        }
    }



    return enuRetVal;
}


void vidTwiAl_Uninit(uint8_t u8TwiInstance)
{
    /* Check for valid arguments */
    if(u8TwiInstance < TWI_AL_MAX_MASTER_INSTANCE_COUNT)
    {

        if(strTwiMasterHandleArray[u8TwiInstance].bIsInitialized)
        {
            if(bTwiAl_TransferInProgress(u8TwiInstance))
            {
                //nrf_drv_spi_abort(&strTwiMasterInstanceArray[u8TwiInstance]);
                strTwiMasterHandleArray[u8TwiInstance].bTransferInProgress = false;
            }

            /* Uninitialize TwiAl Master instance */
            nrf_drv_twi_uninit(&strTwiMasterInstanceArray[u8TwiInstance]);

            /* Reset TwiAl Master handle */
            free(strTwiMasterHandleArray[u8TwiInstance].pstrTwiConfig);
            strTwiMasterHandleArray[u8TwiInstance].pfCallback = NULL;
            strTwiMasterHandleArray[u8TwiInstance].bIsInitialized = false;
        }
    }
}

bool bTwiAl_TransferInProgress(uint8_t u8TwiInstance)
{
    bool bRetVal = false;

    /* Check for valid arguments */
    if(u8TwiInstance < TWI_AL_MAX_MASTER_INSTANCE_COUNT)
    {
        bRetVal = strTwiMasterHandleArray[u8TwiInstance].bIsInitialized
                    ?strTwiMasterHandleArray[u8TwiInstance].bTransferInProgress
                    :false;

    }

    return bRetVal;
}

Drivers_tenuStatus enuTwiAl_Transfer(uint8_t u8TwiInstance, TwiAl_tstrXfer *pstrXfer)
{
    Drivers_tenuStatus enuRetVal = Driver_Failure;

    /* Check for valid arguments */
    if((u8TwiInstance < TWI_AL_MAX_MASTER_INSTANCE_COUNT) &&
       ((pstrXfer) && (strTwiMasterHandleArray[u8TwiInstance].bIsInitialized)))
    {
        if(TWI_AL_ASSERT_TX_TRANSFER(pstrXfer))
        {
            /* Initiate TwiAl Master TX transfer */
            enuRetVal = (NRF_SUCCESS == nrf_drv_twi_tx(&strTwiMasterInstanceArray[u8TwiInstance],
                                                            pstrXfer->u8Address,
                                                            pstrXfer->pu8TxBuffer,
                                                            pstrXfer->u8TxBufferlength,
                                                            NO_STOP_CONDITION_UNSET))
                                                            ?Driver_Success:Driver_Failure;

            /* Set ongoing transfer flag if transfer is being performed in non-blocking mode */
            if(strTwiMasterHandleArray[u8TwiInstance].pfCallback)
            {
                strTwiMasterHandleArray[u8TwiInstance].bTransferInProgress = true;
            }
        }
        if(TWI_AL_ASSERT_RX_TRANSFER(pstrXfer))
        {
            /* Initiate TwiAl Master TX transfer */
            enuRetVal = (NRF_SUCCESS == nrf_drv_twi_rx(&strTwiMasterInstanceArray[u8TwiInstance],
                                                            pstrXfer->u8Address,
                                                            pstrXfer->pu8RxBuffer,
                                                            pstrXfer->u8RxBufferlength))
                                                            ?Driver_Success:Driver_Failure;

            /* Set ongoing transfer flag if transfer is being performed in non-blocking mode */
            if(strTwiMasterHandleArray[u8TwiInstance].pfCallback)
            {
                strTwiMasterHandleArray[u8TwiInstance].bTransferInProgress = true;
            }
        }
    }

    return enuRetVal;
}