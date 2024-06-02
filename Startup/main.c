/* ------------------------------   Entry point for nRF52832   --------------------------------- */
/*  File      -  Application entry point source file                                             */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "nrf_drv_clock.h"
#include "bsp.h"
#include "AppMgr.h"
#include "BLE_Service.h"
#include "NVM_Service.h"

/************************************   PRIVATE FUNCTIONS   **************************************/
void vApplicationIdleHook( void )
{

}

/**************************************   MAIN FUNCTION   ****************************************/
int main(void)
{
    /* Initialize clocks and prepare them for requests */
    nrf_drv_clock_init();

    /* Initialize buttons */
    bsp_init(BSP_INIT_BUTTONS, NULL);

    /* Initialize application tasks */
    (void)AppMgr_enuInit();

    /* Initialize Ble stack task */
    (void)enuBle_Init();

    /* Initialize NVM middleware service */
    (void)enuNvm_Init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* This loop shouldn't be reached */
    while(1)
    {

    }
}