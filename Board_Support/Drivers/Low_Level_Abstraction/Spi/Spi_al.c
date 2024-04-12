/* --------------------------------- Spi abstraction layer ------------------------------------- */
/*  File      -  Source file for nRF52 Spi driver's abstraction layer                            */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/******************************************   INCLUDES   *****************************************/
#include "Spi_al.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_spis.h"

/**************************************   PRIVATE DEFINES   **************************************/
#define SPI_AL_MASTER_FREQ_COUNT 7U
#define SPI_AL_MASTER_INSTANCE_0 0
#define SPI_AL_MASTER_INSTANCE_1 1
#define SPI_AL_MASTER_INSTANCE_2 2
#define SPI_AL_SLAVE_INSTANCE_0  0
#define SPI_AL_SLAVE_INSTANCE_1  1
#define SPI_AL_SLAVE_INSTANCE_2  2

/***************************************   PRIVATE MACROS   **************************************/
/* Spi role assertion macro */
#define SPI_AL_ASSERT_ROLE(PARAM)  \
(                                  \
    ( (PARAM) == SpiAl_Master ) || \
    ( (PARAM) == SpiAl_Slave  )    \
)

/* Spi bit order assertion macro */
#define SPI_AL_ASSERT_BIT_ORDER(PARAM) \
(                                      \
    ( (PARAM) == SpiAl_MsbFirst ) ||   \
    ( (PARAM) == SpiAl_LsbFirst )      \
)

/* Spi clock phase assertion macro */
#define SPI_AL_ASSERT_CLOCK_PHASE(PARAM) \
(                                        \
    ( (PARAM) == SpiAl_First_Edge  ) ||  \
    ( (PARAM) == SpiAl_Second_Edge )     \
)

/* Spi clock polarity assertion macro */
#define SPI_AL_ASSERT_CLOCK_POLARITY(PARAM) \
(                                           \
    ( (PARAM) == SpiAl_Clk_Active_High ) || \
    ( (PARAM) == SpiAl_Clk_Active_Low  )    \
)

/* Spi configuration assertion macro */
#define SPI_AL_ASSERT_CONFIGURATION(PARAM)              \
(                                                       \
    SPI_AL_ASSERT_ROLE(PARAM->enuSpiRole)           &&  \
    SPI_AL_ASSERT_BIT_ORDER(PARAM->enuBitOrder)     &&  \
    SPI_AL_ASSERT_CLOCK_PHASE(PARAM->enuClkPhase)   &&  \
    SPI_AL_ASSERT_CLOCK_POLARITY(PARAM->enuClkPolarity) \
)

/* Spi transfer assertion macro */
#define SPI_AL_ASSERT_TRANSFER(PARAM)                              \
(                                                                  \
    ((PARAM->pu8RxBuffer != NULL) && (PARAM->u8RxBufferLength)) || \
    ((PARAM->pu8TxBuffer != NULL) && (PARAM->u8TxBufferLength))    \
)

/**************************************   PRIVATE TYPES   ****************************************/
typedef void (*SpiMasterEventHandler)(nrf_drv_spi_evt_t const *pstrEvent, void *pvArg);

/************************************   PRIVATE VARIABLES   **************************************/
static SpiAl_tstrHandle strSpiMasterHandleArray[SPI_AL_MAX_MASTER_INSTANCE_COUNT] = {{0}};
static SpiAl_tstrHandle strSpiSlaveHandleArray[SPI_AL_MAX_SLAVE_INSTANCE_COUNT] = {{0}};
static nrf_drv_spi_t strSpiMasterInstanceArray[SPI_AL_MAX_MASTER_INSTANCE_COUNT] = {NRF_DRV_SPI_INSTANCE(SPI_AL_MASTER_INSTANCE_0),
                                                                                    NRF_DRV_SPI_INSTANCE(SPI_AL_MASTER_INSTANCE_1),
                                                                                    NRF_DRV_SPI_INSTANCE(SPI_AL_MASTER_INSTANCE_2)};

static nrf_drv_spis_t strSpiSlaveInstanceArray[SPI_AL_MAX_SLAVE_INSTANCE_COUNT] = {NRF_DRV_SPIS_INSTANCE(SPI_AL_SLAVE_INSTANCE_0),
                                                                                   NRF_DRV_SPIS_INSTANCE(SPI_AL_SLAVE_INSTANCE_1),
                                                                                   NRF_DRV_SPIS_INSTANCE(SPI_AL_SLAVE_INSTANCE_2)};

