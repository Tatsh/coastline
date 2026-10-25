#include "platform/win/directplay.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <objbase.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/frontend.h"
#include "platform/win/settings.h"

// The index of each service provider in g_aServiceProviders.
#define SERVICE_PROVIDER_IPX 0
#define SERVICE_PROVIDER_TCPIP 1
#define SERVICE_PROVIDER_MODEM 2
#define SERVICE_PROVIDER_SERIAL 3

// The timer of the main window that drives network play, and its period in milliseconds.
#define NETWORK_TIMER_ID 1
#define NETWORK_TIMER_PERIOD 1000

DirectPlayInfo g_DirectPlayInfo;
IDirectPlay3A **g_ppDirectPlay;
ServiceProvider g_aServiceProviders[SERVICE_PROVIDER_COUNT];
bool g_dwLaunchedFromLobby;
bool g_dwLobbyIsHost;
bool g_dwSessionEnumPending;
UINT_PTR g_dwNetworkTimerId;
IDirectPlayLobby2A *g_pDirectPlayLobby;
bool g_bDirectPlayReceiveThreadStarted;
CRITICAL_SECTION g_DirectPlaySendCritSec;
HANDLE g_hDirectPlayReceiveStopEvent;
DWORD g_dwDirectPlayReceiveThreadId;
HANDLE g_hDirectPlayReceiveThread;

void InitializeDirectPlayForDesktopWindow(void) {
    // The window is looked up and discarded.
    GetDesktopWindow();
    InitializeDirectPlay();
}

bool InitializeDirectPlay(void) {
    char szError[0x400];
    HRESULT hResult;

    lstrcpyA(g_szModemPhoneNumber, g_szEmptyUiString);
    lstrcpyA(g_szModemDeviceName, g_szEmptyUiString);
    memset(g_GlobalRallyConfig.szTcpIpPort, 0, sizeof(g_GlobalRallyConfig.szTcpIpPort));
    memset(g_GlobalRallyConfig.szTcpIpAddress, 0, sizeof(g_GlobalRallyConfig.szTcpIpAddress));
    for (ServiceProvider &provider : g_aServiceProviders) {
        provider.pConnectionData = nullptr;
    }
    if (FAILED(CreateDirectPlayReceiveThread(GetModuleHandleA(nullptr), &g_DirectPlayInfo))) {
        return false;
    }

    hResult = ConnectFromDirectPlayLobby(&g_DirectPlayInfo);
    if (hResult == DPERR_NOTLOBBIED) {
        g_DirectPlayInfo.pDirectPlay = nullptr;
        hResult = CreateDirectPlayObject(&g_DirectPlayInfo.pDirectPlay);
        if (FAILED(hResult)) {
            // The message is formatted and not shown.
            sprintf(szError,
                    "Could not create DirectPlay object because of error 0x%08X",
                    static_cast<unsigned>(hResult));
            ShowNetworkErrorMessage(GetStringResource(0x12b), hResult);
            return false;
        }
    } else {
        if (FAILED(hResult)) {
            ShowNetworkErrorMessage(GetStringResource(0x12a), hResult);
            exit(1);
        }
        if (g_DirectPlayInfo.bIsHost) {
            g_dwNetworkPlayMode = 2;
            g_dwLobbyIsHost = true;
        } else {
            g_dwNetworkPlayMode = 1;
            g_dwLobbyIsHost = false;
        }
        g_dwLaunchedFromLobby = true;
    }

    g_ppDirectPlay = &g_DirectPlayInfo.pDirectPlay;
    if (!g_dwLaunchedFromLobby) {
        // The result of the enumeration is discarded.
        g_DirectPlayInfo.pDirectPlay->EnumConnections(
            &g_BossRallyAppGuid, EnumConnectionsCallback_StoreServiceProvider, g_hMainWindow, 0);
        return SUCCEEDED(CreateDirectPlayLobby(&g_pDirectPlayLobby));
    }
    g_dwSessionEnumPending = false;
    if (g_pJoinCreateGameMenu != nullptr &&
        FAILED(RefreshNetworkSessionList(g_DirectPlayInfo.pDirectPlay))) {
        return false;
    }
    g_dwNetworkTimerId = SetTimer(g_hMainWindow, NETWORK_TIMER_ID, NETWORK_TIMER_PERIOD, nullptr);
    g_dwDirectPlaySessionActive = true;
    return true;
}

HRESULT CreateDirectPlayReceiveThread(HINSTANCE, DirectPlayInfo *pInfo) {
    if (!g_bDirectPlayReceiveThreadStarted) {
        InitializeCriticalSection(&g_DirectPlaySendCritSec);
        g_bDirectPlayReceiveThreadStarted = true;
    }
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->hPlayerEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (pInfo->hPlayerEvent != nullptr) {
        g_hDirectPlayReceiveStopEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
        if (g_hDirectPlayReceiveStopEvent != nullptr) {
            g_hDirectPlayReceiveThread = CreateThread(
                nullptr, 0, DirectPlayReceiveThreadProc, pInfo, 0, &g_dwDirectPlayReceiveThreadId);
            if (g_hDirectPlayReceiveThread != nullptr) {
                return DP_OK;
            }
        }
    }
    ShutdownDirectPlaySession(pInfo);
    return E_OUTOFMEMORY;
}

