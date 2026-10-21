#pragma once

#include <windows.h>

/**
 * Settings read from BossRally.cfg. The layout is not recovered yet. The first member is the
 * keyboard binding table.
 */
class RallyConfig {
public:
    /**
     * Read the settings from a configuration file.
     *
     * @param pszPath Path of the file.
     * @ghidraAddress BRD3D: 0x10069ff0
     * @ghidraAddress BRGlide: 0x10063060
     */
    void LoadRallyConfigFile(const char *pszPath);
};

/**
 * Settings of the running game.
 *
 * @ghidraAddress BRD3D: 0x10b4df30
 * @ghidraAddress BRGlide: 0x10b71290
 */
extern RallyConfig g_GlobalRallyConfig;

/**
 * Read BossRally.ini and apply the command line on top of it.
 *
 * @param pszCmdLine Command line the launcher received.
 * @ghidraAddress BRD3D: 0x10007bd0
 * @ghidraAddress BRGlide: 0x10007f40
 */
void LoadBossRallyIniSettings(LPSTR pszCmdLine);
