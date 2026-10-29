#include "error.h"

#include <stdlib.h>
#include <string.h>

#include "ear/window.h"

uint32_t g_bShowEarErrors;
const char *const g_earErrorMessages[47] = {
    "No Error",
    "EAR was passed an incorrect validation number",
    "The sound card is in use by another application",
    "EAR could not find the sound card software",
    "Another application is already running EAR",
    "The service table is invalid in the sound card software",
    "There is a problem accessing the sound card",
    "GetConfiguration failed in the sound card software",
    "SetConfiguration failed in the sound card software",
    "User has requested application be terminated",
    "User has requested stereo mode",
    "There are no front speakers attached to the hardware",
    "There are no rear speakers attached to the hardware",
    "Error reading the specified wave file/file not found",
    "Error in file operation on specified wave file",
    "The specified file is not a wave file",
    "No memory available to allocate buffer for wave playback",
    "Channel allocation failed on the hardware DSP",
    "Unable to perform a page lock of the wave file buffer",
    "Unable to assign a buffer pointer in the hardware DSP",
    "Unable to start audio playback",
    "You must register at least one default matrix (fatal)",
    "Matrix already registered (non-fatal)",
    "Function not supported by this ear dll",
    "Ear unable to setup multimedia timer",
    "Ear received a callback to a NULL handle",
    "Ear received a callback for an unregistered handle",
    "Specified channel is not part of mixer",
    "Specified event is not part of channel",
    "Data object size mis-match",
    "Environment already registered (non-fatal)",
    "Specified environment not previously registered or is NULL",
    "Sound card set up to use a stereo solution; use EAR_PDS",
    "The Hwnd passed to the DLL cannot be the main window",
    "DirectSound creation failed",
    "Identifier must be a unique number",
    "Data supplied is out of range for this function",
    "Unable to pause sound event",
    "Channel currently operating at 100% capacity",
    "Requested matrix not previously registered",
    "Preset number already in use",
    "Requested preset not previously registered",
    "Requested resource not found",
    "CD device is empty or unavailable",
    "Unable to initialize MIDI playback device",
    "Hardware does not support EAX property sets",
    "Hardware does not recognize A3D interface",
};

// @ghidraAddress EAR: 0x1003b7e4
static uint32_t g_dwEarLastError;
// @ghidraAddress EAR: 0x100371e4
static const char g_szEarErrorPrefix[] = "Ear Error: ";
// @ghidraAddress EAR: 0x1003b978
static char g_earLastErrorDecimalText[12];
// @ghidraAddress EAR: 0x1003b988
static char g_earLastErrorDialogCaption[23];

void __stdcall SetEarLastError(uint32_t error) {
    g_dwEarLastError = error;
    if (g_bShowEarErrors) {
        EarShowLastErrorExportWrapper();
    }
}

uint32_t __stdcall EarGetLastErrorExportWrapper(void) {
    return EAR_DLL_GetLastError();
}

uint32_t __stdcall EAR_DLL_GetLastError(void) {
    return g_dwEarLastError;
}

void __stdcall EarShowLastErrorExportWrapper(void) {
    EAR_DLL_ShowLastError();
}

void __stdcall EAR_DLL_ShowLastError(void) {
    if (g_dwEarLastError == 0) {
        return;
    }
    strcpy(g_earLastErrorDialogCaption, g_szEarErrorPrefix);
    _itoa((int)g_dwEarLastError, g_earLastErrorDecimalText, 10);
    strcat(g_earLastErrorDialogCaption, g_earLastErrorDecimalText);
    MessageBoxA(g_hEarNotifyWindow,
                g_earErrorMessages[g_dwEarLastError],
                g_earLastErrorDialogCaption,
                MB_OK);
}
