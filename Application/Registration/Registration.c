/* -------------------------   User registration app for nRF52832   ---------------------------- */
/*  File      -  User registration application source file                                       */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "Registration.h"
#include "BLE_Service.h"
#include "NVM_Service.h"

/************************************   PRIVATE DEFINES   ****************************************/
#define APP_USEREG_PUSH_IMMEDIATELY     0U
#define APP_USEREG_POP_IMMEDIATELY      0U
#define APP_USEREG_KEY_PARAMETER_LENGTH 4U
#define APP_USEREG_ID_LENGTH            8U
#define APP_USEREG_MIN_PASSWORD_LENGTH  8U
#define APP_USEREG_MAX_PASSWORD_LENGTH  12U
#define APP_USEREG_MAX_COMMAND_LENGTH   20U
#define APP_USEREG_EVENT_MASK           (APP_USEREG_PEER_DISCONNECTION | \
                                         APP_USEREG_NOTIF_ENABLED      | \
                                         APP_USEREG_NOTIF_DISABLED     | \
                                         APP_USEREG_USR_INPUT_RX       | \
                                         APP_USEADM_NOTIF_ENABLED      | \
                                         APP_USEADM_NOTIF_DISABLED     | \
                                         APP_USEADM_USR_INPUT_RX       | \
                                         APP_USEADM_USR_ADDED_TO_NVM   | \
                                         APP_USEADM_PASSWORD_UPDATED     )

/************************************   PRIVATE MACROS   *****************************************/
/* Compute state machine trigger count */
#define APP_USEREG_TRIGGER_COUNT(list) (sizeof(list) / sizeof(Registration_tstrState))

/* NVM record finder assert macro */
#define APP_RECORD_ASSERT(RCD)    \
(                                 \
    (RCD->pu8Id)               && \
    (RCD->pstrRecordDesc)      && \
    (RCD->pstrPersistentToken) && \
    (RCD->pstrExpirableToken)  && \
    (RCD->pstrFdsRecord)       && \
    (RCD->pstrAppRecord)          \
)

/*************************************   PRIVATE TYPES    ****************************************/
/**
 * App_tstrRecordSearch Structure type used to look for application record in the NVM file system.
*/
typedef struct
{
    uint8_t const *pu8Id;                  /* Provided user Id                           */
    uint16_t u16RecordKey;                 /* Record key                                 */
    fds_record_desc_t *pstrRecordDesc;     /* Record descriptor                          */
    fds_find_token_t *pstrPersistentToken; /* Record find token for persistent keys file */
    fds_find_token_t *pstrExpirableToken;  /* Record find token for expirable keys file  */
    fds_flash_record_t *pstrFdsRecord;     /* Record as sen by FDS                       */
    Nvm_tstrRecord *pstrAppRecord;         /* Record as seen by the application          */
}App_tstrRecordSearch;

/************************************   GLOBAL VARIABLES   ***************************************/
/* Global function used to propagate dispatchable events to other tasks */
extern App_tenuStatus AppMgr_enuDispatchEvent(uint32_t u32Event, void *pvData);

/************************************   PRIVATE VARIABLES   **************************************/
static TaskHandle_t pvUseRegTaskHandle;             /* Registration task handle                  */
static QueueHandle_t pvUseRegQueueHandle;           /* Registration queue handle                 */
static EventGroupHandle_t pvUseRegEventGroupHandle; /* Registration event group handle           */
static volatile bool bRegNotifEnabled = false;      /* Notifications enabled/disabled on ble_reg */
static volatile bool bAdmNotifEnabled = false;      /* Notifications enabled/disabled on ble_adm */
static volatile bool bExpectingPwd = false;         /* Next input should be Id/Pwd on ble_reg    */
static volatile bool bUseSignedIn = false;          /* Active user has/hasn't already signed in  */
static volatile bool bAdmSignedIn = false;          /* Admin user is/isn't currently signed in   */
static void vidUseRegDisconnected(void *pvArg);     /* Disconnection function prototype          */
static void vidUseRegNotifEnabled(void *pvArg);     /* Notifs enabled on ble_reg func prototype  */
static void vidUseRegNotifDisabled(void *pvArg);    /* Notifs disabled on ble_reg func prototype */
static void vidUseRegInputReceived(void *pvArg);    /* Input received on ble_reg func prototype  */
static void vidUseAdmNotifEnabled(void *pvArg);     /* Notifs enabled on ble_adm func prototype  */
static void vidUseAdmNotifDisabled(void *pvArg);    /* Notifs disabled on ble_adm func prototype */
static void vidUseAdmInputReceived(void *pvArg);    /* Input received on ble_adm func prototype  */
static void vidUseAdmAddedToNvm(void *pvArg);       /* New entry added to NVM func prototype     */
static void vidUserPasswordUpdated(void *pvArg);    /* User password updated func prototype      */
static uint8_t u8AddUsrCmd[] = "mkusi";             /* Add user command base                     */
static uint8_t u8UsrDataCmd[] = "mkud -i ";         /* Extract user data command base            */
static uint8_t u8InvalidPasswordBase[8];            /* Invalid pwd base for unregistered users   */
static uint8_t u8CurrentUserPwd[12];                /* Active user's password extracted from NVM */
static fds_record_desc_t strActiveRecordDesc = {0}; /* Active NVM record's descriptor            */
static Nvm_tstrRecord strActiveRecord = {0};        /* Active NVM record data content            */

