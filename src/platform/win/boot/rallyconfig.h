#pragma once

#include <windows.h>

#include "platform/win/input.h"

/** The number of menu selections the configuration saves. */
#define MENU_SELECTION_COUNT 12

/** The settings saved in BossRally.cfg. */
class RallyConfig {
public:
    InputBinding aInputBindingsKeyboard[INPUT_BINDING_COUNT]; /*!< The keyboard bindings. */
    InputBinding aInputBindingsDevice1[INPUT_BINDING_COUNT]; /*!< First game controller bindings. */
    InputBinding aInputBindingsDevice2[INPUT_BINDING_COUNT]; /*!< Second controller bindings. */
    InputBinding aInputBindingsDevice3[INPUT_BINDING_COUNT]; /*!< Third controller bindings. */
    /** The device the game reads, 1 to 3 for a game controller and any other for the keyboard. */
    int dwInputDeviceType;
    InputBinding *pActiveInputBindings;     /*!< The bindings of dwInputDeviceType, not saved. */
    int bCarShadowSelection;                /*!< Whether the shadow of a car is drawn. */
    int bSpecularLightingSelection;         /*!< Whether specular lighting is drawn. */
    int bForceFeedbackEnabled;              /*!< Whether force feedback is on. */
    char szSavedModemPhoneNumber[MAX_PATH]; /*!< The number the modem dials last. */
    char szPlayerName[1024];                /*!< The name of the player. */
    int dwDisplayModeWidth;                 /*!< The width of the display mode. */
    int dwDisplayModeHeight;                /*!< The height of the display mode. */
    int dwDisplayModeBitDepth;              /*!< The bits of a pixel of the display mode. */
    int dwDisplayModeExtra;                 /*!< A fourth value of the display mode. */
    GUID DisplayDriverGuid;                 /*!< The DirectDraw driver, all zero for the default. */
    int dwSfxVolume;                        /*!< The volume of sound effects. */
    int dwMusicVolume;                      /*!< The volume of music. */
    int adwMenuSelectionBackup[MENU_SELECTION_COUNT]; /*!< The last choices of the menus. */
    char szTcpIpPort[32];                             /*!< The TCP/IP port of network play. */
    char szTcpIpAddress[64];                          /*!< The TCP/IP address of network play. */
    int bSkidMarksSelection;                          /*!< Whether skid marks are drawn. */

    /**
     * Set the defaults.
     *
     * @ghidraAddress BRD3D: 0x10069a90
     * @ghidraAddress BRGlide: 0x10062b00
     */
    RallyConfig();

    /** The destructor does nothing, and the linker shares its code with NullStub(). */
    ~RallyConfig() {
    }

    /**
     * Bind a device to its default controls.
     *
     * @param nDevice The device, 1 to 3 for a game controller and any other for the keyboard.
     * @ghidraAddress BRD3D: 0x10069aa0
     * @ghidraAddress BRGlide: 0x10062b10
     */
    void LoadDeviceDefaultBindings(int nDevice);

    /**
     * Bind a control of a device. The alternate controls are taken from the defaults, and an
     * alternate that another binding of the device uses is cleared.
     *
     * @param nDevice The device, 1 to 3 for a game controller and any other for the keyboard.
     * @param nControl The control of the game.
     * @param nKind The kind of device in the high byte.
     * @param nKeyCode The key or button in the low byte.
     * @ghidraAddress BRD3D: 0x10069b10
     * @ghidraAddress BRGlide: 0x10062b80
     */
    void SetInputBinding(int nDevice, int nControl, int nKind, int nKeyCode);

    /**
     * The kind of device of a binding.
     *
     * @param nDevice The device, 1 to 3 for a game controller and any other for the keyboard.
     * @param nControl The control of the game.
     * @return The high byte of the control of the binding, in place.
     * @ghidraAddress BRD3D: 0x10069bc0
     * @ghidraAddress BRGlide: 0x10062c30
     */
    int GetInputBindingFlags(int nDevice, int nControl);

    /**
     * The key or button of a binding. A control of a game controller with its top bit set has
     * the code in its high byte.
     *
     * @param nDevice The device, 1 to 3 for a game controller and any other for the keyboard.
     * @param nControl The control of the game.
     * @return The key or button.
     * @ghidraAddress BRD3D: 0x10069c30
     * @ghidraAddress BRGlide: 0x10062ca0
     */
    int GetInputBindingKeyCode(int nDevice, int nControl);

    /**
     * Set every setting to its default and bind each device to its default controls.
     *
     * @ghidraAddress BRD3D: 0x10069c90
     * @ghidraAddress BRGlide: 0x10062d00
     */
    void InitRallyConfigDefaults();

    /**
     * Copy the settings of another configuration, binding the copy to its own tables.
     *
     * @param other The configuration copied.
     * @return This configuration.
     * @ghidraAddress BRD3D: 0x10069de0
     * @ghidraAddress BRGlide: 0x10062e50
     */
    RallyConfig &CopyRallyConfig(const RallyConfig &other);

    /**
     * Read the settings from a configuration file of version 2. A file that does not read whole
     * results in the defaults.
     *
     * @param pszPath Path of the file.
     * @return Whether the file was read.
     * @ghidraAddress BRD3D: 0x10069ff0
     * @ghidraAddress BRGlide: 0x10063060
     */
    bool LoadRallyConfigFile(const char *pszPath);

    /**
     * Write the settings to a configuration file of version 2.
     *
     * @param pszPath Path of the file.
     * @return Whether every field was written.
     * @ghidraAddress BRD3D: 0x1006a4a0
     * @ghidraAddress BRGlide: 0x100634b0
     */
    bool SaveConfigToFile(const char *pszPath);

    /**
     * The bindings of a device.
     *
     * @param nDevice The device, 1 to 3 for a game controller and any other for the keyboard.
     * @return The table of the device.
     */
    InputBinding *GetDeviceBindings(int nDevice) {
        switch (nDevice) {
        case 1:
            return aInputBindingsDevice1;
        case 2:
            return aInputBindingsDevice2;
        case 3:
            return aInputBindingsDevice3;
        default:
            return aInputBindingsKeyboard;
        }
    }

    /** Point pActiveInputBindings at the table of dwInputDeviceType. */
    void SelectActiveInputBindings() {
        pActiveInputBindings = GetDeviceBindings(dwInputDeviceType);
    }
};

/**
 * Settings of the running game.
 *
 * @ghidraAddress BRD3D: 0x10b4df30
 * @ghidraAddress BRGlide: 0x10b71290
 */
extern RallyConfig g_GlobalRallyConfig;

/**
 * Read BossRally.ini and apply the command line on top of it. Each line sets the key it begins
 * with, and each key found anywhere on the command line is set again.
 *
 * @param pszCmdLine Command line the launcher received.
 * @ghidraAddress BRD3D: 0x10007bd0
 * @ghidraAddress BRGlide: 0x10007f40
 * @stub{BRGlide}
 */
void LoadBossRallyIniSettings(const char *pszCmdLine);
