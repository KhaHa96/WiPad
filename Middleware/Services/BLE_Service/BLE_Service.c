#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "BLE_Service.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "nrf_ble_gatt.h"

static TaskHandle_t pvBLETaskHandle;
static EventGroupHandle_t pvBLEEventGroupHandle;

#define WIPAD_MOCK_EVENT (1 << 0)

static void vidBLE_Task_Function(void *pvArg)
{
    uint32_t u32Event;

    /* Task function's main busy loop */
    while (1)
    {
        /* Pull for synch events set in event group */
        u32Event = xEventGroupWaitBits(pvBLEEventGroupHandle, WIPAD_MOCK_EVENT, pdTRUE, pdTRUE, portMAX_DELAY);

        if (u32Event)
        {
            __NOP();
        }
    }
}

Mid_tenuStatus enuBle_Init(void)
{
    /* Create task for BLE service */
    xTaskCreate(vidBLE_Task_Function, "Task0", 256, NULL, 2, &pvBLETaskHandle);
    pvBLEEventGroupHandle = xEventGroupCreate();

    return (pvBLEEventGroupHandle)?Middleware_Success:Middleware_Failure;
}