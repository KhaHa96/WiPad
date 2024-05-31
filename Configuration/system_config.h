/* --------------------------- System configuration for nRF52832 ------------------------------- */
/*  File      -  Configuration header file for system-level defines                              */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

/************************************   APPLICATION DEFINES   ************************************/
#define APPLICATION_COUNT 3

/* User Registration application */
#define APP_USEREG_TASK_STACK_SIZE 256
#define APP_USEREG_TASK_PRIORITY 2
#define APP_USEREG_QUEUE_LENGTH 5
#define APP_USEREG_MAX_CROSS_IDS 3

/* Key Attribution application */
#define APP_KEYATT_TASK_STACK_SIZE 256
#define APP_KEYATT_TASK_PRIORITY 2
#define APP_KEYATT_QUEUE_LENGTH 5

/* Display application */
#define APP_DISPLAY_TASK_STACK_SIZE 256
#define APP_DISPLAY_TASK_PRIORITY 2
#define APP_DISPLAY_QUEUE_LENGTH 5

/*************************************   MIDDLEWARE DEFINES   ************************************/
/* Ble Middleware Service */
#define MID_BLE_TASK_STACK_SIZE 1024
#define MID_BLE_TASK_PRIORITY 2
#define MID_BLE_TASK_QUEUE_LENGTH 5

/***************************************   UTILITY DEFINES   *************************************/
/* Time utility. Define UTC+n as n and UTC-n as 24-n */
#define UTIL_UTC_TIME_ZONE 1

/*************************************   PERIPHERAL DEFINES   ************************************/
/* LEDS */
#define LED_1 17
#define LED_2 18
#define LED_3 19
#define LED_4 20

#endif /* _SYS_CONFIG_H_ */