HRESULT ShutdownDirectPlaySession(DirectPlayInfo *pInfo) {
    if (g_bDirectPlayReceiveThreadStarted) {
        DeleteCriticalSection(&g_DirectPlaySendCritSec);
        g_bDirectPlayReceiveThreadStarted = false;
    }
    if (g_hDirectPlayReceiveThread != nullptr) {
        SetEvent(g_hDirectPlayReceiveStopEvent);
        WaitForSingleObject(g_hDirectPlayReceiveThread, INFINITE);
        CloseHandle(g_hDirectPlayReceiveThread);
        g_hDirectPlayReceiveThread = nullptr;
    }
    if (g_hDirectPlayReceiveStopEvent != nullptr) {
        CloseHandle(g_hDirectPlayReceiveStopEvent);
        g_hDirectPlayReceiveStopEvent = nullptr;
    }
    if (pInfo == nullptr) {
        return DP_OK;
    }
    if (pInfo->pDirectPlay != nullptr) {
        if (pInfo->dpidPlayer != 0) {
            pInfo->pDirectPlay->DestroyPlayer(pInfo->dpidPlayer);
            pInfo->dpidPlayer = 0;
        }
        pInfo->pDirectPlay->Close();
        pInfo->pDirectPlay->Release();
        pInfo->pDirectPlay = nullptr;
    }
    if (pInfo->hPlayerEvent != nullptr) {
        CloseHandle(pInfo->hPlayerEvent);
        pInfo->hPlayerEvent = nullptr;
    }
    return DP_OK;
}

DWORD WINAPI DirectPlayReceiveThreadProc(void *pParameter) {
    DirectPlayInfo *pInfo = static_cast<DirectPlayInfo *>(pParameter);
    HANDLE ahEvents[2];

    ahEvents[0] = pInfo->hPlayerEvent;
    ahEvents[1] = g_hDirectPlayReceiveStopEvent;
    while (WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE) == WAIT_OBJECT_0) {
        ReceiveAndDispatchDirectPlayMessages(pInfo);
    }
    ExitThread(0);
}

HRESULT CreateDirectPlayObject(IDirectPlay3A **ppDirectPlay) {
    IDirectPlay3A *pDirectPlay = nullptr;
    HRESULT hResult;

    hResult = CoCreateInstance(CLSID_DirectPlay,
                               nullptr,
                               CLSCTX_INPROC_SERVER,
                               IID_IDirectPlay3A,
                               reinterpret_cast<void **>(&pDirectPlay));
    *ppDirectPlay = pDirectPlay;
    return hResult;
}

BOOL WINAPI EnumConnectionsCallback_StoreServiceProvider(const GUID *pGuidServiceProvider,
                                                         void *pConnection,
                                                         DWORD dwConnectionSize,
                                                         const DPNAME *pName,
                                                         DWORD,
                                                         void *) {
    ServiceProvider *pProvider;

    if (IsEqualGUID(*pGuidServiceProvider, DPSPGUID_MODEM)) {
        pProvider = &g_aServiceProviders[SERVICE_PROVIDER_MODEM];
    } else if (IsEqualGUID(*pGuidServiceProvider, DPSPGUID_TCPIP)) {
        pProvider = &g_aServiceProviders[SERVICE_PROVIDER_TCPIP];
    } else if (IsEqualGUID(*pGuidServiceProvider, DPSPGUID_IPX)) {
        pProvider = &g_aServiceProviders[SERVICE_PROVIDER_IPX];
    } else if (IsEqualGUID(*pGuidServiceProvider, DPSPGUID_SERIAL)) {
        pProvider = &g_aServiceProviders[SERVICE_PROVIDER_SERIAL];
    } else {
        return TRUE;
    }
    // The whole name buffer is copied, whatever the length of the name.
    memcpy(pProvider->szName, pName->lpszShortNameA, sizeof(pProvider->szName));
    pProvider->guid = *pGuidServiceProvider;
    pProvider->pConnectionData = GlobalLock(GlobalAlloc(GHND, dwConnectionSize));
    if (pProvider->pConnectionData != nullptr) {
        memcpy(pProvider->pConnectionData, pConnection, dwConnectionSize);
        pProvider->dwConnectionSize = dwConnectionSize;
    }
    return TRUE;
}

void ShowNetworkErrorMessage(const char *pszFormat, HRESULT hResult) {
    char szMessage[200];

    wsprintfA(szMessage, pszFormat, GetDirectPlayErrorString(hResult));
    MessageBoxA(nullptr, szMessage, GetStringResource(0x126), MB_OK);
}