static uint32_t u32MasterSpiFreqPool[SPI_AL_MASTER_FREQ_COUNT] = {125000, 250000, 500000,
                                                                  1000000, 2000000, 4000000, 8000000};

/**************************************   PRIVATE FUNCTIONS   ************************************/
static nrf_drv_spi_frequency_t enuSetMasterFrequency(uint32_t u32Baudrate)
{
    /* Nrf's Spi Master supports 7 different frequency settings, the smallest of which being 125kbps
       Note: Refer to nRF52832 Product Specification v1.4 page 288 for more details */
    nrf_drv_spi_frequency_t enuRetValue = NRF_DRV_SPI_FREQ_125K;
    uint32_t u32MinDiff = MODULUS((int32_t)u32Baudrate - (int32_t)u32MasterSpiFreqPool[0]);

    /* Go through possible frequency values and find closest setting to baudrate */
    for(uint8_t u8Index = 1; u8Index < SPI_AL_MASTER_FREQ_COUNT; u8Index++)
    {
        if(MODULUS((int32_t)u32Baudrate - (int32_t)u32MasterSpiFreqPool[u8Index]) < u32MinDiff)
        {
            u32MinDiff = u32Baudrate - u32MasterSpiFreqPool[u8Index];
            enuRetValue = (nrf_drv_spi_frequency_t)u8Index;
        }
    }

    return enuRetValue;
}

static void vidMapClkMode(void *pvSpiCfg, SpiAl_tenuRole enuSpiRole, SpiAl_tenuClockSamplePhase enuClkPhase, SpiAl_tenuClockPolarity enuClkPolarity)
{
    /* Ensure pointer to configuration structure is not NULL */
    if(pvSpiCfg != NULL)
    {
        if(SpiAl_Master == enuSpiRole)
        {
            nrf_drv_spi_config_t *pstrMasterCfg = (nrf_drv_spi_config_t *)pvSpiCfg;

            switch(enuClkPhase)
            {
            case SpiAl_First_Edge:
            {
                pstrMasterCfg->mode = (SpiAl_Clk_Active_High == enuClkPolarity)?NRF_DRV_SPI_MODE_0:NRF_DRV_SPI_MODE_2;
            }
            break;

            case SpiAl_Second_Edge:
            {
                pstrMasterCfg->mode = (SpiAl_Clk_Active_High == enuClkPolarity)?NRF_DRV_SPI_MODE_1:NRF_DRV_SPI_MODE_3;
            }
            break;

            default:
                /* Nothing to do */
                break;
            }
        }
        else
        {
            nrf_drv_spis_config_t *pstrSlaveCfg = (nrf_drv_spis_config_t *)pvSpiCfg;

            switch(enuClkPhase)
            {
            case SpiAl_First_Edge:
            {
                pstrSlaveCfg->mode = (SpiAl_Clk_Active_High == enuClkPolarity)?NRF_SPIS_MODE_0:NRF_SPIS_MODE_2;
            }
            break;

            case SpiAl_Second_Edge:
            {
                pstrSlaveCfg->mode = (SpiAl_Clk_Active_High == enuClkPolarity)?NRF_SPIS_MODE_1:NRF_SPIS_MODE_3;
            }
            break;

            default:
                /* Nothing to do */
                break;
            }
        }
    }
}

static uint8_t u8FindSlaveInstance(void)
{
    uint8_t u8Indx = 0;

    while((u8Indx < SPI_AL_MAX_SLAVE_INSTANCE_COUNT) && (!strSpiSlaveHandleArray[u8Indx].bTransferInProgress))
    {
        u8Indx++;
    }

    return u8Indx;
}

