#ifndef COASTLINE_LAUNCHER_INSTALLER_H
#define COASTLINE_LAUNCHER_INSTALLER_H

#include <stdbool.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress Boot: 0x004010a0 */
BOOL EnsureGameNotRunning(void);

bool ProbeInstallerCd(int drive);
DWORD RunInstallerProcess(const char *directory, const char *application, char *commandLine);

/** @ghidraAddress Boot: 0x00421d98 */
extern char *g_pszInstallDir;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INSTALLER_H */
