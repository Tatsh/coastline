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
int g_dwDirectPlayInstanceCount;
bool g_bDirectPlayReceiveThreadStarted;
CRITICAL_SECTION g_DirectPlaySendCritSec;
HANDLE g_hDirectPlayReceiveStopEvent;
DWORD g_dwDirectPlayReceiveThreadId;
HANDLE g_hDirectPlayReceiveThread;
char g_szDirectPlayErrorBuffer[16];

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

HRESULT ConnectFromDirectPlayLobby(DirectPlayInfo *pInfo) {
    IDirectPlay3A *pDirectPlay = nullptr;
    IDirectPlayLobby3A *pLobby = nullptr;
    DPLCONNECTION *pConnection = nullptr;
    DWORD dwSize;
    DPID dpidPlayer;
    bool bIsHost;
    HRESULT hResult;

    hResult = CoCreateInstance(CLSID_DirectPlayLobby,
                               nullptr,
                               CLSCTX_INPROC_SERVER,
                               IID_IDirectPlayLobby3A,
                               reinterpret_cast<void **>(&pLobby));
    if (SUCCEEDED(hResult)) {
        hResult = pLobby->GetConnectionSettings(0, nullptr, &dwSize);
        if (hResult == DPERR_BUFFERTOOSMALL) {
            pConnection = static_cast<DPLCONNECTION *>(GlobalLock(GlobalAlloc(GHND, dwSize)));
            if (pConnection == nullptr) {
                hResult = E_OUTOFMEMORY;
            } else {
                hResult = pLobby->GetConnectionSettings(0, pConnection, &dwSize);
            }
        }
    }
    if (pConnection != nullptr && SUCCEEDED(hResult)) {
        bIsHost = (pConnection->dwFlags & DPLCONNECTION_CREATESESSION) != 0;
        pConnection->lpSessionDesc->dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
        pConnection->lpSessionDesc->dwMaxPlayers = 8;
        hResult = pLobby->SetConnectionSettings(0, 0, pConnection);
        // The IDirectPlay4A of the connection is used through its IDirectPlay3A methods.
        if (SUCCEEDED(hResult)) {
            hResult = pLobby->ConnectEx(
                0, IID_IDirectPlay4A, reinterpret_cast<void **>(&pDirectPlay), nullptr);
        }
        if (SUCCEEDED(hResult)) {
            hResult = pDirectPlay->CreatePlayer(&dpidPlayer,
                                                pConnection->lpPlayerName,
                                                pInfo->hPlayerEvent,
                                                nullptr,
                                                0,
                                                bIsHost ? DPPLAYER_SERVERPLAYER : 0);
        }
        if (SUCCEEDED(hResult)) {
            pInfo->pDirectPlay = pDirectPlay;
            pInfo->dpidPlayer = dpidPlayer;
            pInfo->bIsHost = (pConnection->dwFlags & DPLCONNECTION_CREATESESSION) ? TRUE : FALSE;
            strcpy(g_GlobalRallyConfig.szPlayerName, pConnection->lpPlayerName->lpszShortNameA);
            if (pConnection->lpSessionDesc->lpszSessionNameA != nullptr) {
                strcpy(g_szSessionName, pConnection->lpSessionDesc->lpszSessionNameA);
            }
            hResult = DP_OK;
            pDirectPlay = nullptr;
        }
    }
    if (pDirectPlay != nullptr) {
        pDirectPlay->Release();
    }
    if (pLobby != nullptr) {
        pLobby->Release();
    }
    if (pConnection != nullptr) {
        GlobalUnlock(GlobalHandle(pConnection));
        GlobalFree(GlobalHandle(pConnection));
    }
    return hResult;
}

HRESULT CreateDirectPlayLobby(IDirectPlayLobby2A **ppLobby) {
    IDirectPlayLobbyA *pLobbyA = nullptr;
    IDirectPlayLobby2A *pLobby2 = nullptr;
    HRESULT hResult;

    hResult = DirectPlayLobbyCreateA(nullptr, &pLobbyA, nullptr, nullptr, 0);
    if (SUCCEEDED(hResult)) {
        hResult =
            pLobbyA->QueryInterface(IID_IDirectPlayLobby2A, reinterpret_cast<void **>(&pLobby2));
    }
    if (FAILED(hResult)) {
        if (pLobbyA != nullptr) {
            pLobbyA->Release();
        }
        if (pLobby2 != nullptr) {
            pLobby2->Release();
        }
        return hResult;
    }
    pLobbyA->Release();
    // The result of the enumeration is discarded.
    EnumeratePlayerAddress(g_hMainWindow, pLobby2);
    *ppLobby = pLobby2;
    return DP_OK;
}

