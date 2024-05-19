/* -----------------------------   Time utilities for nRF52832   ------------------------------- */
/*  File      -  Time utilities source file                                                      */
/*  target    -  nRF52832                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  May, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

/***************************************   INCLUDES   ********************************************/
#include "Time.h"

/******************************************   DEFINES   ******************************************/
#define INVALID_RETURN          0U
#define NOON_HOUR               12U
#define MONTHS_IN_YEAR          12U
#define MONTH_REALIGN_COEFF     14U
#define HOURS_IN_DAY            24U
#define SECONDS_IN_MINUTE       60U
#define REGULAR_YEAR_DAY_COUNT  365U
#define LEAP_YEAR_DAY_COUNT     366U
#define ROUNDED_YEAR_DAY_COUNT  367U
#define SECS_IN_HOUR            3600U
#define TRANSLATED_JULIAN_START 4800U
#define TRANSLATED_JULIAN_LEAP  4900U
#define EXTRA_DAYS              32075U
#define DAY_SECONDS_COUNT       86400U
#define JD_JAN_FIRST_1970       2440588U

/************************************   PUBLIC FUNCTIONS   ***************************************/
uint32_t u32TimeToEpoch(exact_time_256_t *pstrTime)
{
    uint32_t u32RetVal = INVALID_RETURN;

    /* Make sure valid arguments are passed */
    if(pstrTime)
    {
        /* Notes:
            - The Julian calendar is used as it requires less computation to compare two dates
           in the form of integers than structures.
            - Years are aligned to start from the month of March as part of a fully incorporated
           effort to work around dealing with leap day calculations.
            - Although the current Julian period started on Monday, January 1st 4713BC, we compute
           the number of elapsed years since the year 4801BC to simplify computations. The extra 88
           years are accounted for and haven't been overlooked.
            - The leap year rule in both the Julian and Gregorian calendars states that year
           numbers divisible by 4 are leap years, except those divisible by 100 and not by 400. */

        uint8_t u8Mrc;
        /* Compute the month aligning coefficient. Note: In the original algorithm, this
           coefficient is used as (Month-14)/12. It's supposed to yield -1 for the monthes of
           January and February and 0 for the rest of the months. */
        u8Mrc = (MONTH_REALIGN_COEFF - pstrTime->day_date_time.date_time.month) / MONTHS_IN_YEAR;

        /* Compute the number of days since March 1st 4801 BC. This won't account for
           irregularities such as the leap year rule. Those will be taken into account later. This
           Works by rounding up years in groups of 4 and comuting the number of days in a 4
           consecutive-year cycle. The number of days in the current year will also be accounted
           for later. */
        u32RetVal += ((3 * REGULAR_YEAR_DAY_COUNT + LEAP_YEAR_DAY_COUNT) *
                     (pstrTime->day_date_time.date_time.year + TRANSLATED_JULIAN_START - u8Mrc))
                     / 4;

        /* Compute the number of days in the current Julian year. This rounds up days of all months
           in regular as well as leap years. This means February 29th is accounted for to reach
           a correct average yearly day-count. Extra days are added here just to simplify
           computations, they will eventually be taken away. */
        u32RetVal += ROUNDED_YEAR_DAY_COUNT *
                     (pstrTime->day_date_time.date_time.month - 2 + MONTHS_IN_YEAR * u8Mrc)
                     / MONTHS_IN_YEAR;

        /* Incorporate leap year rule */
        u32RetVal -= (((pstrTime->day_date_time.date_time.year + TRANSLATED_JULIAN_LEAP - u8Mrc)
                     / 100) * 3) / 4;

        /* Add day-count in current month */
        u32RetVal += pstrTime->day_date_time.date_time.day;

        /* readjust to account for extra days added to simplify computations */
        u32RetVal -= EXTRA_DAYS;

        /* Compute number of Julian days since January 1st 1970 and convert to seconds */
        u32RetVal -= JD_JAN_FIRST_1970;
        u32RetVal *= DAY_SECONDS_COUNT;

        uint8_t u8HourUtc;
        /* The Julian reference starts at exactly noon UTC. Convert hours to UTC.
           Note: WiPad was first deployed in the UTC+1 time zone. Time zone variations with
           regard to UTC are configurable in system_config.h */
        u8HourUtc = (pstrTime->day_date_time.date_time.hours
                    - ((UTIL_UTC_TIME_ZONE<=12)*UTIL_UTC_TIME_ZONE)+HOURS_IN_DAY
                    + ((UTIL_UTC_TIME_ZONE>12)*(HOURS_IN_DAY-UTIL_UTC_TIME_ZONE)))%HOURS_IN_DAY;

        /* Account for hours in current day */
        u32RetVal += u8HourUtc*SECS_IN_HOUR;

        /* Add minutes and seconds in current hour */
        u32RetVal += pstrTime->day_date_time.date_time.minutes * SECONDS_IN_MINUTE
                     + pstrTime->day_date_time.date_time.seconds;
    }

    return u32RetVal;
}