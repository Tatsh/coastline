#ifndef COASTLINE_LAUNCHER_INSTALLER_H
#define COASTLINE_LAUNCHER_INSTALLER_H

#include <stdbool.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bring an existing Boss Rally window to the foreground if the game is already running.
 * @return False when a game window was found; true otherwise.
 * @ghidraAddress Boot: 0x004010a0 */
BOOL EnsureGameNotRunning(void);

/** @return Whether @p drive has the Boss Rally installation disc. */
bool ProbeInstallerCd(int drive);
/** Start @p application in @p directory and wait for it to exit. */
DWORD RunInstallerProcess(const char *directory, const char *application, char *commandLine);

/** The installation directory read from the Boss Rally registry entry.
 * @ghidraAddress Boot: 0x00421d98 */
extern char *g_pszInstallDir;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INSTALLER_H */
