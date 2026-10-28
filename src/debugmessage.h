#ifndef COASTLINE_DEBUGMESSAGE_H
#define COASTLINE_DEBUGMESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show a message on the debug display, or the fatal error screen on Windows.
 *
 * @param message The message to show.
 * @ghidraAddress USA: 0x8021e1f4
 * @ghidraAddress EUR: 0x8021f3f4
 * @ghidraAddress JPN: 0x8021ecc4
 * @ghidraAddress BRD3D: 0x10035bba
 * @ghidraAddress BRGlide: 0x1002f26b
 */
void SetDebugMessage(const char *message);

/**
 * The message the debug display shows.
 *
 * @ghidraAddress USA: 0x8028a88c
 * @ghidraAddress EUR: 0x8028ca90
 * @ghidraAddress JPN: 0x8028c560
 * @ghidraAddress BRD3D: 0x106c2cf0
 */
extern const char *g_pszDebugMessage;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_DEBUGMESSAGE_H */
