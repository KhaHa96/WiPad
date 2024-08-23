/* ------------------------------   Display app for nRF51422   --------------------------------- */
/*  File      -  Display application header file                                                 */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_DISPLAY_H_
#define _APP_DISPLAY_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
#define APP_DISPLAY_ID_VERIF_SUCCESS (1 << 0)
#define APP_DISPLAY_ID_VERIF_FAILURE (1 << 1)

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * State machine event-triggered action function pointer
 */
typedef void (*DisplayAction)(void);

/**
 * Led pattern-arranging function pointer
*/
typedef uint32_t (*DisplayLedPattern)(uint8_t u8Index);

/**
 * Display_tstrState State-defining structure for the Display application
*/
typedef struct
{
    uint32_t u32Trigger;
    DisplayAction pfAction;
}Display_tstrState;

/**
 * Display_tstrLedPattern structure associating event to Led arrangement pattern
*/
typedef struct
{
    uint32_t u32Event;
    DisplayLedPattern pfArrangement;
}Display_tstrLedPattern;

/*********************************   PRIVATE INLINE FUNCTIONS   **********************************/
/**
 * @brief u32LedPattern1243 Returns the u8Index-ranked LED pin index in the 1,2,4,3 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,2,4,3 pattern
*/
static inline uint32_t u32LedPattern1243(uint8_t u8Index)
{
    return (u8Index+(u8Index/LED_3)*(1+(u8Index%LED_3))-3*(u8Index/LED_4));
}

/**
 * @brief u32LedPattern1423 Returns the u8Index-ranked LED pin index in the 1,4,2,3 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,4,2,3 pattern
*/
static inline uint32_t u32LedPattern1423(uint8_t u8Index)
{
    return (u8Index+(u8Index/LED_2)*(2+(u8Index%LED_2))-4*(u8Index/LED_3)-(u8Index/LED_4));
}

/************************************   PUBLIC FUNCTIONS   ***************************************/
/**
 * @brief enuDisplay_Init Creates Display task and event group to receive notifications
 *        from other tasks.
 *
 * @note This function is invoked by the Application Manager.
 *
 * @pre This function requires no prerequisites.
 *
 * @return App_tenuStatus Application_Success if initialization was performed successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuDisplay_Init(void);

/**
 * @brief enuDisplay_GetNotified Notifies Display task of an incoming event by
 *        setting it in local event group.
 *
 * @note This function is invoked by the Application Manager signaling that another task
 *       wants to communicate with the Display task.
 *
 * @pre This function can't be called unless Display task is initialized and running.
 *
 * @param u32Event Event to be posted in local event group
 * @param pvData Unused parameter
 *
 * @return App_tenuStatus Application_Success if notification was posted successfully,
 *         Application_Failure otherwise
 */
App_tenuStatus enuDisplay_GetNotified(uint32_t u32Event, void *pvData);

#endif /* _APP_DISPLAY_H_ */