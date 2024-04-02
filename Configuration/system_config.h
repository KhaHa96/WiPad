/* --------------------------- System configuration for nRF51422 ------------------------------- */
/*  File      -  Configuration header file for system-level defines                              */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

/************************************   APPLICATION DEFINES   ************************************/
#define APPLICATION_COUNT 3

/* Key Attribution application */
#define APP_KEYATT_TASK_STACK_SIZE 256
#define APP_KEYATT_TASK_PRIORITY 2
#define APP_KEYATT_QUEUE_LENGTH 5

/* Display application */
#define APP_DISPLAY_TASK_STACK_SIZE 256
#define APP_DISPLAY_TASK_PRIORITY 2
#define APP_DISPLAY_QUEUE_LENGTH 5

/* User Registration application */
#define APP_USEREG_TASK_STACK_SIZE 256
#define APP_USEREG_TASK_PRIORITY 2
#define APP_USEREG_QUEUE_LENGTH 5

/*************************************   MIDDLEWARE DEFINES   ************************************/
/* Ble Middleware Service */
#define MID_BLE_TASK_STACK_SIZE 256
#define MID_BLE_TASK_PRIORITY 2

/*************************************   PERIPHERAL DEFINES   ************************************/
/* SPI */
#define SPI_MOSI_PIN 2
#define SPI_MISO_PIN 3
#define SPI_CS_PIN 4
#define SPI_CLOCK_PIN 1

/* LEDS */
#define LED_1 21
#define LED_2 22
#define LED_3 23
#define LED_4 24

#endif /* _SYS_CONFIG_H_ */