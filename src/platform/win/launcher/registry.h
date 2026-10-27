#ifndef COASTLINE_LAUNCHER_REGISTRY_H
#define COASTLINE_LAUNCHER_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The installation directory, ending with a backslash after it is read.
 * @ghidraAddress BRally: 0x004032a8
 * @ghidraAddress SetVideo: 0x0040e090
 */
extern char g_szInstallDir[260];

/**
 * Read the machine installation directory, falling back to the root of drive C.
 * @ghidraAddress BRally: 0x004016d0
 * @ghidraAddress SetVideo: 0x00401b30
 */
void ReadInstallDirectoryFromRegistry(void);

#ifdef __cplusplus
}
#endif

#endif