HRESULT EnumeratePlayerAddress(HWND hWnd, IDirectPlayLobby2A *pLobby) {
    IDirectPlay3A *pDirectPlay = g_DirectPlayInfo.pDirectPlay;
    void *pAddress = nullptr;
    DWORD dwSize = 0;
    HRESULT hResult;

    hResult = pDirectPlay->GetPlayerAddress(0, nullptr, &dwSize);
    if (hResult == DPERR_BUFFERTOOSMALL) {
        pAddress = GlobalLock(GlobalAlloc(GHND, dwSize));
        if (pAddress == nullptr) {
            hResult = E_OUTOFMEMORY;
        } else {
            hResult = g_DirectPlayInfo.pDirectPlay->GetPlayerAddress(0, pAddress, &dwSize);
            if (SUCCEEDED(hResult)) {
                hResult = pLobby->EnumAddress(EnumAddressCallback, pAddress, dwSize, hWnd);
            }
        }
    }
    if (pAddress != nullptr) {
        GlobalUnlock(GlobalHandle(pAddress));
        GlobalFree(GlobalHandle(pAddress));
    }
    return hResult;
}

BOOL WINAPI EnumAddressCallback(REFGUID guidDataType, DWORD, const void *pData, void *) {
    const char *pszPort = static_cast<const char *>(pData);

    // The strings are walked and none is used.
    if (IsEqualGUID(guidDataType, DPAID_ComPort) && lstrlenA(pszPort) != 0) {
        do {
            pszPort += lstrlenA(pszPort) + 1;
        } while (lstrlenA(pszPort) != 0);
    }
    return TRUE;
}

// A case of GetDirectPlayErrorString() that returns the name of its error.
#define DIRECTPLAY_ERROR_CASE(error)                                                               \
    case error:                                                                                    \
        return #error

const char *GetDirectPlayErrorString(HRESULT hResult) {
    switch (hResult) {
        DIRECTPLAY_ERROR_CASE(DP_OK);
        DIRECTPLAY_ERROR_CASE(DPERR_PENDING);
        DIRECTPLAY_ERROR_CASE(DPERR_UNSUPPORTED);
        DIRECTPLAY_ERROR_CASE(DPERR_NOINTERFACE);
        DIRECTPLAY_ERROR_CASE(DPERR_GENERIC);
        DIRECTPLAY_ERROR_CASE(DPERR_OUTOFMEMORY);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDPARAMS);
        DIRECTPLAY_ERROR_CASE(DPERR_ALREADYINITIALIZED);
        DIRECTPLAY_ERROR_CASE(DPERR_ACCESSDENIED);
        DIRECTPLAY_ERROR_CASE(DPERR_ACTIVEPLAYERS);
        DIRECTPLAY_ERROR_CASE(DPERR_BUFFERTOOSMALL);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTADDPLAYER);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTCREATEGROUP);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTCREATEPLAYER);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTCREATESESSION);
        DIRECTPLAY_ERROR_CASE(DPERR_CAPSNOTAVAILABLEYET);
        DIRECTPLAY_ERROR_CASE(DPERR_EXCEPTION);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDFLAGS);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDOBJECT);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDPLAYER);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDGROUP);
        DIRECTPLAY_ERROR_CASE(DPERR_NOCAPS);
        DIRECTPLAY_ERROR_CASE(DPERR_NOCONNECTION);
        DIRECTPLAY_ERROR_CASE(DPERR_NOMESSAGES);
        DIRECTPLAY_ERROR_CASE(DPERR_NONAMESERVERFOUND);
        DIRECTPLAY_ERROR_CASE(DPERR_NOPLAYERS);
        DIRECTPLAY_ERROR_CASE(DPERR_NOSESSIONS);
        DIRECTPLAY_ERROR_CASE(DPERR_SENDTOOBIG);
        DIRECTPLAY_ERROR_CASE(DPERR_TIMEOUT);
        DIRECTPLAY_ERROR_CASE(DPERR_UNAVAILABLE);
        DIRECTPLAY_ERROR_CASE(DPERR_BUSY);
        DIRECTPLAY_ERROR_CASE(DPERR_USERCANCEL);
        DIRECTPLAY_ERROR_CASE(DPERR_CANNOTCREATESERVER);
        DIRECTPLAY_ERROR_CASE(DPERR_PLAYERLOST);
        DIRECTPLAY_ERROR_CASE(DPERR_SESSIONLOST);
        DIRECTPLAY_ERROR_CASE(DPERR_UNINITIALIZED);
        DIRECTPLAY_ERROR_CASE(DPERR_NONEWPLAYERS);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDPASSWORD);
        DIRECTPLAY_ERROR_CASE(DPERR_CONNECTING);
        DIRECTPLAY_ERROR_CASE(DPERR_BUFFERTOOLARGE);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTCREATEPROCESS);
        DIRECTPLAY_ERROR_CASE(DPERR_APPNOTSTARTED);
        DIRECTPLAY_ERROR_CASE(DPERR_INVALIDINTERFACE);
        DIRECTPLAY_ERROR_CASE(DPERR_NOSERVICEPROVIDER);
        DIRECTPLAY_ERROR_CASE(DPERR_UNKNOWNAPPLICATION);
        DIRECTPLAY_ERROR_CASE(DPERR_NOTLOBBIED);
        DIRECTPLAY_ERROR_CASE(DPERR_SERVICEPROVIDERLOADED);
        DIRECTPLAY_ERROR_CASE(DPERR_ALREADYREGISTERED);
        DIRECTPLAY_ERROR_CASE(DPERR_NOTREGISTERED);
        DIRECTPLAY_ERROR_CASE(DPERR_AUTHENTICATIONFAILED);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTLOADSSPI);
        DIRECTPLAY_ERROR_CASE(DPERR_ENCRYPTIONFAILED);
        DIRECTPLAY_ERROR_CASE(DPERR_SIGNFAILED);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTLOADSECURITYPACKAGE);
        DIRECTPLAY_ERROR_CASE(DPERR_ENCRYPTIONNOTSUPPORTED);
        DIRECTPLAY_ERROR_CASE(DPERR_CANTLOADCAPI);
        DIRECTPLAY_ERROR_CASE(DPERR_NOTLOGGEDIN);
        DIRECTPLAY_ERROR_CASE(DPERR_LOGONDENIED);
    default:
        wsprintfA(g_szDirectPlayErrorBuffer, "0x%08X", hResult);
        return g_szDirectPlayErrorBuffer;
    }
}

