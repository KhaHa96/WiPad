/* ------------------------------   Entry point for nRF52832   --------------------------------- */
/*  File      -  Application entry point source file                                             */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nrf_drv_clock.h"
#include "BLE_Service.h"
#include "AppMgr.h"

/************************************   PRIVATE FUNCTIONS   **************************************/
void vApplicationIdleHook( void )
{

}

/**************************************   MAIN FUNCTION   ****************************************/
int main(void)
{
    /* Initialize clocks and prepare them for requests */
    nrf_drv_clock_init();

    /* Initialize application tasks */
    App_tenuStatus enuAppStatus = AppMgr_enuInit();

    /* Initialize Ble stack task */
    Mid_tenuStatus enuMidStatus = enuBle_Init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* This loop shouldn't be reached */
    while(1)
    {

    }
}