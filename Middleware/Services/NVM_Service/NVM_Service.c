/* -----------------------------   NVM Service for nRF52832   ---------------------------------- */
/*  File      -  NVM Service source file                                                         */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "NVM_Service.h"
#include "fds.h"

/************************************   PRIVATE FUNCTIONS   **************************************/
static void vidNvmEventHandler(fds_evt_t const *pstrEvent)
{
    /* Make sure valid arguments are passed */
    if(pstrEvent)
    {

    }
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
Mid_tenuStatus enuNvm_Init(void)
{
    Mid_tenuStatus enuRetVal = Middleware_Failure;

    /* Register a Flash Data Storage event handler to receive FDS events */
    if(NRF_SUCCESS == fds_register(vidNvmEventHandler))
    {
        /* Initialize the FDS file system module */
        enuRetVal = (NRF_SUCCESS == fds_init())?Middleware_Success:Middleware_Failure;
    }

    return enuRetVal;
}