NetPeerSlot g_aNetPeerSlots[NET_PEER_SLOT_COUNT];
HANDLE g_hNetRaceStartMutex;
HANDLE g_hNetReceiveMutex;
HANDLE g_hNetPauseMutex;
HANDLE g_hNetUnpauseMutex;
HANDLE g_hNetReadyMutex;
HANDLE g_hNetKeyframeMutex;
HANDLE g_hJoinedPlayerStackMutex;
HANDLE g_hDepartedPlayerStackMutex;
HANDLE g_hNetRaceOrderMutex;
HANDLE g_hNetNotifyMutex;

static inline void CloseMutex(HANDLE *phMutex) {
    if (*phMutex != nullptr) {
        CloseHandle(*phMutex);
        *phMutex = nullptr;
    }
}

bool ShutdownNetworkSubsystem(int) {
    bool bShutDown;

    // Network play mode 2 is the host.
    if (g_dwNetworkPlayMode > 1) {
        StopNetworkWorkerThread();
    }
    bShutDown = ShutdownDirectPlaySession(&g_DirectPlayInfo) == DP_OK;
    CloseMutex(&g_hNetRaceStartMutex);
    CloseMutex(&g_hNetReceiveMutex);
    CloseMutex(&g_hNetPauseMutex);
    CloseMutex(&g_hNetUnpauseMutex);
    CloseMutex(&g_hNetReadyMutex);
    CloseMutex(&g_hNetKeyframeMutex);
    CloseMutex(&g_hJoinedPlayerStackMutex);
    CloseMutex(&g_hDepartedPlayerStackMutex);
    CloseMutex(&g_hNetRaceOrderMutex);
    CloseMutex(&g_hNetNotifyMutex);
    for (NetPeerSlot &slot : g_aNetPeerSlots) {
        CloseMutex(&slot.hMutex);
    }
    return bShutDown;
}

void FreeServiceProviderConnectionData(void) {
    for (ServiceProvider &provider : g_aServiceProviders) {
        if (provider.pConnectionData != nullptr) {
            GlobalUnlock(GlobalHandle(provider.pConnectionData));
            GlobalFree(GlobalHandle(provider.pConnectionData));
            provider.pConnectionData = nullptr;
        }
    }
}

void ShutdownDirectPlay(void) {
    char szMessage[0x104];

    ShutdownNetworkSubsystem(1);
    FreeServiceProviderConnectionData();
    sprintf(
        szMessage, "DirectPlay interface final instance count = %d\n", g_dwDirectPlayInstanceCount);
    OutputDebugStringA(szMessage);
    // The lobby is released and its pointer is not cleared.
    if (g_pDirectPlayLobby != nullptr) {
        g_pDirectPlayLobby->Release();
    }
}

void ShowNetworkErrorMessage(const char *pszFormat, HRESULT hResult) {
    char szMessage[200];

    wsprintfA(szMessage, pszFormat, GetDirectPlayErrorString(hResult));
    MessageBoxA(nullptr, szMessage, GetStringResource(0x126), MB_OK);
}