/* Registration state machine's entry list */
static const Registration_tstrState strUseRegStateMachine[] =
{
    {APP_USEREG_PEER_DISCONNECTION, vidUseRegDisconnected }, /* Disconnected from peer            */
    {APP_USEREG_NOTIF_ENABLED     , vidUseRegNotifEnabled }, /* Notifications enabled on ble_reg  */
    {APP_USEREG_NOTIF_DISABLED    , vidUseRegNotifDisabled}, /* Notifications disabled on ble_reg */
    {APP_USEREG_USR_INPUT_RX      , vidUseRegInputReceived}, /* Input received on ble_reg         */
    {APP_USEADM_NOTIF_ENABLED     , vidUseAdmNotifEnabled }, /* Notifications enabled on ble_adm  */
    {APP_USEADM_NOTIF_DISABLED    , vidUseAdmNotifDisabled}, /* Notifications disabled on ble_adm */
    {APP_USEADM_USR_INPUT_RX      , vidUseAdmInputReceived}, /* Input received on ble_adm         */
    {APP_USEADM_USR_ADDED_TO_NVM  , vidUseAdmAddedToNvm   }, /* New user added to NVM             */
    {APP_USEADM_PASSWORD_UPDATED  , vidUserPasswordUpdated}  /* User password updated             */
};

/************************************   PRIVATE FUNCTIONS   **************************************/
static bool bUserRecordFound(App_tstrRecordSearch *pstrRecordFind)
{
    bool bRetVal = false;

    /* Make sure valid arguments are passed */
    if(pstrRecordFind && APP_RECORD_ASSERT(pstrRecordFind))
    {
        uint8_t u8Retries = APP_USEREG_MAX_CROSS_IDS;

        while((!bRetVal) && (u8Retries--))
        {
            /* We use the first 4 numbers of a user Id as a key for that user's entire NVM record.
               This leaves a slight margin of chance for 2 users having the same first 4 numbers of
               their Id to have the same record key. If they also happen to have similar keys (as
               in both expirable or both persistent), they will be indiscernible. */
            if(Middleware_Success == enuNVM_FindRecord(pstrRecordFind->u16RecordKey,
                                                       pstrRecordFind->pstrRecordDesc,
                                                       pstrRecordFind->pstrPersistentToken,
                                                       pstrRecordFind->pstrExpirableToken))
            {
                /* Read record content */
                if(Middleware_Success == enuNVM_ReadRecord(pstrRecordFind->pstrRecordDesc,
                                                           pstrRecordFind->pstrFdsRecord,
                                                           pstrRecordFind->pstrAppRecord))
                {
                    /* Once we locate the user record using the first 4 numbers of their Id, we
                       compare that record's Id against the record we're actively looking for
                       to account for the possibility of more than one user having the same first 4
                       Id numbers. If more than APP_USEREG_MAX_CROSS_IDS users share those same
                       first 4 numbers, this function will fail. */
                    bRetVal = (0 == s8StringCompare(&pstrRecordFind->pstrAppRecord->u8Id[0],
                                                    &pstrRecordFind->pu8Id[0],
                                                    APP_USEREG_ID_LENGTH));
                    if(bRetVal)
                    {
                        /* Record located in NVM */
                        break;
                    }
                }
            }
        }
    }

    return bRetVal;
}

static void vidUseRegDisconnected(void *pvArg)
{
    /* Reset all global variables */
    bRegNotifEnabled = false;
    bAdmNotifEnabled = false;
    bExpectingPwd = false;
    bUseSignedIn = false;
    bAdmSignedIn = false;
    memset(&strActiveRecordDesc, 0, sizeof(strActiveRecordDesc));
    memset(&strActiveRecord, 0, sizeof(strActiveRecord));
    memset(u8InvalidPasswordBase, 0xFF, APP_USEREG_MIN_PASSWORD_LENGTH);
    memset(u8CurrentUserPwd, 0xFF, APP_USEREG_MAX_PASSWORD_LENGTH);
}

static void vidUseRegNotifEnabled(void *pvArg)
{
    /* User Registration service's notifications enabled. Toggle its notifications enabled flag */
    bRegNotifEnabled = true;

    /* Prompt user to input their Id */
    uint8_t u8NotificationBuffer[] = "Please input your Id";
    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
    (void)enuTransferNotification(Ble_Registration, u8NotificationBuffer, &u16NotificationSize);
}

static void vidUseRegNotifDisabled(void *pvArg)
{
    /* User Registration service's notifications disabled. Toggle its notifications enabled flag */
    bRegNotifEnabled = false;
}