static void vidSpiMasterEventHandler(nrf_drv_spi_evt_t const *pstrEvent, void *pvArg)
{
    /* Make sure Spi event structure pointer is not NULL */
    if(pstrEvent && pvArg)
    {
        uint8_t u8SpiInstance = *((uint8_t *)pvArg);

        /* Clear bTransferInProgress flag */
        strSpiMasterHandleArray[u8SpiInstance].bTransferInProgress = false;

        SpiAlCallback pfCallback = strSpiMasterHandleArray[u8SpiInstance].pfCallback;

        /* Application-level callback has to be non-NULL */
        if(pfCallback)
        {
            SpiAl_tenuOutcome enuXferStatus = (NRF_DRV_SPI_EVENT_DONE == pstrEvent->type)
                                              ?SpiAl_Transfer_Success
                                              :SpiAl_Transfer_Failure;

            /* Call application-level callback */
            if((pstrEvent->data.done.p_tx_buffer) && (!pstrEvent->data.done.p_rx_buffer))
            {
                pfCallback(SpiAl_Tx, SpiAl_Master, enuXferStatus);
            }
            else if((!pstrEvent->data.done.p_tx_buffer) && (pstrEvent->data.done.p_rx_buffer))
            {
                pfCallback(SpiAl_Rx, SpiAl_Master, enuXferStatus);
            }
            else
            {
                pfCallback(SpiAl_Tx_Rx, SpiAl_Master, enuXferStatus);
            }
        }
    }
}

static void vidSpiSlaveEventHandler(nrf_drv_spis_event_t strEvent)
{
    /* Find Slave Spi instance responsible for triggering this event handler. Note: This
       implementation supposes that no two Slave Spi transfers can be in progress simultaneously */
    uint8_t u8SpiInstance = u8FindSlaveInstance();

    if(u8SpiInstance < SPI_AL_MAX_SLAVE_INSTANCE_COUNT)
    {
        if(NRFX_SPIS_BUFFERS_SET_DONE != strEvent.evt_type)
        {
            /* Clear bTransferInProgress flag */
            strSpiSlaveHandleArray[u8SpiInstance].bTransferInProgress = false;

            SpiAlCallback pfCallback = strSpiSlaveHandleArray[u8SpiInstance].pfCallback;

            /* Application-level callback has to be non-NULL */
            if(pfCallback)
            {
                SpiAl_tenuOutcome enuXferStatus = (NRF_DRV_SPIS_XFER_DONE == strEvent.evt_type)
                                                  ?SpiAl_Transfer_Success
                                                  :SpiAl_Transfer_Failure;

                /* Call application-level callback */
                if((strEvent.tx_amount) && (!strEvent.rx_amount))
                {
                    pfCallback(SpiAl_Tx, SpiAl_Slave, enuXferStatus);
                }
                else if((!strEvent.tx_amount) && (strEvent.rx_amount))
                {
                    pfCallback(SpiAl_Rx, SpiAl_Slave, enuXferStatus);
                }
                else
                {
                    pfCallback(SpiAl_Tx_Rx, SpiAl_Slave, enuXferStatus);
                }
            }
        }
    }
}

