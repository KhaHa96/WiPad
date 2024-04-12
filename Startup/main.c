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
#include "timers.h"
#include "nrf_drv_clock.h"
#include "BLE_Service.h"
#include "AppMgr.h"

/************************************   PRIVATE VARIABLES   **************************************/
static TimerHandle_t pvTimerHandle;

/************************************   PRIVATE FUNCTIONS   **************************************/
void vApplicationIdleHook( void )
{

}

static void vidTimerCallback( TimerHandle_t xTimer )
{
    __NOP();
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

    /* Create timer */
    pvTimerHandle = xTimerCreate("Timer0", pdMS_TO_TICKS(1000), pdTRUE, NULL, vidTimerCallback);

    /* Start Timer */
    BaseType_t lErrorCode = xTimerStart(pvTimerHandle, 0);

    /* Start scheduler */
    vTaskStartScheduler();

    /* This loop shouldn't be reached */
    while(1)
    {

    }
}