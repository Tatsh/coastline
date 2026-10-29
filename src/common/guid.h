#pragma once

/**
 * @file
 * The globally unique identifier the settings of the game save, laid out as the GUID of Windows.
 * The guard is the one the headers of Windows test, so a source that includes both has one
 * definition.
 */

#include <stdint.h>

#ifndef GUID_DEFINED
#define GUID_DEFINED
/** A globally unique identifier. */
typedef struct _GUID {
    uint32_t Data1;   /*!< The first 32 bits. */
    uint16_t Data2;   /*!< The next 16 bits. */
    uint16_t Data3;   /*!< The next 16 bits. */
    uint8_t Data4[8]; /*!< The last 64 bits. */
} GUID;
#endif /* GUID_DEFINED */
