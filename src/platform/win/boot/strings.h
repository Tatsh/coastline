#pragma once

#include <cstddef>

/** The number of strings of the language resource library, numbered from 1. */
#define STRING_RESOURCE_COUNT 302

/**
 * The strings of the language resource library by identifier. The first entry is unused.
 *
 * @ghidraAddress BRD3D: 0x11829370
 * @ghidraAddress BRGlide: 0x1186c488
 */
extern const char *g_apszStringResourceTable[STRING_RESOURCE_COUNT + 1];

/**
 * The buffer the strings are loaded into, allocated once.
 *
 * @ghidraAddress BRD3D: 0x1182982c
 */
extern char *g_pStringTableBuffer;

/**
 * The bytes of g_pStringTableBuffer the loaded strings use.
 *
 * @ghidraAddress BRD3D: 0x11829830
 */
extern size_t g_dwStringTableUsedBytes;

/**
 * The size of g_pStringTableBuffer, the size of the library file.
 *
 * @ghidraAddress BRD3D: 0x11829834
 */
extern size_t g_dwStringTableBufferSize;

/**
 * Load the string table of the language resource library. The table is cleared each time, and the
 * strings are loaded only the first time.
 *
 * @ghidraAddress BRD3D: 0x10073f60
 * @ghidraAddress BRGlide: 0x1006d1a0
 */
void LoadStringTableFromDll(void);

/**
 * Look up a string of the loaded string table.
 *
 * @param nResourceId Identifier of the string resource.
 * @return The string, managed by the table, or `NULL` for an identifier out of range.
 * @ghidraAddress BRD3D: 0x10074030
 * @ghidraAddress BRGlide: 0x1006d280
 */
const char *GetStringResource(unsigned nResourceId);

/**
 * Free the buffer of the strings.
 *
 * @ghidraAddress BRD3D: 0x10074050
 */
void FreeStringTable(void);