static void vidUseRegInputReceived(void *pvArg)
{
    /* If notifications on User Registration's Status characteristic are disabled, we don't even
       process user's input */
    if(bRegNotifEnabled)
    {
        /* Make sure valid parameters are passed */
        if(pvArg)
        {
            /* Check whether user is already signed in */
            if(!bUseSignedIn)
            {
                /* Extract received data */
                Ble_tstrRxData *pstrInput = (Ble_tstrRxData *)pvArg;

                /* Check whether we're expecting a user Id or a user password */
                if(bExpectingPwd)
                {
                    /* Make sure user input a valid password */
                    if(bContainsSpecialChar(pstrInput->pu8Data, pstrInput->u16Length) &&
                       bContainsNumeral(pstrInput->pu8Data, pstrInput->u16Length) &&
                       (pstrInput->u16Length <= APP_USEREG_MAX_PASSWORD_LENGTH) &&
                       (pstrInput->u16Length >= APP_USEREG_MIN_PASSWORD_LENGTH))
                    {
                        if(0 == s8StringCompare(&strActiveRecord.u8Password[0],
                                                &u8InvalidPasswordBase[0],
                                                APP_USEREG_MIN_PASSWORD_LENGTH))
                        {
                            /* No prior password registered for this user. Register a new one by
                               updating invalid password stored in NVM record */
                            memcpy(&strActiveRecord.u8Password[0], &pstrInput->pu8Data[0], pstrInput->u16Length);
                            Nvm_tenuFiles enuFile = ((App_CountRestrictedKey == strActiveRecord.enuKeyType) ||
                                                     (App_TimeRestrictedKey == strActiveRecord.enuKeyType) ||
                                                     (App_OneTimeKey == strActiveRecord.enuKeyType))
                                                    ?Nvm_ExpirableKeys
                                                    :Nvm_PersistentKeys;
                            /* Update NVM record */
                            (void)enuNVM_UpdateRecord(&strActiveRecordDesc, &strActiveRecord, enuFile, true);
                        }
                        else if(0 == s8StringCompare(&pstrInput->pu8Data[0],
                                                     &strActiveRecord.u8Password[0],
                                                     pstrInput->u16Length))
                        {
                            if(App_AdminKey == strActiveRecord.enuKeyType)
                            {
                                /* Admin successfully logged in. Toggle admin signed-in flag */
                                bAdmSignedIn = true;

                                /* Toggle user signed-in flag to ensure Admin doesn't try to log
                                   in again */
                                bUseSignedIn = true;

                                /* Notify Admin that they've managed to log in and prompt them to check
                                   the Admin User service */
                                uint8_t u8NotificationBuffer[] = "Hi Admin! See BleAdm";
                                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                                (void)enuTransferNotification(Ble_Registration,
                                                              u8NotificationBuffer,
                                                              &u16NotificationSize);
                            }
                            else
                            {
                                /* User successfully logged in. Toggle user signed-in flag */
                                bUseSignedIn = true;

                                /* Notify user that they've managed to log in and prompt them to check
                                   the Key Attribution service */
                                uint8_t u8NotificationBuffer[] = "Hi again! See BleAtt";
                                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                                (void)enuTransferNotification(Ble_Registration,
                                                              u8NotificationBuffer,
                                                              &u16NotificationSize);
                            }

                            /* Notify attribution application
                               Note: Data must be preserved until the Attribution application
                               receives and processes it. */
                            Nvm_tstrRecordDispatch *pstrRecordDispatch = (Nvm_tstrRecordDispatch *)malloc(sizeof(Nvm_tstrRecordDispatch));

                            /* Successfully allocated memory for data pointer */
                            if(pstrRecordDispatch)
                            {
                                pstrRecordDispatch->pstrRecordDesc = (fds_record_desc_t *)malloc(sizeof(fds_record_desc_t));

                                if(NULL == pstrRecordDispatch->pstrRecordDesc)
                                {
                                    free(pstrRecordDispatch);
                                }
                                else
                                {
                                    pstrRecordDispatch->pstrRecord = (Nvm_tstrRecord *)malloc(sizeof(Nvm_tstrRecord));

                                    if(NULL == pstrRecordDispatch->pstrRecord)
                                    {
                                        free(pstrRecordDispatch->pstrRecordDesc);
                                    }
                                }

                                /* Copy data into dispatchable structure */
                                memcpy((void *)pstrRecordDispatch->pstrRecordDesc,
                                        (void *)&strActiveRecordDesc,
                                        sizeof(strActiveRecordDesc));
                                memcpy((void *)pstrRecordDispatch->pstrRecord,
                                       (void *)&strActiveRecord,
                                       sizeof(strActiveRecord));

                                /* Dispatch structure to the Attribution application */
                                (void)AppMgr_enuDispatchEvent(BLE_USEREG_USER_SIGNED_IN,
                                                              (void *)pstrRecordDispatch);
                            }
                        }
                        else
                        {
                            /* Notify user of wrong password */
                            uint8_t u8NotificationBuffer[] = "Wrong password!";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Registration,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Display visual cue */
                            (void)AppMgr_enuDispatchEvent(BLE_USEREG_INVALID_INPUT, NULL);
                        }
                    }
                    else
                    {
                        /* Notify user of invalid password format */
                        uint8_t u8NotificationBuffer[] = "Invalid! Try again";
                        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                        (void)enuTransferNotification(Ble_Registration, u8NotificationBuffer, &u16NotificationSize);
                        /* Display visual cue */
                        (void)AppMgr_enuDispatchEvent(BLE_USEREG_INVALID_INPUT, NULL);
                    }
                }
                else
                {
                    /* Make sure user input is a valid Id */
                    if(bIsAllNumerals(pstrInput->pu8Data, APP_USEREG_ID_LENGTH) &&
                       (APP_USEREG_ID_LENGTH == pstrInput->u16Length))
                    {
                        /* Extract record key from Id */
                        fds_record_desc_t strRecordDesc = {0};
                        fds_find_token_t strPersistentToken = {0};
                        fds_find_token_t strExpirableToken = {0};
                        char *pchRecordKey = (char *)malloc((APP_USEREG_ID_LENGTH/2)+1);
                        memcpy(pchRecordKey, &pstrInput->pu8Data[4], (APP_USEREG_ID_LENGTH/2));
                        pchRecordKey[(APP_USEREG_ID_LENGTH/2)] = '\0';
                        free(pchRecordKey);

                        /* Find record in NVM */
                        fds_flash_record_t strFdsRecord = {0};
                        Nvm_tstrRecord strRecord;
                        App_tstrRecordSearch strRecordSearch;
                        strRecordSearch.pu8Id = &pstrInput->pu8Data[0];
                        strRecordSearch.u16RecordKey = (uint16_t)atoi(pchRecordKey);
                        strRecordSearch.pstrRecordDesc = &strRecordDesc;
                        strRecordSearch.pstrPersistentToken = &strPersistentToken;
                        strRecordSearch.pstrExpirableToken = &strExpirableToken;
                        strRecordSearch.pstrFdsRecord = &strFdsRecord;
                        strRecordSearch.pstrAppRecord = &strRecord;

                        if(bUserRecordFound(&strRecordSearch))
                        {
                            /* Id located in NVM. Toggle expecting password flag */
                            bExpectingPwd = true;

                            /* Store record descriptor and record content */
                            memcpy(&strActiveRecordDesc, &strRecordDesc, sizeof(fds_record_desc_t));
                            memcpy(&strActiveRecord, &strRecord, sizeof(Nvm_tstrRecord));

                            /* Ask user to input their password */
                            uint8_t u8NotificationBuffer[] = "Please type password";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Registration,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Display visual cue */
                            (void)AppMgr_enuDispatchEvent(BLE_USEREG_VALID_INPUT, NULL);
                        }
                        else
                        {
                            /* Notify user that they haven't been found in WiPad's database */
                            uint8_t u8NotificationBuffer[] = "Unregistered Id";
                            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                            (void)enuTransferNotification(Ble_Registration,
                                                          u8NotificationBuffer,
                                                          &u16NotificationSize);
                            /* Display visual cue */
                            (void)AppMgr_enuDispatchEvent(BLE_USEREG_INVALID_INPUT, NULL);
                        }
                    }
                    else
                    {
                        /* Notify user of invalid Id format */
                        uint8_t u8NotificationBuffer[] = "Invalid! Try again";
                        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                        (void)enuTransferNotification(Ble_Registration, u8NotificationBuffer, &u16NotificationSize);
                        /* Display visual cue */
                        (void)AppMgr_enuDispatchEvent(BLE_USEREG_INVALID_INPUT, NULL);
                    }
                }
            }
            else
            {
                /* Notify user that they're already signed in */
                uint8_t u8NotificationBuffer[] = "Already signed in";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                (void)enuTransferNotification(Ble_Registration, u8NotificationBuffer, &u16NotificationSize);
            }

            /* Free allocated memory */
            free((void *)((Ble_tstrRxData *)pvArg)->pu8Data);
            free((void *)(Ble_tstrRxData *)pvArg);
        }
    }
    else
    {
        /* Received user input while notifications are disabled. Give user a visual heads-up */
        (void)AppMgr_enuDispatchEvent(BLE_USEREG_NOTIF_DISABLED, NULL);
    }
}

