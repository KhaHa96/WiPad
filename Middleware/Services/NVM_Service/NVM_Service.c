/* -----------------------------   NVM Service for nRF52832   ---------------------------------- */
/*  File      -  NVM Service source file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "NVM_Service.h"
#include "BLE_Service.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define NVM_PERSISTENT_KEYS_FILE_ID 0x8010
#define NVM_EXPIRABLE_KEYS_FILE_ID  0x9010
#define NVM_PEER_MANAGER_ADDR_START 0xC000
#define NVM_ID_LENGTH               8U

/************************************   GLOBAL VARIABLES   ***************************************/
/* Global function used to propagate dispatchable events to other tasks */
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
/* Flag indicating whether NVM_Service is initialized */
static bool bIsInitialized = false;

/* Flag indicating whether or not the current record update is a password registration operation */
static bool bIsPwdRegistration = false;

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidNvmEventHandler(fds_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {
        switch (pstrEvent->id)
        {
        case FDS_EVT_INIT:
        {
            if (NRF_SUCCESS == pstrEvent->result)
            {
                /* File system successfully installed in flash */
                bIsInitialized = true;
            }
        }
        break;

        case FDS_EVT_WRITE:
        {
            /* Peer manager uses the 0xC000 -- 0xFFFF address range and stores records with key
               values that happen to follow in that integer range as a way of tagging them as Peer
               manager records. It's therefore safe to assume that FDS records with key values
               outside of the Peer manager's address range are application records. */
            if((NRF_SUCCESS == pstrEvent->result) &&
               (pstrEvent->write.record_key < NVM_PEER_MANAGER_ADDR_START))
            {
                /* Notify Registration application of successful operation */
                (void)AppMgr_enuDispatchEvent(NVM_ENTRY_ADDED, NULL);
            }
        }
        break;

        case FDS_EVT_UPDATE:
        {
            if((NRF_SUCCESS == pstrEvent->result) && bIsPwdRegistration)
            {
                /* Notify Registration application of successful operation */
                (void)AppMgr_enuDispatchEvent(NVM_PASSWORD_REGISTERED, NULL);
            }
        }
        break;

        case FDS_EVT_DEL_RECORD:
        {
            if(NRF_SUCCESS == pstrEvent->result)
            {
                /* Added for debugging */
                __NOP();
            }
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
Mid_tenuStatus enuNvm_Init(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Register a Flash Data Storage event handler to receive FDS events */
    if(NRF_SUCCESS == fds_register(vidNvmEventHandler))
    {
        /* Initialize FDS by installing file system in flash */
        enuRetVal = (NRF_SUCCESS == fds_init())?Middleware_Success:Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuNVM_AddNewRecord(fds_record_desc_t *pstrRcDesc, Nvm_tstrRecord const *pstrRecord, Nvm_tenuFiles enuFile)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid parameters are passed and NVM_Service is initialized */
    if(pstrRcDesc && pstrRecord && (enuFile < Nvm_MaxFiles) && bIsInitialized)
    {
        /* We use two seperate files for expirable and persistent keys.
           Note: FDS imposes no major restrictions on file id and record key values (record keys
           must be distinct from 0x0000 and file Ids must be distinct from 0xFFFF). However, we need
           to particularly identify records with unique keys to be able to quickly find them between
           softdevice events. To establish this uniqueness, we use the four first numbers of the
           user Id. If this is not enough to distinguish between two entries that happen to have
           the same first four numbers, we will use the second four numbers to tell those two users
           apart. */

        /* Create string out of the four first numbers of user's Id */
        char *pchRecordKey = (char *)malloc((NVM_ID_LENGTH/2)+1);
        memcpy(pchRecordKey, &pstrRecord->u8Id[4], NVM_ID_LENGTH/2);
        pchRecordKey[NVM_ID_LENGTH/2] = '\0';

        /* Make sure data length is 4-byte aligned */
        fds_record_t const strFdsRecord =
        {
            .file_id = enuFile?NVM_EXPIRABLE_KEYS_FILE_ID:NVM_PERSISTENT_KEYS_FILE_ID,
            .key = (uint16_t)atoi(pchRecordKey),
            .data.p_data = pstrRecord,
            .data.length_words = (sizeof(*pstrRecord)+3) / sizeof(uint32_t),
        };

        /* Free allocated memory */
        free(pchRecordKey);

        /* Add new record to NVM */
        enuRetVal = (NRF_SUCCESS == fds_record_write(pstrRcDesc, &strFdsRecord))
                                                    ?Middleware_Success
                                                    :Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuNVM_FindRecord(uint16_t u16RecordKey, fds_record_desc_t *pstrRecordDesc, fds_find_token_t *pstrPersistentKeyToken, fds_find_token_t *pstrExpirableKeyToken)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid parameters are passed and NVM_Service is initialized */
    if(u16RecordKey && bIsInitialized)
    {
        memset(pstrRecordDesc, 0, sizeof(fds_record_desc_t));

        /* Go through persistent keys in NVM records and try to find a match */
        enuRetVal = (FDS_ERR_NOT_FOUND == fds_record_find(NVM_PERSISTENT_KEYS_FILE_ID,
                                                          u16RecordKey,
                                                          pstrRecordDesc,
                                                          pstrPersistentKeyToken))
                                                          ?Middleware_Failure
                                                          :Middleware_Success;
        if(Middleware_Failure == enuRetVal)
        {
            /* Go through expirable keys in NVM records and try to find a match */
            enuRetVal = (NRF_SUCCESS == fds_record_find(NVM_EXPIRABLE_KEYS_FILE_ID,
                                                        u16RecordKey,
                                                        pstrRecordDesc,
                                                        pstrExpirableKeyToken))
                                                        ?Middleware_Success
                                                        :Middleware_Failure;
        }
    }

    return enuRetVal;
}

Mid_tenuStatus enuNVM_ReadRecord(fds_record_desc_t *pstrRecordDesc, fds_flash_record_t *pstrRecord, Nvm_tstrRecord *pstrData)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid parameters are passed and NVM_Service is initialized */
    if(pstrRecordDesc && pstrRecord && bIsInitialized)
    {
        /* Open record */
        if(NRF_SUCCESS == fds_record_open(pstrRecordDesc, pstrRecord))
        {
            /* Copy data content out of NVM storage record */
            memcpy(pstrData, pstrRecord->p_data, sizeof(Nvm_tstrRecord));

            /* Close record when done reading to allow garbage collection to eventually reclaim
               record's memory space in flash */
            enuRetVal = (NRF_SUCCESS == fds_record_close(pstrRecordDesc))
                                                        ?Middleware_Success
                                                        :Middleware_Failure;
        }
    }

    return enuRetVal;
}

Mid_tenuStatus enuNVM_UpdateRecord(fds_record_desc_t *pstrRcDesc, Nvm_tstrRecord const *pstrRecord, Nvm_tenuFiles enuFile, bool bPwdReg)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid parameters are passed and NVM_Service is initialized */
    if(pstrRcDesc && pstrRecord && (enuFile < Nvm_MaxFiles) && bIsInitialized)
    {
        /* Create string out of the four first numbers of user's Id */
        char *pchRecordKey = (char *)malloc((NVM_ID_LENGTH/2)+1);
        memcpy(pchRecordKey, &pstrRecord->u8Id[4], NVM_ID_LENGTH/2);
        pchRecordKey[NVM_ID_LENGTH/2] = '\0';

        /* Make sure data length is 4-byte aligned */
        fds_record_t const strFdsRecord =
        {
            .file_id = enuFile?NVM_EXPIRABLE_KEYS_FILE_ID:NVM_PERSISTENT_KEYS_FILE_ID,
            .key = (uint16_t)atoi(pchRecordKey),
            .data.p_data = pstrRecord,
            .data.length_words = (sizeof(*pstrRecord)+3) / sizeof(uint32_t),
        };

        /* Free allocated memory */
        free(pchRecordKey);

        /* Set password registration flag if this update is to save a new password */
        bIsPwdRegistration = bPwdReg;

        /* Add new record to NVM */
        enuRetVal = (NRF_SUCCESS == fds_record_update(pstrRcDesc, &strFdsRecord))
                                                      ?Middleware_Success
                                                      :Middleware_Failure;
    }

    return enuRetVal;
}

Mid_tenuStatus enuNVM_DeleteRecord(fds_record_desc_t *pstrRcDesc)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Make sure valid arguments are passed and NVM_Service is initialized */
    if(pstrRcDesc && bIsInitialized)
    {
        enuRetVal = (NRF_SUCCESS == fds_record_delete(pstrRcDesc))
                                                      ?Middleware_Success
                                                      :Middleware_Failure;
    }

    return enuRetVal;
}