/**************************************   PUBLIC FUNCTIONS   *************************************/
Drivers_tenuStatus enuSpiAl_Init(SpiAl_tstrConfig *pstrConfig, SpiAlCallback pfCallback)
{
    Drivers_tenuStatus enuRetVal = Driver_Failure;

    if(pstrConfig)
    {
        uint8_t u8SpiInstance = pstrConfig->u8SpiInstance;
        bool bInitializedInst = (SpiAl_Master == pstrConfig->enuSpiRole)
                                  ?strSpiMasterHandleArray[u8SpiInstance].bIsInitialized
                                  :strSpiSlaveHandleArray[u8SpiInstance].bIsInitialized;

        if(!bInitializedInst)
        {
            /* Make sure user passed a valid configuration */
            if((SPI_AL_ASSERT_CONFIGURATION(pstrConfig)) &&
               (((SpiAl_Master == pstrConfig->enuSpiRole) &&
                 (pstrConfig->u8SpiInstance < SPI_AL_MAX_MASTER_INSTANCE_COUNT)) ||
                ((SpiAl_Slave == pstrConfig->enuSpiRole) &&
                 ((pfCallback != NULL) && (pstrConfig->u8SpiInstance < SPI_AL_MAX_SLAVE_INSTANCE_COUNT)))))
            {
                SpiAl_tstrHandle *pstrSpiHandle = (SpiAl_Master == pstrConfig->enuSpiRole)
                                                  ?&strSpiMasterHandleArray[u8SpiInstance]
                                                  :&strSpiSlaveHandleArray[u8SpiInstance];

                /* Store user configuration in SpiAl's handle instance */
                pstrSpiHandle->pstrSpiConfig = malloc(sizeof(SpiAl_tstrConfig));
                pstrSpiHandle->pstrSpiConfig->enuSpiRole = pstrConfig->enuSpiRole;
                pstrSpiHandle->pstrSpiConfig->enuBitOrder = pstrConfig->enuBitOrder;
                pstrSpiHandle->pstrSpiConfig->enuClkPhase = pstrConfig->enuClkPhase;
                pstrSpiHandle->pstrSpiConfig->enuClkPolarity = pstrConfig->enuClkPolarity;
                pstrSpiHandle->pstrSpiConfig->u32Baudrate = pstrConfig->u32Baudrate;
                pstrSpiHandle->pstrSpiConfig->u8SpiInstance = u8SpiInstance;

                /* Branch off depending on Spi device's chosen role */
                if(SpiAl_Master == pstrConfig->enuSpiRole)
                {
                    /* Populate nRF's Master Spi driver configuration structure */
                    nrf_drv_spi_config_t strNrfSpiCfg = NRF_DRV_SPI_DEFAULT_CONFIG;
                    strNrfSpiCfg.frequency = enuSetMasterFrequency(pstrConfig->u32Baudrate);
                    strNrfSpiCfg.bit_order = (nrf_drv_spi_bit_order_t)pstrConfig->enuBitOrder;
                    vidMapClkMode((void *)&strNrfSpiCfg, SpiAl_Master, pstrConfig->enuClkPhase, pstrConfig->enuClkPolarity);
                    strNrfSpiCfg.ss_pin = SPI_CS_PIN;
                    strNrfSpiCfg.miso_pin = SPI_MISO_PIN;
                    strNrfSpiCfg.mosi_pin = SPI_MOSI_PIN;
                    strNrfSpiCfg.sck_pin = SPI_CLOCK_PIN;

                    /* Map Spi event handler to a local placeholder */
                    SpiMasterEventHandler pfEventHandler = (pfCallback != NULL)?vidSpiMasterEventHandler:NULL;

                    /* Initialize Master Spi */
                    enuRetVal = (NRF_SUCCESS == nrf_drv_spi_init(&strSpiMasterInstanceArray[u8SpiInstance],
                                                                 &strNrfSpiCfg, pfEventHandler,
                                                                 (void *)&pstrSpiHandle->pstrSpiConfig->u8SpiInstance))
                                                                 ?Driver_Success:Driver_Failure;
                }
                else
                {
                    /* Populate nRF's Slave Spi driver configuration structure */
                    nrf_drv_spis_config_t strNrfSpisCfg = NRF_DRV_SPIS_DEFAULT_CONFIG;
                    strNrfSpisCfg.bit_order = (nrf_spis_bit_order_t)pstrConfig->enuBitOrder;
                    vidMapClkMode((void *)&strNrfSpisCfg, SpiAl_Slave, pstrConfig->enuClkPhase, pstrConfig->enuClkPolarity);
                    strNrfSpisCfg.miso_pin = SPI_MISO_PIN;
                    strNrfSpisCfg.mosi_pin = SPI_MOSI_PIN;
                    strNrfSpisCfg.sck_pin = SPI_CLOCK_PIN;
                    strNrfSpisCfg.csn_pin = SPI_CS_PIN;

                    /* Initialize Slave Spi */
                    enuRetVal = (NRF_SUCCESS == nrf_drv_spis_init(&strSpiSlaveInstanceArray[u8SpiInstance],
                                                                  &strNrfSpisCfg,
                                                                  vidSpiSlaveEventHandler))
                                                                  ?Driver_Success:Driver_Failure;
                }
            }
        }
    }

    return enuRetVal;
}