static void vidUseAdmNotifEnabled(void *pvArg)
{
    /* Admin User service's notifications enabled. Toggle its notifications enabled flag */
    bAdmNotifEnabled = true;

    /* Prompt Admin user to sign in if they haven't already, otherwise display a simple greeting */
    const char *pchNotification = bAdmSignedIn?"Hi there Admin":"Please input your Id";
    uint16_t u16NotificationSize = strlen(pchNotification);
    (void)enuTransferNotification(Ble_Admin, (uint8_t *)pchNotification, &u16NotificationSize);
}

static void vidUseAdmNotifDisabled(void *pvArg)
{
    /* Admin User service's notifications disabled. Toggle its notifications enabled flag */
    bAdmNotifEnabled = false;
}

static Registration_tenuAdmCmdType enuExtractCommandType(const uint8_t *pu8Data, uint8_t u8Length)
{
    Registration_tenuAdmCmdType enuRetVal = Adm_InvalidCmd;

    /* Make sure valid parameters are passed */
    if(pu8Data && (u8Length > 0) && (u8Length <= APP_USEREG_MAX_COMMAND_LENGTH))
    {
        /* Only accept 15, 16 and 20-character-long commands (Add user with persistant key, get
           user data and add user with expirable key commands are 15, 16 and 20 characters long
           respectively). */
        if((15 == u8Length) || (20 == u8Length))
        {
            /* Compare received input against the Add user command base */
            if(0 == s8StringCompare(pu8Data, u8AddUsrCmd, 5))
            {
                /* Add user command received. Check whether input contains a valid user Id */
                if(bIsAllNumerals(&pu8Data[5], APP_USEREG_ID_LENGTH))
                {
                    /* Check for specific key type specifiers in the received input */
                    if(((0 == s8StringCompare(&pu8Data[13], "k1", 2))  ||
                        (0 == s8StringCompare(&pu8Data[13], "k3", 2))  ||
                        (0 == s8StringCompare(&pu8Data[13], "k5", 2))) &&
                       (15 == u8Length))
                    {
                        enuRetVal = Adm_AddUser;
                    }
                    else if((0 == s8StringCompare(&pu8Data[13], "k2", 2)) && (20 == u8Length))
                    {
                        if('c' == pu8Data[15])
                        {
                            if(bIsAllNumerals(&pu8Data[16], 4))
                            {
                                enuRetVal = Adm_AddUser;
                            }
                        }
                    }
                    else if((0 == s8StringCompare(&pu8Data[13], "k4", 2)) && (20 == u8Length))
                    {
                        if('t' == pu8Data[15])
                        {
                            if(bIsAllNumerals(&pu8Data[16], 4))
                            {
                                enuRetVal = Adm_AddUser;
                            }
                        }
                    }
                }
            }
        }
        else if(16 == u8Length)
        {
            /* Compare received input against the User data command base */
            if(0 == s8StringCompare(pu8Data, u8UsrDataCmd, 8))
            {
                /* User data command received. Check whether input contains a valid user Id */
                if(bIsAllNumerals(&pu8Data[8], APP_USEREG_ID_LENGTH))
                {
                    enuRetVal = Adm_UserData;
                }
            }
        }
    }

    return enuRetVal;
}

