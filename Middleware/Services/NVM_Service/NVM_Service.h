/* -----------------------------   NVM Service for nRF52832   ---------------------------------- */
/*  File      -  NVM Service header file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

#ifndef _MID_NVM_H_
#define _MID_NVM_H_

/****************************************   INCLUDES   *******************************************/
#include "middleware_utils.h"
#include "system_config.h"
#include "App_types.h"
#include "fds.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define NVM_ID_SIZE  8U
#define NVM_PWD_SIZE 12U

/* Dispatchable events */
#define NVM_ENTRY_ADDED 17U

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * Nvm_tenuFiles Enumeration of the different FDS files used.
*/
typedef enum
{
    Nvm_PersistentKeys = 0, /* File used for user entries with persistent keys */
    Nvm_ExpirableKeys,      /* File used for user entries with expirable keys  */
    Nvm_MaxFiles            /* Max number of files used                        */
}Nvm_tenuFiles;

/**
 * Nvm_tstrTimeResKey Time-restricted key storage structure definition.
*/
typedef struct
{
    bool bIsKeyActive;
    uint16_t u16Timeout;
}Nvm_tstrTimeResKey;

/**
 * Nvm_tstrCountResKey Count-restricted key storage structure definition.
*/
typedef struct
{
    uint16_t u16CountLimit;
    uint16_t u16UsedCount;
}Nvm_tstrCountResKey;

/**
 * Nvm_tstrRecord FDS record-defining structure.
*/
typedef struct
{
    uint8_t u8Id[NVM_ID_SIZE];           /* User Id                         */
    uint8_t u8Password[NVM_PWD_SIZE];    /* User password                   */
    App_tenuKeyTypes enuKeyType;         /* User's key type                 */
    union{
        Nvm_tstrCountResKey strCountRes; /* Count-restricted key quantifier */
        Nvm_tstrTimeResKey strTimeRes;   /* Time-restricted key quantifier  */
    }uKeyQuantifier;
}Nvm_tstrRecord;

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuNvm_Init Initializes the NVM middleware service responsible for manipulating FDS.
 *
 * @note This is an asynchronous function. Completion is reported through the FDS_EVT_INIT event
 *       in vidNvmEventHandler.
 *
 * @pre This function is preferably executed after initializing Softdevice.
 *
 * @return Mid_tenuStatus Middleware_Success if initialization request was successfully queued,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuNvm_Init(void);

/**
 * @brief enuNVM_AddNewRecord Adds a new entry in the form of an FDS record to the NVM file system.
 *
 * @note NVM_Service uses two seperate files to keep track of data entries depending the provided
 *       user key type; expirable as in one-time, count-restricted and time-restricted keys and
 *       persistent as in unlimited and admin keys.
 *
 * @pre enuNvm_Init must be called before attempting any record write to NVM.
 *
 * @param pstrRcDesc Pointer to record descriptor structure.
 * @param pstrRecord Pointer to data record structure.
 * @param enuFile File to be used for record storage.
 *
 * @return Mid_tenuStatus Middleware_Success if write operation request was successfully queued,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuNVM_AddNewRecord(fds_record_desc_t *pstrRcDesc, Nvm_tstrRecord const *pstrRecord, Nvm_tenuFiles enuFile);

/**
 * @brief enuNVM_FindRecord Goes through the NVM file system looking for a record identified by
 *        its key.
 *
 * @note This is a synchronous call. Both NVM_Service's files are looked through to find a match
 *       for the given record which take a while depending on the number of records stored in the
 *       file system.
 *
 * @pre enuNvm_Init must be called before attempting to find any record in file system.
 *
 * @param u16RecordKey Record key to be looked for.
 * @param pstrRecordDesc Pointer to record descriptor structure.
 * @param pstrPersistentKeyToken Pointer to token structure for persistent key record.
 * @param pstrExpirableKeyToken Pointer to token structure for expirable key record.
 *
 * @return Mid_tenuStatus Middleware_Success if record was successfully found in NVM,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuNVM_FindRecord(uint16_t u16RecordKey, fds_record_desc_t *pstrRecordDesc, fds_find_token_t *pstrPersistentKeyToken, fds_find_token_t *pstrExpirableKeyToken);

/**
 * @brief enuNVM_ReadRecord Extracts record data from NVM.
 *
 * @note This is a synchronous call. Every read operation involves opening a record, copying its
 *       content then closing it again.
 *
 * @pre enuNvm_Init must be called before attempting to read any record.
 *
 * @param pstrRecordDesc Pointer to record descriptor structure.
 * @param pstrRecord Pointer to record structure as defined by FDS.
 * @param pstrData Pointer to record structure as defined by NVM_Service.
 *
 * @return Mid_tenuStatus Middleware_Success if record was extracted and read successfully,
 *         Middleware_Failure otherwise.
 */
Mid_tenuStatus enuNVM_ReadRecord(fds_record_desc_t *pstrRecordDesc, fds_flash_record_t *pstrRecord, Nvm_tstrRecord *pstrData);

#endif /* _MID_NVM_H_ */