void vidSpiAl_Uninit(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance)
{
    /* Check for valid arguments */
    if((u8SpiInstance < SPI_AL_MAX_MASTER_INSTANCE_COUNT) && (SPI_AL_ASSERT_ROLE(enuRole)))
    {
        /* Abort any ongoing Master Spi transfers */
        if(SpiAl_Master == enuRole)
        {
            if(strSpiMasterHandleArray[u8SpiInstance].bIsInitialized)
            {
                if(bSpiAl_TransferInProgress(enuRole, u8SpiInstance))
                {
                    nrf_drv_spi_abort(&strSpiMasterInstanceArray[u8SpiInstance]);
                    strSpiMasterHandleArray[u8SpiInstance].bTransferInProgress = false;
                }

                /* Uninitialize SpiAl Master instance */
                nrf_drv_spi_uninit(&strSpiMasterInstanceArray[u8SpiInstance]);

                /* Reset SpiAl Master handle */
                free(strSpiMasterHandleArray[u8SpiInstance].pstrSpiConfig);
                strSpiMasterHandleArray[u8SpiInstance].pfCallback = NULL;
                strSpiMasterHandleArray[u8SpiInstance].bIsInitialized = false;
            }
        }
        else
        {
            if(strSpiSlaveHandleArray[u8SpiInstance].bIsInitialized)
            {
                /* Uninitialize SpiAl Slave instance */
                nrf_drv_spis_uninit(&strSpiSlaveInstanceArray[u8SpiInstance]);

                /* Reset SpiAl Slave handle */
                free(strSpiSlaveHandleArray[u8SpiInstance].pstrSpiConfig);
                strSpiSlaveHandleArray[u8SpiInstance].pfCallback = NULL;
                strSpiSlaveHandleArray[u8SpiInstance].bIsInitialized = false;
            }
        }
    }
}

bool bSpiAl_TransferInProgress(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance)
{
    bool bRetVal = false;

    /* Check for valid arguments */
    if((u8SpiInstance < SPI_AL_MAX_MASTER_INSTANCE_COUNT) && (SPI_AL_ASSERT_ROLE(enuRole)))
    {
        if(SpiAl_Master == enuRole)
        {
            bRetVal = strSpiMasterHandleArray[u8SpiInstance].bIsInitialized
                      ?strSpiMasterHandleArray[u8SpiInstance].bTransferInProgress
                      :false;
        }
        else
        {
            bRetVal = strSpiSlaveHandleArray[u8SpiInstance].bIsInitialized
                      ?strSpiSlaveHandleArray[u8SpiInstance].bTransferInProgress
                      :false;
        }
    }

    return bRetVal;
}

Drivers_tenuStatus enuSpiAl_Transfer(SpiAl_tenuRole enuRole, uint8_t u8SpiInstance, SpiAl_tstrXfer *pstrXfer)
{
    Drivers_tenuStatus enuRetVal = Driver_Failure;

    /* Check for valid arguments */
    if((u8SpiInstance < SPI_AL_MAX_MASTER_INSTANCE_COUNT) &&
       (SPI_AL_ASSERT_ROLE(enuRole)) &&
       ((pstrXfer) && (SPI_AL_ASSERT_TRANSFER(pstrXfer))))
    {
        if(SpiAl_Master == enuRole)
        {
            if(strSpiMasterHandleArray[u8SpiInstance].bIsInitialized)
            {
                /* Initiate SpiAl Master transfer */
                enuRetVal = (NRF_SUCCESS == nrf_drv_spi_transfer(&strSpiMasterInstanceArray[u8SpiInstance],
                                                                 pstrXfer->pu8TxBuffer,
                                                                 pstrXfer->u8TxBufferLength,
                                                                 pstrXfer->pu8RxBuffer,
                                                                 pstrXfer->u8RxBufferLength))
                                                                 ?Driver_Success:Driver_Failure;

                /* Set ongoing transfer flag if transfer is being performed in non-blocking mode */
                if(strSpiMasterHandleArray[u8SpiInstance].pfCallback)
                {
                    strSpiMasterHandleArray[u8SpiInstance].bTransferInProgress = true;
                }
            }
        }
        else
        {
            if(strSpiSlaveHandleArray[u8SpiInstance].bIsInitialized)
            {
                /* Initiate SpiAl Slave transfer */
                enuRetVal = (NRF_SUCCESS == nrf_drv_spis_buffers_set(&strSpiSlaveInstanceArray[u8SpiInstance],
                                                                     pstrXfer->pu8TxBuffer,
                                                                     pstrXfer->u8TxBufferLength,
                                                                     pstrXfer->pu8RxBuffer,
                                                                     pstrXfer->u8RxBufferLength))
                                                                     ?Driver_Success:Driver_Failure;

                /* Set ongoing transfer flag */
                strSpiSlaveHandleArray[u8SpiInstance].bTransferInProgress = true;
            }
        }
    }

    return enuRetVal;
}