static App_tenuKeyTypes enuDecodeAddCommand(const uint8_t *pu8Data, uint16_t u16Length, uint16_t *pu16Arg)
{
    App_tenuKeyTypes enuRetVal = App_KeyLowerBound;

    /* Make sure valid parameters are passed */
    if(pu8Data && pu16Arg)
    {
        if((20 == u16Length) && ('c' == pu8Data[15]))
        {
            /* The atoi function expects a C-style string with a NULL-terminated character array */
            char *pchKeyCountStr = (char *)malloc(APP_USEREG_KEY_PARAMETER_LENGTH+1);
            memcpy(pchKeyCountStr, &pu8Data[16], APP_USEREG_KEY_PARAMETER_LENGTH);
            pchKeyCountStr[APP_USEREG_KEY_PARAMETER_LENGTH] = '\0';
            *pu16Arg = (uint16_t)atoi(pchKeyCountStr);
            enuRetVal = App_CountRestrictedKey;
            free(pchKeyCountStr);
        }
        else if((20 == u16Length) && ('t' == pu8Data[15]))
        {
            /* The atoi function expects a C-style string with a NULL-terminated character array */
            char *pchKeyTimeoutStr = (char *)malloc(APP_USEREG_KEY_PARAMETER_LENGTH+1);
            memcpy(pchKeyTimeoutStr, &pu8Data[16], APP_USEREG_KEY_PARAMETER_LENGTH);
            pchKeyTimeoutStr[APP_USEREG_KEY_PARAMETER_LENGTH] = '\0';
            *pu16Arg = (uint16_t)atoi(pchKeyTimeoutStr);
            enuRetVal = App_TimeRestrictedKey;
            free(pchKeyTimeoutStr);
        }
        else
        {
            /* The atoi function expects a C-style string with a NULL-terminated character array */
            char *pchKeyStr = (char *)malloc(2);
            memcpy(pchKeyStr, &pu8Data[14], 1);
            pchKeyStr[1] = '\0';
            pu16Arg = NULL;
            enuRetVal = (App_tenuKeyTypes)atoi(pchKeyStr);
            free(pchKeyStr);
        }
    }

    return enuRetVal;
}

