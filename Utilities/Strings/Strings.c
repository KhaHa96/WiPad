/* -----------------------------   NVM Service for nRF51422   ---------------------------------- */
/*  File      -  NVM Service source file                                                         */
/*  target    -  nRF51422                                                                        */
/*  toolchain -  IAR                                                                             */
/*  created   -  July, 2024                                                                       */
/* --------------------------------------------------------------------------------------------- */

/****************************************   INCLUDES   *******************************************/
#include "Strings.h"

/************************************   PRIVATE FUNCTIONS   **************************************/
int8_t s8StringCompare(const uint8_t *pu8String1, const uint8_t *pu8String2, uint8_t u8Length)
{
    int8_t s8RetVal = 2;

    /* Make sure valid arguments are passed */
    if (pu8String1 && pu8String2 && (u8Length > 0))
    {
        /* We cast both our string pointers to unsigned char for clarity purposes. Unsigned char
           and uint8_t are essentially the same and in most cases are interchangeable, but since
           we're dealing with strings (groups of characters), this cast will dispel any doubt. */
        const unsigned char *pchString1 = (const unsigned char *)pu8String1;
        const unsigned char *pchString2 = (const unsigned char *)pu8String2;

        /* Go through both strings comparing them character by character.
           Note: If two strings are passed as arguments, it's safe to assume that
           pchString1 and pchString2 won't ever be NULL pointers as long as the NULL string
           terminator '\0' hasn't showed up in either string yet. Despite that being the case,
           we have added a precautionary assert condition to make sure both pointers aren't NULL
           in every loop iteration before dereferencing them, in case bad arguments were input into
           the function. */
        while ((pchString1 && pchString2) && *pchString1 && (*pchString1 == *pchString2) && (--u8Length))
        {
            /* Increment both pointers to compare next set of characters in the next iteration */
            ++pchString1;
            ++pchString2;
        }

        s8RetVal = (*pchString1 > *pchString2) - (*pchString2 > *pchString1);
    }

    return s8RetVal;
}

bool bContainsNumeral(const uint8_t *pu8String, uint8_t u8Length)
{
    bool bRetVal = false;

    /* Make sure valid arguments are passed */
    if (pu8String && (u8Length > 0))
    {
        /* We cast our string pointer to unsigned char for clarity purposes. Unsigned char
           and uint8_t are essentially the same and in most cases are interchangeable, but since
           we're dealing with a string (a group of characters), this cast will dispel any doubt. */
        const unsigned char *pchString = (const unsigned char *)pu8String;

        /* Go through the string's characters and find the first numeric character if any.
           Note: The non-NULL pointer assert is unnecessary and has been added as a precautionary
           measure in case this function is fed a bad string */
        while (pchString && *pchString && u8Length)
        {
            if ((*pchString >= '0') && (*pchString <= '9'))
            {
                /* Numeric character found. No need to keep going through string */
                bRetVal = true;
                break;
            }

            /* Increment pointer to check next character in the next iteration and decrement string
               length */
            pchString++;
            u8Length--;
        }
    }

    return bRetVal;
}

bool bIsAllNumerals(const uint8_t *pu8String, uint8_t u8Length)
{
    bool bRetVal = true;

    /* Make sure valid arguments are passed */
    if (pu8String && (u8Length > 0))
    {
        /* We cast our string pointer to unsigned char for clarity purposes. Unsigned char
           and uint8_t are essentially the same and in most cases are interchangeable, but since
           we're dealing with a string (a group of characters), this cast will dispel any doubt. */
        const unsigned char *pchString = (const unsigned char *)pu8String;

        /* Go through the string's characters and check whether they're numerals.
           Note: The non-NULL pointer assert is unnecessary and has been added as a precautionary
           measure in case this function is fed a bad string */
        while (pchString && *pchString && u8Length)
        {
            if ((*pchString < '0') || (*pchString > '9'))
            {
                /* Non-numeral character found. No need to keep going through string */
                bRetVal = false;
                break;
            }

            /* Increment pointer to check next character in the next iteration and decrement string
               length */
            pchString++;
            u8Length--;
        }
    }

    return bRetVal;
}

bool bIsAllLowCaseAlpha(const uint8_t *pu8String, uint8_t u8Length)
{
    bool bRetVal = true;

    /* Make sure valid arguments are passed */
    if (pu8String && (u8Length > 0))
    {
        /* We cast our string pointer to unsigned char for clarity purposes. Unsigned char
           and uint8_t are essentially the same and in most cases are interchangeable, but since
           we're dealing with a string (a group of characters), this cast will dispel any doubt. */
        const unsigned char *pchString = (const unsigned char *)pu8String;

        /* Go through the string's characters and check whether they're low case alphabeticals.
           Note: The non-NULL pointer assert is unnecessary and has been added as a precautionary
           measure in case this function is fed a bad string */
        while (pchString && *pchString && u8Length)
        {
            if ((*pchString < 'a') || (*pchString > 'z'))
            {
                /* Non-alphabetical character found. No need to keep going through string */
                bRetVal = false;
                break;
            }

            /* Increment pointer to check next character in the next iteration and decrement string
               length*/
            pchString++;
            u8Length--;
        }
    }

    return bRetVal;
}

bool bContainsSpecialChar(const uint8_t *pu8String, uint8_t u8Length)
{
    bool bRetVal = false;

    /* Make sure valid arguments are passed */
    if (pu8String && (u8Length > 0))
    {
        /* We cast our string pointer to unsigned char for clarity purposes. Unsigned char
           and uint8_t are essentially the same and in most cases are interchangeable, but since
           we're dealing with a string (a group of characters), this cast will dispel any doubt. */
        const unsigned char *pchString = (const unsigned char *)pu8String;

        /* Go through the string's characters and find the first special character if any.
           Note: The non-NULL pointer assert is unnecessary and has been added as a precautionary
           measure in case this function is fed a bad string */
        while (pchString && *pchString && u8Length)
        {
            if (((*pchString < '0') || (*pchString > '9')) &&
                ((*pchString < 'a') || (*pchString > 'z')) &&
                ((*pchString < 'A') || (*pchString > 'Z')))
            {
                /* Special character found. No need to keep going through string */
                bRetVal = true;
                break;
            }

            /* Increment pointer to check next character in the next iteration and decrement string
               length */
            pchString++;
            u8Length--;
        }
    }

    return bRetVal;
}