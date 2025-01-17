/* ------------------------------   Display app for nRF52832   --------------------------------- */
/*  File      -  Display application header file                                                 */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  March, 2024                                                                     */
/* --------------------------------------------------------------------------------------------- */

#ifndef _APP_DISPLAY_H_
#define _APP_DISPLAY_H_

/****************************************   INCLUDES   *******************************************/
#include "App_Types.h"
#include "system_config.h"

/*************************************   PUBLIC DEFINES   ****************************************/
/* Event bits */
#define APP_DISPLAY_ADVERTISING_START         (1 << 0) /* Advertising started                    */
#define APP_DISPLAY_PEER_CONNECTION           (1 << 1) /* Connection with peer established       */
#define APP_DISPLAY_PEER_DISCONNECTION        (1 << 2) /* Disconnected from peer                 */
#define APP_DISPLAY_VALID_USER_INPUT          (1 << 3) /* Received valid user input              */
#define APP_DISPLAY_INVALID_USER_INPUT        (1 << 4) /* Received invalid user input            */
#define APP_DISPLAY_ACCESS_GRANTED            (1 << 5) /* Access granted                         */
#define APP_DISPLAY_ACCESS_DENIED             (1 << 6) /* Access denied                          */
#define APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP   (1 << 7) /* Admin successfully added a new user    */
#define APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP (1 << 8) /* Admin successfully requested user data */
#define APP_DISPLAY_DISABLED_NOTIFICATIONS    (1 << 9) /* Received data with notifs disabled     */

/* Event ranks */
#define APP_DISPLAY_ADVERTISING_START_RANK         1U  /* Advertising started display pattern    */
#define APP_DISPLAY_PEER_CONNECTION_RANK           2U  /* Connection established display pattern */
#define APP_DISPLAY_PEER_DISCONNECTION_RANK        3U  /* Disconnected from peer display pattern */
#define APP_DISPLAY_VALID_USER_INPUT_RANK          4U  /* Received valid input display pattern   */
#define APP_DISPLAY_INVALID_USER_INPUT_RANK        5U  /* Received invalid input display pattern */
#define APP_DISPLAY_ACCESS_GRANTED_RANK            6U  /* Access granted display pattern         */
#define APP_DISPLAY_ACCESS_DENIED_RANK             7U  /* Access denied display pattern          */
#define APP_DISPLAY_ADMIN_SUCCESSFUL_ADD_OP_RANK   8U  /* New user added display pattern         */
#define APP_DISPLAY_ADMIN_SUCCESSFUL_CHECK_OP_RANK 9U  /* User data requested display pattern    */
#define APP_DISPLAY_DISABLED_NOTIFICATIONS_RANK    10U /* Notifications disabled display pattern */

/**************************************   PUBLIC TYPES   *****************************************/
/**
 * DisplayAction State machine event-triggered action function prototype.
 *
 * @note This prototype is used to define state machine actions associated to different state
 *       triggers. A state action is invoked upon receiving its trigger.
 *
 * @note Functions of this type take no arguments.
 */
typedef void (*DisplayAction)(void);

/**
 * DisplayLedPattern Led pattern-arranging function prototype.
 *
 * @note This prototype is used to define our LED pattern-arranging functions.
 *
 * @note Functions of this type take one parameter:
 *         - uint8_t u8Index: LED rank in pattern.
*/
typedef uint32_t (*DisplayLedPattern)(uint8_t u8Index);

/**
 * Display_tstrState State-defining structure for the Display application.
*/
typedef struct
{
    uint32_t u32Trigger;
    DisplayAction pfAction;
}Display_tstrState;

/**
 * Display_tstrLedPattern structure associating event to Led arrangement pattern.
*/
typedef struct
{
    uint32_t u32Event;
    DisplayLedPattern pfArrangement;
}Display_tstrLedPattern;

/*********************************   PRIVATE INLINE FUNCTIONS   **********************************/
/**
 * @brief u32LedPattern1342 Returns the u8Index-ranked LED pin index in the 1,3,4,2 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,3,4,2 pattern.
*/
static inline uint32_t u32LedPattern1342(uint8_t u8Index)
{
    return (u8Index+(u8Index%LED_1)-(u8Index/LED_3)-(4*(u8Index/LED_4)));
}

/**
 * @brief u32LedPattern1324 Returns the u8Index-ranked LED pin index in the 1,3,2,4 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,3,2,4 pattern.
*/
static inline uint32_t u32LedPattern1324(uint8_t u8Index)
{
    return (u8Index+(u8Index/LED_2)-2*(u8Index/LED_3)+(u8Index/LED_4));
}

/**
 * @brief u32LedPattern4231 Returns the u8Index-ranked LED pin index in the 4,2,3,1 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 4,2,3,1 pattern.
*/
static inline uint32_t u32LedPattern4231(uint8_t u8Index)
{
    return (u8Index-6*(u8Index/LED_4)+3*(u8Index%(LED_1-1))-6*(u8Index/LED_2)-3*(u8Index/LED_3));
}

/**
 * @brief u32LedPattern2341 Returns the u8Index-ranked LED pin index in the 2,3,4,1 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 2,3,4,1 pattern.
*/
static inline uint32_t u32LedPattern2341(uint8_t u8Index)
{
    return (((u8Index+1)%(LED_4+1))+(u8Index/LED_4)*LED_1);
}

/**
 * @brief u32LedPattern1432 Returns the u8Index-ranked LED pin index in the 1,4,3,2 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,4,3,2 pattern.
*/
static inline uint32_t u32LedPattern1432(uint8_t u8Index)
{
    return (u8Index+(2*((u8Index+1)%2))-(4*(u8Index/LED_4)));
}

/**
 * @brief u32LedPattern1243 Returns the u8Index-ranked LED pin index in the 1,2,4,3 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,2,4,3 pattern.
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
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,4,2,3 pattern.
*/
static inline uint32_t u32LedPattern1423(uint8_t u8Index)
{
    return (u8Index+(u8Index/LED_2)*(2+(u8Index%LED_2))-4*(u8Index/LED_3)-(u8Index/LED_4));
}

/**
 * @brief u32LedPattern1234 Returns the u8Index-ranked LED pin index in the 1,2,3,4 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 1,2,3,4 pattern.
*/
static inline uint32_t u32LedPattern1234(uint8_t u8Index)
{
    return u8Index;
}

/**
 * @brief u32LedPattern4321 Returns the u8Index-ranked LED pin index in the 4,3,2,1 LED pattern.
 *
 * @note Our current implementation does not guarantee the safety of this function's output if
 *       it takes any value outside the range of LED indexes as input. Therefore, this function
 *       should be invoked with caution as it has no built-in assertion mechanism to ensure the
 *       validity of its input.
 *
 * @param u8Index LED rank in the pattern.
 *
 * @return uint32_t Pin index of u8Index-ranked LED in the 4,3,2,1 pattern.
*/
static inline uint32_t u32LedPattern4321(uint8_t u8Index)
{
    return (LED_3+(LED_2-u8Index));
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
 *         Application_Failure otherwise.
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
 * @param u32Event Event to be posted in local event group.
 * @param pvData Unused parameter.
 *
 * @return App_tenuStatus Application_Success if notification was posted successfully,
 *         Application_Failure otherwise.
 */
App_tenuStatus enuDisplay_GetNotified(uint32_t u32Event, void *pvData);

#endif /* _APP_DISPLAY_H_ */