static void vidUserDataNotify(Nvm_tstrRecord *pstrRecord)
{
    /* Make sure valid arguments are passed */
    if(pstrRecord)
    {
        switch(pstrRecord->enuKeyType)
        {
        case App_OneTimeKey:
        {
            if(pstrRecord->uKeyQuantifier.bOneTimeExpired)
            {
                /* One-time expirable key has been used */
                uint8_t u8NotificationBuffer[] = "One-time: Used";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

                /* Transfer notification to peer */
                (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
            }
            else
            {
                /* One-time expirable key still available */
                uint8_t u8NotificationBuffer[] = "One-time expirable";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

                /* Transfer notification to peer */
                (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
                
            }
        }
        break;

        case App_CountRestrictedKey:
        {
            /* Count-restricted expirable key */
            uint8_t u8NotificationBuffer[] = "Count-limited: ";
            /* Copy count-limit value into buffer in a human-readable format */
            uint8_t u8LimitDigitCnt = u8DigitCount(pstrRecord->uKeyQuantifier.strCountRes.u16CountLimit);
            char chTimeoutStr[5];
            snprintf(chTimeoutStr, sizeof(chTimeoutStr), "%d", pstrRecord->uKeyQuantifier.strCountRes.u16CountLimit -
                                                               pstrRecord->uKeyQuantifier.strCountRes.u16UsedCount);
            strncpy((char *)&u8NotificationBuffer[15], chTimeoutStr, u8LimitDigitCnt+1);
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)+u8LimitDigitCnt;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_UnlimitedKey:
        {
            /* Unlimited persistent key */
            uint8_t u8NotificationBuffer[] = "Unlimited persistent";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_TimeRestrictedKey:
        {
            /* Time-restricted expirable key */
            uint8_t u8NotificationBuffer[] = "Time-limited: ";
            /* Copy timeout value into buffer in a human-readable format */
            uint8_t u8TimeoutDigitCnt = u8DigitCount(pstrRecord->uKeyQuantifier.strTimeRes.u16Timeout);
            char chTimeoutStr[5];
            snprintf(chTimeoutStr, sizeof(chTimeoutStr), "%d", pstrRecord->uKeyQuantifier.strTimeRes.u16Timeout);
            strncpy((char *)&u8NotificationBuffer[14], chTimeoutStr, u8TimeoutDigitCnt+1);
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)+u8TimeoutDigitCnt;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        case App_AdminKey:
        {
            /* Admin persistent key */
            uint8_t u8NotificationBuffer[] = "Admin persistent";
            uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;

            /* Transfer notification to peer */
            (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);
        }
        break;

        default:
            /* Nothing to do */
            break;
        }
    }
}

static void vidUseAdmInputReceived(void *pvArg)
{
    /* If notifications on Admin User's Status characteristic are disabled, we don't even
       process user's input */
    if(bAdmNotifEnabled)
    {
        /* Make sure valid parameters are passed */
        if(pvArg)
        {
            /* Check whether user has already been through the Admin authentication process */
            if(bAdmSignedIn)
            {
                /* Extract command from received data */
                Ble_tstrRxData *pstrCommand = (Ble_tstrRxData *)pvArg;
                /* Determine command type */
                Registration_tenuAdmCmdType enuCmdType = enuExtractCommandType(pstrCommand->pu8Data,
                                                                               pstrCommand->u16Length);

                switch(enuCmdType)
                {
                case Adm_AddUser:
                {
                    /* Decode Add user command to extract key type.
                       Note: enuDecodeAddCommand shouldn't be called unless we have determined for sure
                       that user input is a valid add user command by calling enuExtractCommandType. */
                    uint16_t u16Argument;
                    App_tenuKeyTypes enuKeyType = enuDecodeAddCommand(pstrCommand->pu8Data,
                                                                      pstrCommand->u16Length,
                                                                      &u16Argument);

                    /* Create new NVM entry */
                    Nvm_tenuFiles enuNvmFile;
                    fds_record_desc_t strRecordDesc = {0};
                    Nvm_tstrRecord strRecord;

                    /* Set Id extracted from command, invalid password and key type in NVM entry */
                    memcpy(strRecord.u8Id, &pstrCommand->pu8Data[5], APP_USEREG_ID_LENGTH);
                    memset(strRecord.u8Password, 0xFF, APP_USEREG_MAX_PASSWORD_LENGTH);
                    strRecord.enuKeyType = enuKeyType;
                    if(App_CountRestrictedKey == enuKeyType)
                    {
                        /* Set count-restricted key's count-limit */
                        strRecord.uKeyQuantifier.strCountRes.u16CountLimit = u16Argument;
                        strRecord.uKeyQuantifier.strCountRes.u16UsedCount = 0;
                        enuNvmFile = Nvm_ExpirableKeys;
                    }
                    else if(App_TimeRestrictedKey == enuKeyType)
                    {
                        /* Set time-restricted key's timeout */
                        strRecord.uKeyQuantifier.strTimeRes.bIsKeyActive = false;
                        strRecord.uKeyQuantifier.strTimeRes.u16Timeout = u16Argument;
                        enuNvmFile = Nvm_ExpirableKeys;
                    }
                    else if(App_OneTimeKey == enuKeyType)
                    {
                        strRecord.uKeyQuantifier.bOneTimeExpired = false;
                        enuNvmFile = Nvm_ExpirableKeys;
                    }
                    else
                    {
                        enuNvmFile = Nvm_PersistentKeys;
                    }
                    /* Add new NVM entry */
                    (void)enuNVM_AddNewRecord(&strRecordDesc, &strRecord, enuNvmFile);
                }
                break;

                case Adm_UserData:
                {
                    /* Extract record key from command */
                    fds_record_desc_t strRecordDesc = {0};
                    fds_find_token_t strPersistentToken = {0};
                    fds_find_token_t strExpirableToken = {0};
                    char *pchRecordKey = (char *)malloc((APP_USEREG_ID_LENGTH/2)+1);
                    memcpy(pchRecordKey, &pstrCommand->pu8Data[12], (APP_USEREG_ID_LENGTH/2));
                    pchRecordKey[(APP_USEREG_ID_LENGTH/2)] = '\0';
                    free(pchRecordKey);

                    /* Find record in NVM */
                    fds_flash_record_t strFdsRecord = {0};
                    Nvm_tstrRecord strRecord;
                    App_tstrRecordSearch strRecordSearch;
                    strRecordSearch.pu8Id = &pstrCommand->pu8Data[8];
                    strRecordSearch.u16RecordKey = (uint16_t)atoi(pchRecordKey);
                    strRecordSearch.pstrRecordDesc = &strRecordDesc;
                    strRecordSearch.pstrPersistentToken = &strPersistentToken;
                    strRecordSearch.pstrExpirableToken = &strExpirableToken;
                    strRecordSearch.pstrFdsRecord = &strFdsRecord;
                    strRecordSearch.pstrAppRecord = &strRecord;

                    if(bUserRecordFound(&strRecordSearch))
                    {
                        /* User data extracted successfully. Send user data to peer */
                        vidUserDataNotify(&strRecord);

                        /* TODO: Print it on CLI */

                        /* Display visual cue */
                        (void)AppMgr_enuDispatchEvent(BLE_USEREG_USER_DATA, NULL);
                    }
                    else
                    {
                        /* Notify user that record couldn't be found */
                        uint8_t u8NotificationBuffer[] = "Could not find Id";
                        uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                        (void)enuTransferNotification(Ble_Admin,
                                                      u8NotificationBuffer,
                                                      &u16NotificationSize);
                    }
                }
                break;

                case Adm_InvalidCmd:
                {
                    /* Notify user of invalid input */
                    uint8_t u8NotificationBuffer[] = "Invalid! Try again";
                    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                    (void)enuTransferNotification(Ble_Admin,
                                                  u8NotificationBuffer,
                                                  &u16NotificationSize);
                    /* Display visual cue */
                    (void)AppMgr_enuDispatchEvent(BLE_USEREG_INVALID_INPUT, NULL);
                }
                break;

                default:
                    /* Nothing to do */
                    break;
                }
            }
            else
            {
                /* User hasn't signed in as Admin yet. Prompt them to do so */
                uint8_t u8NotificationBuffer[] = "Please sign in first";
                uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
                (void)enuTransferNotification(Ble_Admin,
                                              u8NotificationBuffer,
                                              &u16NotificationSize);
            }

            /* Free allocated memory */
            free((void *)((Ble_tstrRxData *)pvArg)->pu8Data);
            free((void *)(Ble_tstrRxData *)pvArg);
        }
    }
    else
    {
        /* Received user input while notifications are disabled. Give user a visual heads-up */
        (void)AppMgr_enuDispatchEvent(BLE_USEREG_NOTIF_DISABLED, NULL);
    }
}

static void vidUseAdmAddedToNvm(void *pvArg)
{
    /* Send notification to peer */
    uint8_t u8NotificationBuffer[] = "User added";
    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
    (void)enuTransferNotification(Ble_Admin, u8NotificationBuffer, &u16NotificationSize);

    /* New user successfully added to NVM */
    (void)AppMgr_enuDispatchEvent(BLE_USEREG_USER_ADDED, NULL);
}

static void vidRegistrationEvent_Process(uint32_t u32Trigger, void *pvData)
{
    /* Go through trigger list to find trigger.
       Note: We use a while loop as we require that no two distinct actions have the
       same trigger in a State trigger listing */
    uint8_t u8TriggerCount = APP_USEREG_TRIGGER_COUNT(strUseRegStateMachine);
    uint8_t u8Index = 0;
    while(u8Index < u8TriggerCount)
    {
        if(u32Trigger == (strUseRegStateMachine + u8Index)->u32Trigger)
        {
            /* Invoke associated action and exit loop */
            (strUseRegStateMachine + u8Index)->pfAction(pvData);
            break;
        }
        u8Index++;
    }
}

static void vidUserPasswordUpdated(void *pvArg)
{
    /* Send notification to peer */
    uint8_t u8NotificationBuffer[] = "Registered Password";
    uint16_t u16NotificationSize = sizeof(u8NotificationBuffer)-1;
    (void)enuTransferNotification(Ble_Registration,
                                  u8NotificationBuffer,
                                  &u16NotificationSize);
    /* Toggle user signed-in flag */
    bUseSignedIn = true;

    /* Notify attribution application.
       Note: Data must be preserved until the Attribution application
       receives and processes it. */
    Nvm_tstrRecordDispatch *pstrRecordDispatch = (Nvm_tstrRecordDispatch *)malloc(sizeof(Nvm_tstrRecordDispatch));

    /* Successfully allocated memory for data pointer */
    if(pstrRecordDispatch)
    {
        pstrRecordDispatch->pstrRecordDesc = (fds_record_desc_t *)malloc(sizeof(fds_record_desc_t));

        if(NULL == pstrRecordDispatch->pstrRecordDesc)
        {
            free(pstrRecordDispatch);
        }
        else
        {
            pstrRecordDispatch->pstrRecord = (Nvm_tstrRecord *)malloc(sizeof(Nvm_tstrRecord));

            if(NULL == pstrRecordDispatch->pstrRecord)
            {
                free(pstrRecordDispatch->pstrRecordDesc);
            }
        }

        /* Copy data into dispatchable structure */
        memcpy((void *)pstrRecordDispatch->pstrRecordDesc,
               (void *)&strActiveRecordDesc,
               sizeof(strActiveRecordDesc));
        memcpy((void *)pstrRecordDispatch->pstrRecord,
               (void *)&strActiveRecord,
        sizeof(strActiveRecord));

        /* Dispatch structure to the Attribution application */
        (void)AppMgr_enuDispatchEvent(BLE_USEREG_USER_SIGNED_IN,
                                      (void *)pstrRecordDispatch);
    }
}

static void vidUseRegTaskFunction(void *pvArg)
{
    uint32_t u32Event;
    void *pvData;

    /* Initialize invalid and active user password arrays.
       Note: Passwords are handled as strings and should therefore only contain characters with
       ASCII values in the ASCII range (0 - 0x7F). We use 0xFF to initialize these arrays as this
       value cannot be mistaken for a printable character since it's not an ASCII value. */
    memset(u8InvalidPasswordBase, 0xFF, APP_USEREG_MIN_PASSWORD_LENGTH);
    memset(u8CurrentUserPwd, 0xFF, APP_USEREG_MAX_PASSWORD_LENGTH);

    /* User Registration task's main polling loop */
    while(1)
    {
        /* Task will remain blocked until an event is set in event group */
        u32Event = xEventGroupWaitBits(pvUseRegEventGroupHandle,
                                       APP_USEREG_EVENT_MASK,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
        if(u32Event)
        {
            /* Check whether queue holds any data */
            if(uxQueueMessagesWaiting(pvUseRegQueueHandle))
            {
                /* We have no tasks of higher priority so we're guaranteed that no other
                   message will be received in the queue until this message is processed */
                xQueueReceive(pvUseRegQueueHandle, &pvData, APP_USEREG_POP_IMMEDIATELY);
            }

            /* Process received event */
            vidRegistrationEvent_Process(u32Event, pvData);
        }
    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
App_tenuStatus enuRegistration_Init(void)
{
    App_tenuStatus enuRetVal = Application_Failure;

    /* Create task for User Registration application */
    if(pdPASS == xTaskCreate(vidUseRegTaskFunction,
                             "APP_UseReg_Task",
                             APP_USEREG_TASK_STACK_SIZE,
                             NULL,
                             APP_USEREG_TASK_PRIORITY,
                             &pvUseRegTaskHandle))
    {
        /* Create message queue for User Registration application */
        pvUseRegQueueHandle = xQueueCreate(APP_USEREG_QUEUE_LENGTH, sizeof(void *));

        if(pvUseRegQueueHandle)
        {
            /* Create event group for User Registration application */
            pvUseRegEventGroupHandle = xEventGroupCreate();
            enuRetVal = (NULL != pvUseRegEventGroupHandle)?Application_Success:Application_Failure;
        }
    }

    return enuRetVal;
}

App_tenuStatus enuRegistration_GetNotified(uint32_t u32Event, void *pvData)
{
    App_tenuStatus enuRetVal = Application_Success;

    if(pvData)
    {
        /* Push event-related data to local message queue */
        enuRetVal = (pdTRUE == xQueueSend(pvUseRegQueueHandle,
                                          &pvData,
                                          APP_USEREG_PUSH_IMMEDIATELY))
                                          ?Application_Success
                                          :Application_Failure;
        if(Application_Failure == enuRetVal)
        {
            /* Free allocated memory */
            free((void *)((Ble_tstrRxData *)pvData)->pu8Data);
            free((void *)(Ble_tstrRxData *)pvData);
        }
    }

    if(Application_Success == enuRetVal)
    {
        /* Unblock Registration task by setting event in local event group */
        enuRetVal = xEventGroupSetBits(pvUseRegEventGroupHandle, u32Event)
                                       ?Application_Success
                                       :Application_Failure;
    }

    return enuRetVal;
}