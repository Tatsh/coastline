#include "win32/directplay.h"

#include <bit>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <windows.h>

#include "common/vehicleaccess.h"
// The headers below need the types of windows.h.
#include <objbase.h>

#include "common/instantreplay.h"
#include "common/laps.h"
#include "common/pause.h"
#include "common/racesound.h"
#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/strings.h"
#include "common/trackselect.h"
#include "common/vehiclecount.h"
#include "win32/app.h"
#include "win32/frametimer.h"
#include "win32/frontend.h"
#include "win32/hud.h"
#include "win32/input.h"
#include "win32/netstream.h"
#include "win32/sound.h"
#include "win32/timer.h"
#include "win32/vehicle.h"

namespace {
/**
 * The GUID of the game for DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1008f538
 * @ghidraAddress BRGlide: 0x10077500
 */
const GUID g_BossRallyAppGuid = {
    0xaeb983e0, 0xc288, 0x11d2, {0xbf, 0xd5, 0x00, 0x60, 0x08, 0x16, 0x32, 0x11}};
} // namespace

/** @ghidraAddress BRD3D: 0x1022af1c */
int g_bNetSpectator;
/** @ghidraAddress BRD3D: 0x10a9cffc */
int g_dwDirectPlaySessionActive;
/** @ghidraAddress BRD3D: 0x1022af18 */
int g_dwNetworkPlayMode;
/** @ghidraAddress BRD3D: 0x10aa287c */
int g_dwServiceProviderIndex;
/** @ghidraAddress BRD3D: 0x1039b720 */
const char g_szEmptyUiString[1] = {0};

// The index of each service provider in g_aServiceProviders.
#define SERVICE_PROVIDER_IPX 0
#define SERVICE_PROVIDER_TCPIP 1
#define SERVICE_PROVIDER_MODEM 2
#define SERVICE_PROVIDER_SERIAL 3

// The timer of the main window that drives network play, and its period in milliseconds.
#define NETWORK_TIMER_ID 1
#define NETWORK_TIMER_PERIOD 1000

DirectPlayInfo g_DirectPlayInfo;
DirectPlayInfo *g_pDirectPlayInfo;
ServiceProvider g_aServiceProviders[SERVICE_PROVIDER_COUNT];

int GetSelectedServiceProvider(GUID **ppGuid) {
    *ppGuid = &g_aServiceProviders[g_dwServiceProviderIndex].guid;
    return 0;
}

HRESULT DirectPlaySendLocked(IDirectPlay3A *pDirectPlay,
                             DPID dpidFrom,
                             DPID dpidTo,
                             DWORD dwFlags,
                             void *pData,
                             DWORD dwSize) {
    EnterCriticalSection(&g_DirectPlaySendCritSec);
    HRESULT hResult = pDirectPlay->Send(dpidFrom, dpidTo, dwFlags, pData, dwSize);
    LeaveCriticalSection(&g_DirectPlaySendCritSec);
    return hResult;
}

// The types of the messages of the game.
#define NETWORK_MESSAGE_READY 0x60000002
#define NETWORK_MESSAGE_3 0x60000003
#define NETWORK_MESSAGE_4 0x60000004
#define NETWORK_MESSAGE_RACE_STATE 0x60000005
#define NETWORK_MESSAGE_6 0x60000006
#define NETWORK_MESSAGE_7 0x60000007

// A message of the game of two words, the second naming a player or giving a value.
struct NetworkPlayerMessage {
    DWORD dwType;
    union {
        DPID dpidPlayer;
        DWORD dwValue;
    };
};

static inline HRESULT SendNetworkPlayerMessage(DirectPlayInfo *pInfo,
                                               NetworkPlayerMessage *pMessage) {
    return DirectPlaySendLocked(pInfo->pDirectPlay,
                                pInfo->dpidPlayer,
                                DPID_ALLPLAYERS,
                                DPSEND_GUARANTEED,
                                pMessage,
                                sizeof(*pMessage));
}

HRESULT SendNetworkMessage3(DirectPlayInfo *pInfo) {
    NetworkPlayerMessage message;

    if (pInfo == nullptr || pInfo->pDirectPlay == nullptr || g_dwNetworkSendBlocked) {
        return 0;
    }
    // The second word is sent unwritten.
    message.dwType = NETWORK_MESSAGE_3;
    return SendNetworkPlayerMessage(pInfo, &message);
}

static inline HRESULT SendNetworkValueMessage(DirectPlayInfo *pInfo, DWORD dwType, DWORD dwValue) {
    NetworkPlayerMessage message;

    if (pInfo == nullptr || pInfo->pDirectPlay == nullptr || g_dwNetworkSendBlocked) {
        return 0;
    }
    message.dwType = dwType;
    message.dwValue = dwValue;
    return SendNetworkPlayerMessage(pInfo, &message);
}

HRESULT SendNetworkReadyMessage(DirectPlayInfo *pInfo, DWORD dwReady) {
    return SendNetworkValueMessage(pInfo, NETWORK_MESSAGE_READY, dwReady);
}

HRESULT SendNetworkMessage4(DirectPlayInfo *pInfo, DPID dpidPlayer) {
    return SendNetworkValueMessage(pInfo, NETWORK_MESSAGE_4, dpidPlayer);
}

HRESULT SendNetworkRaceStateMessage(DirectPlayInfo *pInfo, DWORD dwState) {
    return SendNetworkValueMessage(pInfo, NETWORK_MESSAGE_RACE_STATE, dwState);
}

HRESULT SendNetworkMessage6(DirectPlayInfo *pInfo, DPID dpidPlayer) {
    NetworkPlayerMessage message;

    if (pInfo == nullptr || pInfo->pDirectPlay == nullptr) {
        return 0;
    }
    message.dpidPlayer = dpidPlayer;
    message.dwType = NETWORK_MESSAGE_6;
    return SendNetworkPlayerMessage(pInfo, &message);
}

HRESULT SendNetworkMessage7(DirectPlayInfo *pInfo, DPID dpidPlayer) {
    NetworkPlayerMessage message;

    if (pInfo == nullptr || pInfo->pDirectPlay == nullptr) {
        return 0;
    }
    message.dpidPlayer = dpidPlayer;
    message.dwType = NETWORK_MESSAGE_7;
    return SendNetworkPlayerMessage(pInfo, &message);
}

void SendNetworkMessage6ToSession(void) {
    if (g_pDirectPlayInfo != nullptr && g_pDirectPlayInfo->dpidPlayer != 0) {
        SendNetworkMessage6(g_pDirectPlayInfo, g_pDirectPlayInfo->dpidPlayer);
    }
}

void ResetNetworkStreamWithTimestamp(NetStream *pStream) {
    pStream->StreamResetReadWrite();
    pStream->StreamWriteInt24(g_dwNetWorkerCurrentTime);
}

unsigned g_dwNetClockBaseMs;

unsigned GetElapsedFrameCount(void) {
    unsigned dwElapsed = GetFrameTickCounter() - g_dwNetClockBaseMs;
    return dwElapsed / TICK_PERIOD_MS * TICKS_PER_PERIOD +
           dwElapsed % TICK_PERIOD_MS / TICK_INTERVAL_MS;
}

unsigned g_dwNetLastKeyframeFrame;

void WriteTimestampToStream(NetStream *pStream) {
    pStream->StreamResetReadWrite();
    WaitForSingleObject(g_hNetKeyframeMutex, INFINITE);
    g_dwNetLastKeyframeFrame = GetElapsedFrameCount();
    pStream->StreamWriteInt24(g_dwNetLastKeyframeFrame);
    ReleaseMutex(g_hNetKeyframeMutex);
}

unsigned g_dwNetRaceStartFrame;
int g_bNetRaceStartReached;

void CheckNetworkRaceStartTime(void) {
    WaitForSingleObject(g_hNetRaceStartMutex, INFINITE);
    if (GetElapsedFrameCount() >= g_dwNetRaceStartFrame) {
        g_bNetRaceStartReached = true;
    }
    ReleaseMutex(g_hNetRaceStartMutex);
}

HRESULT DestroyLocalDirectPlayPlayer(void) {
    HRESULT hResult = DP_OK;
    if (g_pDirectPlayInfo != nullptr && g_pDirectPlayInfo->pDirectPlay != nullptr &&
        g_pDirectPlayInfo->dpidPlayer != 0) {
        hResult = g_pDirectPlayInfo->pDirectPlay->DestroyPlayer(g_pDirectPlayInfo->dpidPlayer);
        g_pDirectPlayInfo->dpidPlayer = 0;
    }
    return hResult;
}

void ClampFloat(float *pValue) {
    if (*pValue < -1.0f) {
        *pValue = -1.0f;
    }
    if (*pValue > 1.0f) {
        *pValue = 1.0f;
    }
}

void ClampWorldCoordinate(float *pValue) {
    if (*pValue < 0.0f) {
        *pValue = 0.0f;
    }
    if (*pValue > 2048.0f) {
        *pValue = 2048.0f;
    }
}

void ClampTerrainHeightCoordinate(float *pHeight) {
    if (*pHeight < -256.0f) {
        *pHeight = -256.0f;
    }
    if (*pHeight > 256.0f) {
        *pHeight = 256.0f;
    }
}

int g_dwLaunchedFromLobby;
int g_dwLobbyIsHost;
int g_dwSessionEnumPending;
UINT_PTR g_dwNetworkTimerId;
IDirectPlayLobby2A *g_pDirectPlayLobby;
int g_dwDirectPlayInstanceCount;
int g_bDirectPlayReceiveThreadStarted;
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

    g_pDirectPlayInfo = &g_DirectPlayInfo;
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
    // The released lobby is cleared and not read again.
    pLobbyA = nullptr;
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
    const char *pszModem = static_cast<const char *>(pData);

    // The strings are walked and none is used.
    if (IsEqualGUID(guidDataType, DPAID_Modem) && lstrlenA(pszModem) != 0) {
        do {
            pszModem += lstrlenA(pszModem) + 1;
        } while (lstrlenA(pszModem) != 0);
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

int GetPlayerPingMs(int nSlot) {
    NetPeerSlot &slot = g_aNetPeerSlots[nSlot];
    WaitForSingleObject(slot.hMutex, INFINITE);
    int nPingMs = slot.dwPingMs;
    ReleaseMutex(slot.hMutex);
    return nPingMs < 0 ? 0 : nPingMs;
}

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

int g_adwNetRaceOrder[NET_PEER_SLOT_COUNT];

int GetNetworkRaceOrderEntry(int nIndex) {
    WaitForSingleObject(g_hNetRaceOrderMutex, INFINITE);
    int nEntry = g_adwNetRaceOrder[nIndex];
    ReleaseMutex(g_hNetRaceOrderMutex);
    return nEntry;
}

DWORD g_dwNetSessionStartTime;

DWORD GetElapsedTimeSinceStart(void) {
    return GetHighResolutionTime() - g_dwNetSessionStartTime;
}

bool InitializeNetworkThreading(void) {
    CreateNetworkPlayerSlotMutexes();
    StartNetworkWorkerThread();
    return true;
}

int g_bNetWorkerThreadRunning;
HANDLE g_hNetWorkerStopEvent;
HANDLE g_hNetWorkerThread;
DWORD g_dwNetWorkerThreadId;
DWORD g_dwNetWorkerNextTickTime;
unsigned g_dwNetWorkerCurrentTime;

// The milliseconds between the ticks of the worker thread of network play.
#define NET_WORKER_TICK_MS 1000

DWORD WINAPI NetworkWorkerThreadProc(LPVOID) {
    // The stop event is never read, and the loop never ends.
    for (;;) {
        g_dwNetWorkerCurrentTime = GetElapsedTimeSinceStart();
        if (g_dwNetWorkerCurrentTime < g_dwNetWorkerNextTickTime) {
            Sleep(g_dwNetWorkerNextTickTime - g_dwNetWorkerCurrentTime);
            continue;
        }
        PromoteReadyNetworkPlayersToStart();
        RankNetworkPlayersByProgress();
        ResetAllNetworkPlayerStreams();
        BuildNetworkPlayerStatusPackets();
        FlushNetworkPlayerStreams(&g_DirectPlayInfo);
        g_dwNetWorkerNextTickTime += NET_WORKER_TICK_MS;
    }
}

void StartNetworkWorkerThread(void) {
    g_hNetWorkerStopEvent = CreateEventA(nullptr, false, false, nullptr);
    g_hNetWorkerThread =
        CreateThread(nullptr, 0, NetworkWorkerThreadProc, nullptr, 0, &g_dwNetWorkerThreadId);
    g_dwNetWorkerNextTickTime = NET_WORKER_TICK_MS;
    g_bNetWorkerThreadRunning = true;
}

void StopNetworkWorkerThread(void) {
    if (!g_bNetWorkerThreadRunning) {
        return;
    }
    SetEvent(g_hNetWorkerStopEvent);
    WaitForSingleObject(g_hNetWorkerThread, INFINITE);
    CloseHandle(g_hNetWorkerThread);
    g_hNetWorkerThread = nullptr;
    CloseHandle(g_hNetWorkerStopEvent);
    g_hNetWorkerStopEvent = nullptr;
    g_bNetWorkerThreadRunning = false;
}

static inline void FreeGlobalBuffer(void *pBuffer) {
    GlobalUnlock(GlobalHandle(pBuffer));
    GlobalFree(GlobalHandle(pBuffer));
}

SessionSlot g_aSessionSlots[SESSION_SLOT_COUNT];

int GetSessionSlotFlag(DPID dpidPlayer) {
    for (const SessionSlot &slot : g_aSessionSlots) {
        if (slot.dpidPlayer == dpidPlayer) {
            return slot.nFlag;
        }
    }
    return 0;
}

int ToggleSelectedSessionSlotFlag(void) {
    if (g_pDirectPlayInfo == nullptr) {
        return 0;
    }
    for (SessionSlot &slot : g_aSessionSlots) {
        if (slot.dpidPlayer == g_pDirectPlayInfo->dpidPlayer) {
            slot.nFlag = !slot.nFlag;
            return slot.nFlag;
        }
    }
    return 0;
}

BOOL CALLBACK EnumPlayersCallback_AddPlayerToList(
    DPID dpidPlayer, DWORD, LPCDPNAME pName, DWORD dwFlags, void *) {
    if (g_pNetworkPlayerListMenu == nullptr) {
        return false;
    }
    if (!(dwFlags & DPENUMPLAYERS_SPECTATOR)) {
        MenuItemList *list = &g_pNetworkPlayerListMenu->GetItemList();
        list->AddItem(pName->lpszShortNameA, 0, 1, &g_rcMenuNetworkPlayers, true);
        list = &g_pNetworkPlayerListMenu->GetItemList();
        list->GetFont(list->GetItemCount() - 1).SetItemData(dpidPlayer);
        MarkSessionListEntrySeen(dpidPlayer);
    }
    return true;
}

void HandleDirectPlaySystemMessageHost(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD, DPID, DPID dpidTo) {
    DPMSG_SECUREMESSAGE *pSecure;

    switch (pMessage->dwType) {
    case DPSYS_DESTROYPLAYERORGROUP:
        if (!g_dwNullStubEnable) {
            DPID dpidPlayer = reinterpret_cast<DPMSG_DESTROYPLAYERORGROUP *>(pMessage)->dpId;

            ClearNetworkPlayerSlotsByOwner(dpidPlayer);
            HandlePlayerDisconnect(dpidPlayer);
        }
        break;
    case DPSYS_SECUREMESSAGE:
        pSecure = reinterpret_cast<DPMSG_SECUREMESSAGE *>(pMessage);
        ProcessMultiplayerPackets(
            pInfo, pSecure->lpData, pSecure->dwDataSize, pSecure->dpIdFrom, dpidTo);
        break;
    default:
        break;
    }
}

void HandleDirectPlaySystemMessageClient(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD, DPID, DPID dpidTo) {
    DPMSG_SECUREMESSAGE *pSecure;

    switch (pMessage->dwType) {
    case DPSYS_DESTROYPLAYERORGROUP:
        if (!g_dwNullStubEnable) {
            HandlePlayerDisconnect(reinterpret_cast<DPMSG_DESTROYPLAYERORGROUP *>(pMessage)->dpId);
        }
        break;
    case DPSYS_SECUREMESSAGE:
        pSecure = reinterpret_cast<DPMSG_SECUREMESSAGE *>(pMessage);
        ProcessNetworkDataStream(
            pInfo, pSecure->lpData, pSecure->dwDataSize, pSecure->dpIdFrom, dpidTo);
        break;
    default:
        break;
    }
}

int g_dwNetworkSendBlocked;
NetPlayerSlot g_aNetPlayerSlots[NET_PLAYER_SLOT_COUNT];
NetPlayerSlot g_aNetPeerSlotMatrix[NET_PLAYER_SLOT_COUNT][NET_PLAYER_SLOT_COUNT];
int g_nNetPauseRequestTicks;

int g_bNetRaceStarted;
unsigned g_dwNetRaceStartCountdownTime;
DWORD g_dwNetSlotResetTime;
unsigned g_dwNetNextFinishPosition;

void ResetAllPlayerSlots(void) {
    g_bNetRaceStarted = false;
    g_dwNetNextFinishPosition = 0;
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        WaitForSingleObject(slot.hMutex, INFINITE);
        slot.dwLastTimeSync = 0;
        slot.dwRaceTime = 0;
        slot.dwTimeSyncValue = 0;
        slot.dwFinishTime = 0;
        slot.dwState = 0;
        ReleaseMutex(slot.hMutex);
        for (int j = 0; j < NET_PLAYER_SLOT_COUNT; j++) {
            NetPlayerSlot &peer = g_aNetPeerSlotMatrix[j][i];
            WaitForSingleObject(peer.hMutex, INFINITE);
            peer.dwLastUpdateTime = g_dwNetSlotResetTime;
            peer.dwLastTimeSync = 0;
            peer.dwRaceTime = 0;
            peer.dwTimeSyncValue = 0;
            peer.dwFinishTime = 0;
            peer.dwState = 0;
            ReleaseMutex(peer.hMutex);
        }
    }
}

int FindNetworkSlotByPlayerId(DPID dpidPlayer) {
    if (dpidPlayer == DPID_SERVERPLAYER) {
        return 0;
    }
    for (int i = 1; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        WaitForSingleObject(slot.hMutex, INFINITE);
        DPID owner = slot.dpidOwner;
        int state = slot.dwState;
        ReleaseMutex(slot.hMutex);
        if ((state & 0x3f) >= 1 && owner == dpidPlayer) {
            return i;
        }
    }
    for (int i = 1; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        WaitForSingleObject(slot.hMutex, INFINITE);
        bool free = (slot.dwState & 0x3f) == 0;
        ReleaseMutex(slot.hMutex);
        if (free) {
            return i;
        }
    }
    return -1;
}

template <typename Slot>
concept NetworkSnapshotSlot = requires(Slot &slot, const Slot &readOnly) {
    { readOnly.adwSnapshotTimes[0] } -> std::same_as<const unsigned &>;
    { slot.adwSnapshotTypes[0] } -> std::same_as<unsigned &>;
    { slot.nCurrentSnapshot } -> std::same_as<int &>;
    { slot.aSnapshots[0] } -> std::same_as<KeyframeState &>;
};

template <NetworkSnapshotSlot Slot>
static inline int FindOldestNetworkSnapshot(const Slot &slot) {
    unsigned oldest = 0xffffffff;
    int index = 0;
    for (int i = 0; i < 8; i++) {
        if (slot.adwSnapshotTimes[i] <= oldest) {
            index = i;
            oldest = slot.adwSnapshotTimes[i];
        }
    }
    return index;
}

template <NetworkSnapshotSlot Slot>
static inline void ReadNetworkDeltaSnapshot(Slot &slot, NetStream &stream, unsigned timestamp) {
    int oldest = FindOldestNetworkSnapshot(slot);
    int newest = 0;
    int previous = 0;
    unsigned newestTime = 0;
    unsigned previousTime = 0;
    for (int i = 0; i < 8; i++) {
        if (slot.adwSnapshotTypes[i] == 0x40 && slot.adwSnapshotTimes[i] > newestTime) {
            newest = i;
            newestTime = slot.adwSnapshotTimes[i];
        }
    }
    for (int i = 0; i < 8; i++) {
        if (slot.adwSnapshotTypes[i] == 0x40 && slot.adwSnapshotTimes[i] > previousTime &&
            i != newest) {
            previous = i;
            previousTime = slot.adwSnapshotTimes[i];
        }
    }
    unsigned difference = slot.adwSnapshotTimes[newest] - slot.adwSnapshotTimes[previous];
    float share = difference == 0 ?
                      1.0f :
                      static_cast<float>(
                          static_cast<long double>(timestamp - slot.adwSnapshotTimes[previous]) /
                          static_cast<int>(difference));
    slot.nCurrentSnapshot = oldest;
    slot.adwSnapshotTimes[oldest] = timestamp;
    slot.adwSnapshotTypes[oldest] = 0x80;
    InterpolateKeyframeState(
        &slot.aSnapshots[oldest], share, &slot.aSnapshots[previous], &slot.aSnapshots[newest]);
    DeserializeKeyframeDelta(&slot.aSnapshots[oldest], &slot.aSnapshots[newest], &stream);
}

static inline void
ReadHostSnapshot(NetPlayerSlot &slot, NetStream &stream, unsigned timestamp, bool delta) {
#ifdef ENABLE_PATCHES
    KeyframeState discarded = {};
#else
    KeyframeState discarded;
#endif /* ENABLE_PATCHES */
    if (!delta) {
        if (timestamp > slot.adwSnapshotTimes[slot.nCurrentSnapshot]) {
            slot.nCurrentSnapshot = FindOldestNetworkSnapshot(slot);
            slot.adwSnapshotTimes[slot.nCurrentSnapshot] = timestamp;
            slot.adwSnapshotTypes[slot.nCurrentSnapshot] = 0x40;
            KeyframeState &snapshot = slot.aSnapshots[slot.nCurrentSnapshot];
            DeserializeVehicleState(&snapshot, &stream);
            if (slot.dwFinishTime == 0 && snapshot.fProgress >= 4188888.0f) {
                slot.dwFinishTime = GetElapsedTimeSinceStart();
            }
        } else {
            DeserializeVehicleState(&discarded, &stream);
        }
    } else if (timestamp > slot.adwSnapshotTimes[slot.nCurrentSnapshot]) {
        ReadNetworkDeltaSnapshot(slot, stream, timestamp);
    } else {
        // Rejected deltas still consume their bits using an uninitialised reference frame.
        DeserializeKeyframeDelta(&discarded, &discarded, &stream);
    }
}

static inline void PropagateNetworkStatusFlag(int flag) {
    for (NetPlayerSlot &slot : g_aNetPlayerSlots) {
        WaitForSingleObject(slot.hMutex, INFINITE);
        int state = slot.dwState & 0x3f;
        if (state >= 2 && state <= 4) {
            slot.dwState = (slot.dwState & ~(flag == 0x80 ? 0x40 : 0x80)) | flag;
        }
        ReleaseMutex(slot.hMutex);
    }
}

void ProcessMultiplayerPackets(
    DirectPlayInfo *pInfo, void *pData, DWORD dwSize, DPID dpidFrom, DPID dpidTo) {
    NetStream stream;
    stream.StreamAttachBuffer(static_cast<unsigned char *>(pData), dwSize);
    stream.StreamResetRead();
    unsigned timestamp = stream.StreamReadInt24();
    bool propagate40 = false;
    bool propagate80 = false;
    while (!stream.StreamIsAtEnd()) {
        unsigned header = stream.StreamReadByte();
        int player = header & 0xf;
        bool echoed = (header & 0x10) != 0;
        NetPlayerSlot &slot = g_aNetPlayerSlots[player];
        switch (header & 0xe0) {
        case 0: {
            int subjectIndex = stream.StreamReadByte();
            int state = stream.StreamReadByte();
            int car = stream.StreamReadByte();
            unsigned char red = stream.StreamReadByte();
            unsigned char green = stream.StreamReadByte();
            unsigned char blue = stream.StreamReadByte();
            unsigned owner = stream.StreamReadInt32();
            char name[25];
            bool hasName = (state & 0x3f) <= 2;
            if (hasName) {
                for (int i = 0; i < 24; i++) {
                    name[i] = static_cast<char>(stream.StreamReadByte());
                }
                name[24] = 0;
            }
            if ((state & 0x3f) == 4) {
                stream.StreamReadInt24();
            }
            WaitForSingleObject(slot.hMutex, INFINITE);
            if ((slot.dwState & 0x3f) >= 1 && slot.dpidOwner == dpidFrom) {
                if (echoed) {
                    NetPlayerSlot &peer = g_aNetPeerSlotMatrix[player][subjectIndex];
                    WaitForSingleObject(peer.hMutex, INFINITE);
                    if (timestamp >= peer.dwLastUpdateTime) {
                        if (player == subjectIndex && (state & 0xc0) != 0) {
                            slot.dwState &= ~0xc0;
                            state &= 0x3f;
                        }
                        peer.dwLastUpdateTime = timestamp;
                        peer.dwState = state;
                        peer.nStatusValue = car;
                        peer.bStatusOptionA = red;
                        peer.bStatusOptionB = green;
                        peer.bStatusOptionC = blue;
                        peer.dpidOwner = owner;
                        if (hasName) {
                            std::strcpy(peer.szPlayerName, name);
                        }
                    }
                    ReleaseMutex(peer.hMutex);
                } else if (player == subjectIndex) {
                    propagate40 |= (state & 0x40) != 0;
                    propagate80 |= (state & 0x80) != 0;
                    slot.dwState = state;
                }
            }
            ReleaseMutex(slot.hMutex);
            break;
        }
        case 0x40:
        case 0x80:
            WaitForSingleObject(slot.hMutex, INFINITE);
            if ((slot.dwState & 0x3f) >= 2 && slot.dpidOwner == dpidFrom) {
                ReadHostSnapshot(slot, stream, timestamp, (header & 0xe0) == 0x80);
            }
            ReleaseMutex(slot.hMutex);
            break;
        case 0x60:
            stream.StreamResetRead();
            stream.StreamSkip(stream.StreamGetWriteSize());
            break;
        case 0xc0: {
            unsigned raceTime = stream.StreamReadInt24();
            unsigned elapsed = GetElapsedTimeSinceStart() - raceTime;
            WaitForSingleObject(slot.hMutex, INFINITE);
            if ((slot.dwState & 0x3f) >= 1 && slot.dpidOwner == dpidFrom && echoed) {
                slot.dwRaceTime = raceTime;
                slot.dwTimeSyncValue = elapsed;
            }
            ReleaseMutex(slot.hMutex);
            break;
        }
        case 0xe0: {
            int subjectIndex = stream.StreamReadByte();
            int car = stream.StreamReadByte();
            unsigned char red = stream.StreamReadByte();
            unsigned char green = stream.StreamReadByte();
            unsigned char blue = stream.StreamReadByte();
            char name[25];
            for (int i = 0; i < 24; i++) {
                name[i] = static_cast<char>(stream.StreamReadByte());
            }
            name[24] = 0;
            if (echoed) {
                WaitForSingleObject(slot.hMutex, INFINITE);
                if ((slot.dwState & 0x3f) >= 1 && slot.dpidOwner == dpidFrom &&
                    player == subjectIndex) {
                    slot.nStatusValue = car;
                    slot.bStatusOptionA = red;
                    slot.bStatusOptionB = green;
                    slot.bStatusOptionC = blue;
                    std::strcpy(slot.szPlayerName, name);
                    slot.dwState = 2;
                }
                ReleaseMutex(slot.hMutex);
            } else if (!g_bNetRaceStarted) {
                int index = FindNetworkSlotByPlayerId(dpidFrom);
                if (index != -1) {
                    NetPlayerSlot &assigned = g_aNetPlayerSlots[index];
                    WaitForSingleObject(assigned.hMutex, INFINITE);
                    assigned.dpidOwner = dpidFrom;
                    for (int i = 0; i < 8; i++) {
                        assigned.adwSnapshotTimes[i] = 0;
                        assigned.adwSnapshotTypes[i] = 0;
                        std::memset(&assigned.aSnapshots[i], 0, sizeof(KeyframeState));
                    }
                    assigned.dwState = 1;
                    assigned.nCurrentSnapshot = 0;
                    assigned.dwLastTimeSync = GetElapsedTimeSinceStart();
                    for (NetPlayerSlot &peer : g_aNetPeerSlotMatrix[index]) {
                        WaitForSingleObject(peer.hMutex, INFINITE);
                        peer.dwLastUpdateTime = 0;
                        peer.dwState = 0;
                        ReleaseMutex(peer.hMutex);
                    }
                    ReleaseMutex(assigned.hMutex);
                }
            }
            break;
        }
        default:
            // Every return passes through the NullStub destructor of the stream.
            NullStub();
            return;
        }
    }
    if (propagate80) {
        PropagateNetworkStatusFlag(0x80);
    }
    if (propagate40) {
        PropagateNetworkStatusFlag(0x40);
    }
    if (dpidFrom != DPID_SERVERPLAYER) {
        unsigned char *bytes = static_cast<unsigned char *>(pData);
        unsigned tag = bytes[3] & 0xe0;
        if (tag == 0x40 || tag == 0x80 || tag == 0x60) {
            ProcessNetworkDataStream(pInfo, pData, dwSize, dpidFrom, dpidTo);
        }
    }
    NullStub();
}

void CreateNetworkPlayerSlotMutexes(void) {
    g_dwNetSessionStartTime = GetHighResolutionTime();
    g_dwNetSlotResetTime = GetElapsedTimeSinceStart();
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        g_aNetPlayerSlots[i].hMutex = CreateMutexA(nullptr, FALSE, nullptr);
        for (int j = 0; j < NET_PLAYER_SLOT_COUNT; j++) {
            g_aNetPeerSlotMatrix[j][i].hMutex = CreateMutexA(nullptr, FALSE, nullptr);
        }
    }
    ResetAllPlayerSlots();
}

HRESULT SendNetworkFinishPositionMessage(DirectPlayInfo *pInfo, DPID dpidPlayer, DWORD dwPosition) {
    if (pInfo == nullptr || pInfo->pDirectPlay == nullptr) {
        return 0;
    }
    struct {
        DWORD dwType;
        DPID dpidPlayer;
        DWORD dwPosition;
    } message = {0x60000008, dpidPlayer, dwPosition};
    if (pInfo->bIsHost) {
        ProcessNetworkDataStream(
            pInfo, &message, sizeof(message), pInfo->dpidPlayer, DPID_SERVERPLAYER);
    }
    return DirectPlaySendLocked(pInfo->pDirectPlay,
                                pInfo->dpidPlayer,
                                DPID_ALLPLAYERS,
                                DPSEND_GUARANTEED,
                                &message,
                                sizeof(message));
}

static inline void LockNetworkWorkerSlot(NetPlayerSlot &slot) {
    HANDLE handles[2] = {g_hNetWorkerStopEvent, slot.hMutex};
    if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0) {
        ExitThread(0);
    }
}

static inline void FinishNetworkPlayer(NetPlayerSlot &slot) {
    SendNetworkFinishPositionMessage(g_pDirectPlayInfo, slot.dpidOwner, g_dwNetNextFinishPosition);
    slot.dwState = g_dwNetNextFinishPosition + 5;
    g_dwNetNextFinishPosition++;
}

void RankNetworkPlayersByProgress(void) {
    int finishCount = 0;
    for (int i = NET_PLAYER_SLOT_COUNT - 1; i >= 0; i--) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        LockNetworkWorkerSlot(slot);
        int state = slot.dwState & 0x3f;
        if (state >= 2 && state < 5 && slot.dwFinishTime != 0) {
            g_aNetPlayerProgressSort[finishCount].nPlayer = i;
            g_aNetPlayerProgressSort[finishCount].fProgress = static_cast<float>(slot.dwFinishTime);
            finishCount++;
        }
        ReleaseMutex(slot.hMutex);
    }
    if (finishCount != 0) {
        std::qsort(g_aNetPlayerProgressSort,
                   finishCount,
                   sizeof(NetPlayerProgress),
                   CompareNetworkPlayerProgressDescending);
        for (int i = finishCount - 1; i >= 0; i--) {
            NetPlayerSlot &slot = g_aNetPlayerSlots[g_aNetPlayerProgressSort[i].nPlayer];
            LockNetworkWorkerSlot(slot);
            int state = slot.dwState & 0x3f;
            if (state >= 2 && state < 5 && slot.dwFinishTime != 0) {
                FinishNetworkPlayer(slot);
            }
            ReleaseMutex(slot.hMutex);
        }
    }
    int activeCount = 0;
    int tail = NET_PLAYER_SLOT_COUNT - 1;
    for (int i = NET_PLAYER_SLOT_COUNT - 1; i >= 0; i--) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        LockNetworkWorkerSlot(slot);
        int state = slot.dwState & 0x3f;
        if (state >= 2 && state < 5 &&
            slot.aSnapshots[slot.nCurrentSnapshot].fProgress >= 4188888.0f) {
            FinishNetworkPlayer(slot);
        }
        state = slot.dwState & 0x3f;
        if (state >= 5) {
            g_aNetPlayerProgressSort[activeCount].nPlayer = i;
            g_aNetPlayerProgressSort[activeCount++].fProgress = 10000000.0f - slot.dwState;
        } else if (state >= 2) {
            g_aNetPlayerProgressSort[activeCount].nPlayer = i;
            g_aNetPlayerProgressSort[activeCount++].fProgress =
                slot.aSnapshots[slot.nCurrentSnapshot].fProgress;
        } else {
            g_aNetPlayerProgressSort[tail].nPlayer = i;
            g_aNetPlayerProgressSort[tail--].fProgress = -10000000.0f - i;
        }
        ReleaseMutex(slot.hMutex);
    }
    std::qsort(g_aNetPlayerProgressSort,
               activeCount,
               sizeof(NetPlayerProgress),
               CompareNetworkPlayerProgressDescending);
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        g_adwNetPlayerRankPosition[g_aNetPlayerProgressSort[i].nPlayer] = i;
    }
}

void PromoteReadyNetworkPlayersToStart(void) {
    for (NetPlayerSlot &slot : g_aNetPlayerSlots) {
        LockNetworkWorkerSlot(slot);
        int state = slot.dwState & 0x3f;
        ReleaseMutex(slot.hMutex);
        if (state >= 2 && state != 3) {
            return;
        }
    }
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        LockNetworkWorkerSlot(slot);
        bool ready = (slot.dwState & 0x3f) == 3;
        ReleaseMutex(slot.hMutex);
        if (ready) {
            NetPlayerSlot &peer = g_aNetPeerSlotMatrix[i][i];
            LockNetworkWorkerSlot(peer);
            bool waiting = (peer.dwState & 0x3f) != 3;
            ReleaseMutex(peer.hMutex);
            if (waiting) {
                return;
            }
        }
    }
    for (NetPlayerSlot &slot : g_aNetPlayerSlots) {
        LockNetworkWorkerSlot(slot);
        if ((slot.dwState & 0x3f) == 3) {
            slot.dwState = 4;
            g_bNetRaceStarted = true;
            g_dwNetRaceStartCountdownTime = g_dwNetWorkerCurrentTime + 3000;
        }
        ReleaseMutex(slot.hMutex);
    }
}

void ResetAllNetworkPlayerStreams(void) {
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        LockNetworkWorkerSlot(slot);
        ResetNetworkStreamWithTimestamp(&g_aNetPlayerStreams[i]);
        ReleaseMutex(slot.hMutex);
    }
}

int FlushNetworkPlayerStreams(DirectPlayInfo *pInfo) {
    int result = 0;
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &slot = g_aNetPlayerSlots[i];
        LockNetworkWorkerSlot(slot);
        NetStream &stream = g_aNetPlayerStreams[i];
        if ((slot.dwState & 0x3f) >= 1 && stream.StreamGetWriteSize() > 3) {
            if (i == 0) {
                ProcessNetworkDataStream(pInfo,
                                         stream.StreamGetBufferPtr(),
                                         stream.StreamGetWriteSize(),
                                         DPID_SERVERPLAYER,
                                         DPID_SERVERPLAYER);
            } else if (DirectPlaySendLocked(pInfo->pDirectPlay,
                                            DPID_SERVERPLAYER,
                                            slot.dpidOwner,
                                            0,
                                            stream.StreamGetBufferPtr(),
                                            stream.StreamGetWriteSize()) != 0) {
                result = -1;
            }
        }
        ReleaseMutex(slot.hMutex);
    }
    return result;
}

void BuildNetworkPlayerStatusPackets(void) {
    bool active = false;
    for (int i = 0; i < NET_PLAYER_SLOT_COUNT; i++) {
        NetPlayerSlot &recipient = g_aNetPlayerSlots[i];
        NetStream &stream = g_aNetPlayerStreams[i];
        LockNetworkWorkerSlot(recipient);
        int state = recipient.dwState & 0x3f;
        if (state >= 1) {
            active = true;
            if (state == 1) {
                WriteGameSettingsPacket(&stream, static_cast<unsigned char>(i));
            } else {
                for (int j = 0; j < NET_PLAYER_SLOT_COUNT; j++) {
                    NetPlayerSlot &subject = g_aNetPlayerSlots[j];
                    NetPlayerSlot &peer = g_aNetPeerSlotMatrix[i][j];
                    char playerName[0x400];
                    LockNetworkWorkerSlot(subject);
                    LockNetworkWorkerSlot(peer);
                    int statusValue = subject.nStatusValue;
                    unsigned char optionA = subject.bStatusOptionA;
                    unsigned char optionB = subject.bStatusOptionB;
                    unsigned char optionC = subject.bStatusOptionC;
                    std::strcpy(playerName, subject.szPlayerName);
                    int subjectState = subject.dwState;
                    bool changed = peer.dwState != subjectState;
                    if (changed && (subjectState & 0x3f) == 2 && i == j) {
                        peer.dwState = subjectState;
                        changed = false;
                    }
                    ReleaseMutex(peer.hMutex);
                    if (changed) {
                        WritePlayerStatusPacket(&stream,
                                                static_cast<unsigned char>(j),
                                                subjectState,
                                                statusValue,
                                                optionA,
                                                optionB,
                                                optionC,
                                                playerName,
                                                subject.dpidOwner);
                    }
                    ReleaseMutex(subject.hMutex);
                }
            }
            WritePlayerRankingPacket(&stream, static_cast<unsigned char>(i));
            if (g_dwNetWorkerCurrentTime > recipient.dwLastTimeSync + 1000 && !g_bNetRaceStarted &&
                WriteTimeSyncPacket(&stream,
                                    static_cast<unsigned char>(i),
                                    recipient.dwRaceTime,
                                    recipient.dwTimeSyncValue)) {
                recipient.dwLastTimeSync = g_dwNetWorkerCurrentTime;
                for (int j = 0; j < NET_PLAYER_SLOT_COUNT; j++) {
                    if (j == i) {
                        continue;
                    }
                    NetPlayerSlot &subject = g_aNetPlayerSlots[j];
                    LockNetworkWorkerSlot(subject);
                    bool fits = true;
                    if ((subject.dwState & 0x3f) >= 1) {
                        fits = WriteTimeSyncPacket(&stream,
                                                   static_cast<unsigned char>(j),
                                                   subject.dwRaceTime,
                                                   subject.dwTimeSyncValue);
                    }
                    ReleaseMutex(subject.hMutex);
                    if (!fits) {
                        break;
                    }
                }
            }
        }
        ReleaseMutex(recipient.hMutex);
    }
    if (!active) {
        g_bNetRaceStarted = false;
    }
}

static inline int GetNetPlayerSlotStateLocked(NetPlayerSlot &slot) {
    WaitForSingleObject(slot.hMutex, INFINITE);
    int nState = slot.dwState;
    ReleaseMutex(slot.hMutex);
    return nState;
}

int GetNetworkPlayerSlotStatus(int nSlot) {
    return GetNetPlayerSlotStateLocked(g_aNetPlayerSlots[nSlot]);
}

int GetNetworkPeerChannelStatus(int nSlot) {
    return GetNetPlayerSlotStateLocked(g_aNetPeerSlotMatrix[nSlot][nSlot]);
}

// The state of a slot below 5 is one the departure of its owner frees.
#define NET_PLAYER_STATE_MASK 0x3f
#define NET_PLAYER_STATE_FREEABLE_LIMIT 5

void ClearNetworkPlayerSlotsByOwner(DPID dpidOwner) {
    for (NetPlayerSlot &slot : g_aNetPlayerSlots) {
        WaitForSingleObject(slot.hMutex, INFINITE);
        if (slot.dpidOwner == dpidOwner &&
            (slot.dwState & NET_PLAYER_STATE_MASK) < NET_PLAYER_STATE_FREEABLE_LIMIT) {
            slot.dwState = 0;
        }
        ReleaseMutex(slot.hMutex);
    }
}

void ResetSessionSlotTable(void) {
    g_dwNetworkSendBlocked = false;
    for (SessionSlot &slot : g_aSessionSlots) {
        slot.dpidPlayer = 0xffffffff;
        slot.nFlag = 0;
        slot.nSeen = 0;
    }
}

void MarkSessionListEntrySeen(DPID dpidPlayer) {
    // The index is a signed byte in the original.
    signed char nFree = -1;

    for (int i = 0; i < SESSION_SLOT_COUNT; i++) {
        if (g_aSessionSlots[i].dpidPlayer == dpidPlayer) {
            g_aSessionSlots[i].nSeen = true;
            return;
        }
        if (g_aSessionSlots[i].dpidPlayer == 0xffffffff && nFree < 0) {
            nFree = static_cast<signed char>(i);
        }
    }
    if (nFree < 0) {
        return;
    }
    g_aSessionSlots[nFree].dpidPlayer = dpidPlayer;
    g_aSessionSlots[nFree].nFlag = dpidPlayer == DPID_SERVERPLAYER;
    g_aSessionSlots[nFree].nSeen = true;
}

void PurgeStaleSessionListEntries(void) {
    for (SessionSlot &slot : g_aSessionSlots) {
        if (!slot.nSeen) {
            slot.dpidPlayer = 0xffffffff;
            slot.nFlag = 0;
            slot.nSeen = 0;
        }
    }
}

bool RequestNetworkPause(void) {
    WaitForSingleObject(g_hNetPauseMutex, INFINITE);
    if (g_nNetPauseRequestTicks == 0) {
        g_nNetPauseRequestTicks = 1;
    }
    ReleaseMutex(g_hNetPauseMutex);
    return true;
}

void CopyPlayerNameToSlotBuffer(int slotIndex, const char *name) {
    WaitForSingleObject(g_aNetPeerSlots[slotIndex].hMutex, INFINITE);
    char *destination = g_aNetPeerSlots[slotIndex].szName;
    size_t length = std::strlen(name) + 1;
    size_t offset = 0;
    /* Forward four-byte groups preserve the original overlapping-copy behaviour. */
    for (; length - offset >= 4; offset += 4) {
        unsigned word;
        std::memcpy(&word, name + offset, 4);
        std::memcpy(destination + offset, &word, 4);
    }
    for (; offset < length; ++offset) {
        destination[offset] = name[offset];
    }
    ReleaseMutex(g_aNetPeerSlots[slotIndex].hMutex);
}

int CountActivePlayerSlots(void) {
    const int count = g_dwNumActiveVehicles;
    int active = 0;
    for (int i = 0; i < count; ++i) {
        if (VEHICLE_GET(&(g_aVehicles[i]), Driver)) {
            ++active;
        }
    }
    return active;
}

void MaybeSendAllPlayersReadyPacket(void) {
    WaitForSingleObject(g_hNetReadyMutex, INFINITE);
    int ready = g_bNetReadyToStart;
    ReleaseMutex(g_hNetReadyMutex);
    if (ready) {
        const unsigned active = static_cast<unsigned>(CountActivePlayerSlots());
        if (active == GetDirectPlaySessionPlayerCount()) {
            SendPlayerInfoPacket(&g_DirectPlayInfo,
                                 static_cast<unsigned char>(g_dwLocalNetPlayerId),
                                 static_cast<unsigned char>(g_dwChosenCar),
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                                 g_DirectPlayInfo.dpidPlayer,
                                 g_GlobalRallyConfig.szPlayerName,
                                 3,
                                 0);
        }
    }
}

void TickNetworkPauseRequest(void) {
    WaitForSingleObject(g_hNetPauseMutex, INFINITE);
    int ticks = g_nNetPauseRequestTicks;
    if (ticks != 0) {
        ticks++;
        g_nNetPauseRequestTicks = ticks;
        if (ticks >= 27) {
            g_bNetGamePaused = true;
            ticks = 0;
            g_nNetPauseRequestTicks = 0;
        }
    }
    ReleaseMutex(g_hNetPauseMutex);
    if (ticks != 0 && g_dwNetworkPlayMode && g_bNetRaceStartReached && !gInstantReplayActive &&
        VEHICLE_GET(&(g_aVehicles[0]), Lap) < g_dwTotalLaps) {
        int state = GetSlotStatusThreadSafe(g_dwLocalNetPlayerId);
        state = (state & ~0x40) | 0x80;
        SendPlayerInfoPacket(&g_DirectPlayInfo,
                             static_cast<unsigned char>(g_dwLocalNetPlayerId),
                             static_cast<unsigned char>(g_dwChosenCar),
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                             g_DirectPlayInfo.dpidPlayer,
                             g_GlobalRallyConfig.szPlayerName,
                             state,
                             0);
    }
}

int g_nNetUnpauseRequestTicks;

void TickNetworkUnpauseRequest(void) {
    WaitForSingleObject(g_hNetUnpauseMutex, INFINITE);
    int ticks = g_nNetUnpauseRequestTicks;
    if (ticks != 0) {
        ticks = std::bit_cast<int>(static_cast<unsigned>(ticks) + 1u);
        g_nNetUnpauseRequestTicks = ticks;
        if (ticks >= 100) {
            g_dwNetworkDataReceived = 1;
            ticks = 0;
            g_nNetUnpauseRequestTicks = 0;
        }
    }
    ReleaseMutex(g_hNetUnpauseMutex);
    if (ticks != 0) {
        int state = (GetSlotStatusThreadSafe(g_dwLocalNetPlayerId) & ~0x80) | 0x40;
        SendPlayerInfoPacket(&g_DirectPlayInfo,
                             static_cast<unsigned char>(g_dwLocalNetPlayerId),
                             static_cast<unsigned char>(g_dwChosenCar),
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                             VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                             g_DirectPlayInfo.dpidPlayer,
                             g_GlobalRallyConfig.szPlayerName,
                             state,
                             0);
    }
}

bool RequestNetworkUnpause(void) {
    WaitForSingleObject(g_hNetUnpauseMutex, INFINITE);
    if (g_nNetUnpauseRequestTicks == 0) {
        g_nNetUnpauseRequestTicks = 1;
    }
    ReleaseMutex(g_hNetUnpauseMutex);
    return true;
}

int g_bNetReadyToStart;

bool SetNetworkReadyToStartFlag(void) {
    WaitForSingleObject(g_hNetReadyMutex, INFINITE);
    g_bNetReadyToStart = true;
    ReleaseMutex(g_hNetReadyMutex);
    return true;
}

int g_bNetReceiveEnabled;

void EnableNetworkReceive(void) {
    WaitForSingleObject(g_hNetReceiveMutex, INFINITE);
    g_bNetReceiveEnabled = true;
    ReleaseMutex(g_hNetReceiveMutex);
}

void DisableNetworkReceive(void) {
    WaitForSingleObject(g_hNetReceiveMutex, INFINITE);
    g_bNetReceiveEnabled = false;
    ReleaseMutex(g_hNetReceiveMutex);
}

int g_dwLocalNetPlayerId = -1;

unsigned WaitForPlayerIdAssignment(void) {
#ifdef ENABLE_PATCHES
    unsigned joined = 0;
#else
    // An id assigned between the store of -1 and the first test returns this unset.
    unsigned joined;
#endif
    g_dwLocalNetPlayerId = -1;
    while (g_dwLocalNetPlayerId == -1) {
        joined = SendPlayerJoinPacket(&g_DirectPlayInfo,
                                      static_cast<unsigned char>(g_dwLocalNetPlayerId),
                                      VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                                      VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                                      VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                                      g_GlobalRallyConfig.szPlayerName,
                                      0) != -1;
        DWORD start = GetHighResolutionTime();
        if (g_DirectPlayInfo.bIsHost) {
            g_dwLocalNetPlayerId = 0;
            LoadNetworkPlayerColors();
            SendPlayerJoinPacket(&g_DirectPlayInfo,
                                 static_cast<unsigned char>(g_dwLocalNetPlayerId),
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                                 g_GlobalRallyConfig.szPlayerName,
                                 0x10);
            SendPlayerInfoPacket(&g_DirectPlayInfo,
                                 static_cast<unsigned char>(g_dwLocalNetPlayerId),
                                 static_cast<unsigned char>(g_dwChosenCar),
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                                 g_DirectPlayInfo.dpidPlayer,
                                 g_GlobalRallyConfig.szPlayerName,
                                 2,
                                 0x10);
        }
        if (g_dwLocalNetPlayerId != -1) {
            break;
        }
        DWORD deadline = start + 2000;
        do {
            if (GetHighResolutionTime() >= deadline) {
                break;
            }
            Sleep(0);
        } while (g_dwLocalNetPlayerId == -1);
    }
    return joined;
}

const RECT g_rcNetCarSetupPlayerList = {32, 370, 610, 467};
DPID g_dwNetCarSetupSelectedPlayerId = 0xffffffff;
int g_dwNetworkCarSetupShown;
int g_dwNetworkDataStreamActive;
int g_dwNetworkEscapePending;
unsigned g_dwNetRaceStartTimeMs;
NetClockSample g_aNetClockSamples[8];
int g_nNetClockSampleIndex;
unsigned g_dwNetBestClockRoundTrip;
char g_szSlotPlayerNameBuffer[0x400];
int g_dwNetworkDataReceived;
int g_nPendingNetSoundEffect;
const char *g_apszPlaceNames[8] = {
    "First!", "Second!", "Third!", "Fourth.", "Fifth.", "Sixth.", "7th.", "8th."};

char *GetSlotPlayerName(int nSlot) {
    NetPeerSlot &slot = g_aNetPeerSlots[nSlot];
    WaitForSingleObject(slot.hMutex, INFINITE);
    std::strcpy(g_szSlotPlayerNameBuffer, slot.szName);
    ReleaseMutex(slot.hMutex);
    return g_szSlotPlayerNameBuffer;
}

int SendTimeSyncReply(DirectPlayInfo *pInfo, unsigned timestamp) {
    NetStream stream;
    WriteTimestampToStream(&stream);
    stream.StreamWriteByte(static_cast<unsigned char>((g_dwLocalNetPlayerId & 0xf) | 0xd0));
    stream.StreamWriteInt24(timestamp);
    int result = SendNetworkStreamReliable(pInfo, &stream);
    NullStub();
    return result;
}

void SyncNetworkClockFromServer(unsigned serverFrame,
                                unsigned localTime,
                                unsigned echoedFrame,
                                unsigned roundTrip) {
    if (roundTrip <= g_dwNetBestClockRoundTrip && roundTrip != 0) {
        for (const NetClockSample &sample : g_aNetClockSamples) {
            if (sample.dwServerFrame == echoedFrame) {
                unsigned offset =
                    roundTrip / 2 - sample.dwLocalTime + localTime + sample.dwServerFrame;
                ResetFrameTimer();
                g_dwNetBestClockRoundTrip = roundTrip;
                g_dwNetClockBaseMs = GetFrameTickCounter() - offset;
            }
        }
    }
    g_aNetClockSamples[g_nNetClockSampleIndex].dwServerFrame = serverFrame;
    g_aNetClockSamples[g_nNetClockSampleIndex].dwLocalTime = localTime;
    g_nNetClockSampleIndex++;
    if (g_nNetClockSampleIndex >= 8) {
        g_nNetClockSampleIndex = 0;
    }
}

void LoadNetworkPlayerColors(void) {
    VEHICLE_GET(&(g_aVehicles[0]), Color)[2] = g_abPlayerColorsRgb[g_dwLocalNetPlayerId][2];
    VEHICLE_GET(&(g_aVehicles[0]), Color)[1] = g_abPlayerColorsRgb[g_dwLocalNetPlayerId][1];
    VEHICLE_GET(&(g_aVehicles[0]), Color)[0] = g_abPlayerColorsRgb[g_dwLocalNetPlayerId][0];
}

int SendPlayerJoinPacket(DirectPlayInfo *pInfo,
                         unsigned char player,
                         unsigned char red,
                         unsigned char green,
                         unsigned char blue,
                         const char *name,
                         unsigned char tag) {
    NetStream stream;
    stream.StreamResetReadWrite();
    stream.StreamWriteInt24(0);
    stream.StreamWriteByte(static_cast<unsigned char>((g_dwLocalNetPlayerId & 0xf) | tag | 0xe0));
    stream.StreamWriteByte(player);
    stream.StreamWriteByte(static_cast<unsigned char>(g_dwChosenCar));
    stream.StreamWriteByte(red);
    stream.StreamWriteByte(green);
    stream.StreamWriteByte(blue);
    bool ended = false;
    for (int i = 0; i < 24; i++) {
        if (ended) {
            stream.StreamWriteByte(0);
        } else {
            stream.StreamWriteByte(static_cast<unsigned char>(name[i]));
            if (name[i] == 0) {
                ended = true;
            }
        }
    }
    int result = SendNetworkStreamReliable(pInfo, &stream);
    NullStub();
    return result;
}

static inline void StoreOutgoingKeyframe(KeyframeState *state, unsigned type) {
    NetPeerSlot &slot = g_aNetPeerSlots[g_dwLocalNetPlayerId];
    HANDLE mutexes[2] = {g_hNetKeyframeMutex, slot.hMutex};
    WaitForMultipleObjects(2, mutexes, true, INFINITE);
    g_dwNetLastKeyframeFrame = GetElapsedFrameCount();
    slot.nCurrentSnapshot = static_cast<int>(static_cast<unsigned>(slot.nCurrentSnapshot) + 1u);
    if (slot.nCurrentSnapshot >= 8) {
        slot.nCurrentSnapshot = 0;
    }
    slot.adwSnapshotTimes[slot.nCurrentSnapshot] = g_dwNetLastKeyframeFrame;
    slot.adwSnapshotTypes[slot.nCurrentSnapshot] = type;
    slot.aSnapshots[slot.nCurrentSnapshot] = *state;
    ReleaseMutex(slot.hMutex);
    ReleaseMutex(g_hNetKeyframeMutex);
}

unsigned SendKeyframePacket(KeyframeState *state) {
    StoreOutgoingKeyframe(state, 0x40);
    NetStream stream;
    WriteTimestampToStream(&stream);
    stream.StreamWriteByte(static_cast<unsigned char>(g_dwLocalNetPlayerId) | 0x40);
    EncodeKeyframeBitstreamFull(&stream, state);
    int result = SendNetworkStreamUnreliable(&g_DirectPlayInfo, &stream);
    NullStub();
    return result != -1;
}

unsigned SendDeltaKeyframePacket(KeyframeState *state, KeyframeState *base) {
    StoreOutgoingKeyframe(state, 0x80);
    NetStream stream;
    WriteTimestampToStream(&stream);
    stream.StreamWriteByte(static_cast<unsigned char>(g_dwLocalNetPlayerId) | 0x80);
    EncodeKeyframeBitstreamDelta(&stream, state, base);
    int result = SendNetworkStreamUnreliable(&g_DirectPlayInfo, &stream);
    NullStub();
    return result != -1;
}

int SendNetworkStreamUnreliable(DirectPlayInfo *pInfo, NetStream *pStream) {
    if (g_dwNetworkDataStreamActive) {
        return pStream->StreamGetWriteSize();
    }
    if (pInfo->bIsHost) {
        unsigned size = pStream->StreamGetWriteSize();
        void *buffer = pStream->StreamGetBufferPtr();
        ProcessMultiplayerPackets(pInfo, buffer, size, 1, 1);
    }
    unsigned size = pStream->StreamGetWriteSize();
    void *buffer = pStream->StreamGetBufferPtr();
    HRESULT result =
        DirectPlaySendLocked(pInfo->pDirectPlay, pInfo->dpidPlayer, 0, 0, buffer, size);
    return result == 0 ? pStream->StreamGetWriteSize() : -1;
}

int SendNetworkStreamReliable(DirectPlayInfo *pInfo, NetStream *pStream) {
    if (!g_dwNetworkDataStreamActive) {
        if (pInfo->bIsHost) {
            ProcessMultiplayerPackets(pInfo,
                                      pStream->StreamGetBufferPtr(),
                                      pStream->StreamGetWriteSize(),
                                      DPID_SERVERPLAYER,
                                      DPID_SERVERPLAYER);
        } else if (DirectPlaySendLocked(pInfo->pDirectPlay,
                                        pInfo->dpidPlayer,
                                        DPID_SERVERPLAYER,
                                        0,
                                        pStream->StreamGetBufferPtr(),
                                        pStream->StreamGetWriteSize()) != 0) {
            return -1;
        }
    }
    return pStream->StreamGetWriteSize();
}

int SendPlayerInfoPacket(DirectPlayInfo *pInfo,
                         unsigned char bPlayer,
                         unsigned char bCar,
                         unsigned char bColorR,
                         unsigned char bColorG,
                         unsigned char bColorB,
                         DPID dpidOwner,
                         const char *szPlayerName,
                         int nState,
                         unsigned char bTag) {
    NetStream stream;
    WriteTimestampToStream(&stream);
    stream.StreamWriteByte(static_cast<unsigned char>(g_dwLocalNetPlayerId) | bTag);
    stream.StreamWriteByte(bPlayer);
    stream.StreamWriteByte(static_cast<unsigned char>(nState));
    stream.StreamWriteByte(bCar);
    stream.StreamWriteByte(bColorR);
    stream.StreamWriteByte(bColorG);
    stream.StreamWriteByte(bColorB);
    stream.StreamWriteInt32(dpidOwner);
    int state = nState & 0x3f;
    if (state <= 2) {
        bool ended = false;
        for (int i = 0; i < 24; i++) {
            if (ended) {
                stream.StreamWriteByte(0);
            } else {
                stream.StreamWriteByte(static_cast<unsigned char>(szPlayerName[i]));
                if (szPlayerName[i] == 0) {
                    ended = true;
                }
            }
        }
    }
    if (state == 4) {
        stream.StreamWriteInt24(g_dwNetRaceStartTimeMs);
    }
    int result = SendNetworkStreamReliable(pInfo, &stream);
    NullStub();
    return result;
}

HRESULT GetDirectPlayPlayerName(IDirectPlay3A *pDirectPlay, DPID dpidPlayer, DPNAME **ppName) {
    DWORD size = 0;
    DPNAME *name = nullptr;
    HRESULT result = pDirectPlay->GetPlayerName(dpidPlayer, nullptr, &size);
    if (result == DPERR_BUFFERTOOSMALL) {
        name = static_cast<DPNAME *>(GlobalLock(GlobalAlloc(GHND, size)));
        if (name == nullptr) {
            result = E_OUTOFMEMORY;
        } else {
            result = pDirectPlay->GetPlayerName(dpidPlayer, name, &size);
            if (SUCCEEDED(result)) {
                *ppName = name;
                return DP_OK;
            }
        }
    }
    if (name != nullptr) {
        GlobalUnlock(GlobalHandle(name));
        GlobalFree(GlobalHandle(name));
    }
    return result;
}

HRESULT FormatNetworkChatLine(
    IDirectPlay3A *pDirectPlay, DPID dpidPlayer, const char *text, char **ppLine, bool bNotice) {
    if (pDirectPlay == nullptr) {
        return DP_OK;
    }
    const char *format = bNotice ? "%s%s\r\n" : "%s: %s\r\n";
    DPNAME *name = nullptr;
    HRESULT result = GetDirectPlayPlayerName(pDirectPlay, dpidPlayer, &name);
    if (SUCCEEDED(result)) {
        const char *playerName = name->lpszShortNameA != nullptr ? name->lpszShortNameA : "unknown";
        DWORD size = lstrlenA(format) + lstrlenA(playerName) + lstrlenA(text) + 1;
        char *line = static_cast<char *>(GlobalLock(GlobalAlloc(GHND, size)));
        if (line == nullptr) {
            result = E_OUTOFMEMORY;
        } else {
            wsprintfA(line, format, playerName, text);
            *ppLine = line;
        }
    }
    if (name != nullptr) {
        GlobalUnlock(GlobalHandle(name));
        GlobalFree(GlobalHandle(name));
    }
    return result;
}

struct NetworkChatPacket {
    DWORD dwType;
    char szText[1];
};

HRESULT SendNetworkChatMessage(HWND hWindow, DirectPlayInfo *pInfo, bool bNotice) {
    if (!pInfo || !pInfo->pDirectPlay || g_dwNetworkSendBlocked) {
        return DP_OK;
    }
    char *line = nullptr;
    NetworkChatPacket *packet = nullptr;
    char *text = static_cast<char *>(GlobalLock(GlobalAlloc(GHND, 201)));
    HRESULT result;
    if (!text) {
        result = E_OUTOFMEMORY;
    } else {
        strcpy(text, g_szNetworkChatText);
        result = FormatNetworkChatLine(pInfo->pDirectPlay, pInfo->dpidPlayer, text, &line, bNotice);
        if (SUCCEEDED(result)) {
            PostMessageA(hWindow, WM_NETWORK_NOTICE, 0, reinterpret_cast<LPARAM>(line));
            line = nullptr;
            DWORD size = lstrlenA(text) + 8;
            packet = static_cast<NetworkChatPacket *>(GlobalLock(GlobalAlloc(GHND, size)));
            if (!packet) {
                result = E_OUTOFMEMORY;
            } else {
                packet->dwType = 0x60000000 + bNotice;
                lstrcpyA(packet->szText, text);
                result = DirectPlaySendLocked(pInfo->pDirectPlay,
                                              pInfo->dpidPlayer,
                                              DPID_ALLPLAYERS,
                                              DPSEND_GUARANTEED,
                                              packet,
                                              size);
            }
        }
    }
    if (text) {
        FreeGlobalBuffer(text);
    }
    if (line) {
        FreeGlobalBuffer(line);
    }
    if (packet) {
        FreeGlobalBuffer(packet);
    }
    return result;
}

struct NetworkSessionMessage {
    DWORD dwType;
    union {
        char szText[1];
        DWORD dwValue;
        struct {
            DPID dpidPlayer;
            int nPosition;
        } finish;
    };
};

void HandleMultiplayerSessionMessage(
    DirectPlayInfo *pInfo, void *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo) {
    if (g_dwNetworkSendBlocked) {
        if (!pInfo->bIgnoreMessages) {
            if (pInfo->bIsHost) {
                ProcessMultiplayerPackets(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
            } else {
                ProcessNetworkDataStream(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
            }
        }
        return;
    }
    NetworkSessionMessage *message = static_cast<NetworkSessionMessage *>(pMessage);
    char *line = nullptr;
    char debug[0x104];
    char text[0x400];
    switch (message->dwType) {
    case 0x60000000:
    case 0x60000001:
        FormatNetworkChatLine(
            pInfo->pDirectPlay, dpidFrom, message->szText, &line, message->dwType == 0x60000001);
        break;
    case 0x60000002:
        if (dpidFrom == DPID_SERVERPLAYER) {
            for (SessionSlot &slot : g_aSessionSlots) {
                if (slot.dpidPlayer == 0xffffffff) {
                    slot.dpidPlayer = DPID_SERVERPLAYER;
                    slot.nFlag = true;
                    std::sprintf(
                        debug, "Host set ready message received, ID: %d\n", DPID_SERVERPLAYER);
                    OutputDebugStringA(debug);
                    break;
                }
            }
        } else {
            for (SessionSlot &slot : g_aSessionSlots) {
                if (slot.dpidPlayer == dpidFrom) {
                    slot.nFlag = message->dwValue;
                    std::sprintf(debug,
                                 "APPMSG_PLAYERREADY: Set: %d, ID: %d\n",
                                 static_cast<int>(message->dwValue),
                                 static_cast<int>(dpidFrom));
                    OutputDebugStringA(debug);
                }
            }
        }
        break;
    case 0x60000003:
        g_dwNetworkSendBlocked = true;
        std::sprintf(debug, "APPMSG_HOSTSTARTED: received");
        OutputDebugStringA(debug);
        return;
    case 0x60000004:
        if (pInfo->dpidPlayer == message->dwValue) {
            g_dwNetworkDataStreamActive = true;
        }
        std::sprintf(debug, "APPMSG_BOOTPLAYER: received\n");
        OutputDebugStringA(debug);
        break;
    case 0x60000005:
        if (message->dwValue >= 4 && message->dwValue <= 7) {
            PlaySoundChannelOneOnce(message->dwValue, 0x200020);
            g_dwNetworkMenuState = message->dwValue;
        }
        break;
    case 0x60000006:
        if (dpidFrom == message->dwValue) {
            FormatNetworkChatLine(pInfo->pDirectPlay, dpidFrom, " left the race.", &line, true);
        }
        break;
    case 0x60000007:
        if (dpidFrom == message->dwValue) {
            FormatNetworkChatLine(
                pInfo->pDirectPlay, dpidFrom, " returned to race lobby.", &line, true);
        }
        break;
    case 0x60000008:
        if (dpidFrom == DPID_SERVERPLAYER && message->finish.nPosition >= 0 &&
            message->finish.nPosition < 8) {
            std::sprintf(text, " finished %s", g_apszPlaceNames[message->finish.nPosition]);
            FormatNetworkChatLine(
                pInfo->pDirectPlay, message->finish.dpidPlayer, text, &line, true);
        }
        break;
    default:
        break;
    }
    if (line != nullptr) {
        if (g_hMainWindow != nullptr) {
            PostMessageA(g_hMainWindow, WM_NETWORK_NOTICE, 0, reinterpret_cast<LPARAM>(line));
        } else {
            GlobalUnlock(GlobalHandle(line));
            GlobalFree(GlobalHandle(line));
        }
    }
}

static inline int FindNetworkVehicle(DPID player) {
    for (int i = 0; i < g_dwNumActiveVehicles; i++) {
        if (GetSlotPlayerId(VEHICLE_GET(&(g_aVehicles[i]), Index)) == player) {
            return i;
        }
    }
    return -1;
}

static inline void HandleClientSessionMessage(NetworkSessionMessage *message, DPID sender) {
    char text[0x400];
    int vehicle;
    switch (message->dwType) {
    case 0x60000000:
    case 0x60000001:
        text[0] = 0;
        vehicle = FindNetworkVehicle(sender);
        if (vehicle != -1) {
            std::strcpy(text, GetSlotPlayerName(VEHICLE_GET(&(g_aVehicles[vehicle]), Index)));
            std::strcat(text, ": ");
        }
        std::strcat(text, message->szText);
        SetGlobalStatusString(text);
        break;
    case 0x60000004:
        if (message->dwValue == GetSlotPlayerId(g_dwLocalNetPlayerId)) {
            DestroyLocalDirectPlayPlayer();
            g_dwNetworkDataStreamActive = true;
            vehicle = FindNetworkVehicle(sender);
            if (vehicle != -1) {
                std::strcpy(text, GetSlotPlayerName(VEHICLE_GET(&(g_aVehicles[vehicle]), Index)));
                std::strcat(text, " booted you from the game.");
                SetGlobalStatusString(text);
            }
        }
        break;
    case 0x60000005:
        WaitForSingleObject(g_hNetNotifyMutex, INFINITE);
        if (message->dwValue >= 4 && message->dwValue <= 7) {
            g_nPendingNetSoundEffect = message->dwValue + 16;
        }
        ReleaseMutex(g_hNetNotifyMutex);
        break;
    case 0x60000006:
        if (sender == message->dwValue) {
            vehicle = FindNetworkVehicle(sender);
            if (vehicle != -1) {
                int slot = VEHICLE_GET(&(g_aVehicles[vehicle]), Index);
                if ((GetSlotStatusThreadSafe(slot) & 0x3f) != 0) {
                    WaitForSingleObject(g_hDepartedPlayerStackMutex, INFINITE);
                    g_anDepartedPlayerSlotStack[++g_nDepartedPlayerStackTop] = slot;
                    ReleaseMutex(g_hDepartedPlayerStackMutex);
                    // The original clears the vehicle index rather than the slot index.
                    SetSlotStatusThreadSafe(vehicle, 0);
                    std::strcpy(text, GetSlotPlayerName(slot));
                    std::strcat(text, " left the race.");
                    SetGlobalStatusString(text);
                }
            }
        }
        break;
    case 0x60000007:
        if (sender == message->dwValue) {
            vehicle = FindNetworkVehicle(sender);
            if (vehicle != -1) {
                std::strcpy(text, GetSlotPlayerName(VEHICLE_GET(&(g_aVehicles[vehicle]), Index)));
                std::strcat(text, " returned to race lobby.");
                SetGlobalStatusString(text);
            }
        }
        break;
    case 0x60000008:
        if (sender == DPID_SERVERPLAYER) {
            vehicle = FindNetworkVehicle(message->finish.dpidPlayer);
            if (vehicle != -1 && message->finish.nPosition >= 0 && message->finish.nPosition < 8) {
                std::sprintf(text,
                             "%s finished %s",
                             GetSlotPlayerName(VEHICLE_GET(&(g_aVehicles[vehicle]), Index)),
                             g_apszPlaceNames[message->finish.nPosition]);
                SetGlobalStatusString(text);
            }
        }
        break;
    default:
        break;
    }
}

static inline void ResetPeerSnapshotsForJoin(NetPeerSlot &slot) {
    slot.dwLastUpdateTime = 0;
    std::memset(slot.adwSnapshotTimes, 0, sizeof(slot.adwSnapshotTimes));
    slot.dwStatus = 0;
    std::memset(slot.adwSnapshotTypes, 0, sizeof(slot.adwSnapshotTypes));
    std::memset(slot.aSnapshots, 0, sizeof(slot.aSnapshots));
    slot.nSnapshotCount = 0;
    slot.nCurrentSnapshot = 0;
    slot.nAppliedSnapshot = -1;
    slot.dwPredictionFrame = 0;
    slot.nRepeatedSnapshotFrames = 0;
    slot.nLateSnapshotFrames = 0;
    slot.dwPingMs = 0;
}

bool ResetNetworkState(void) {
    for (NetPeerSlot &slot : g_aNetPeerSlots) {
        WaitForSingleObject(slot.hMutex, INFINITE);
        slot.dwLastUpdateTime = 0;
        std::memset(slot.adwSnapshotTimes, 0, sizeof(slot.adwSnapshotTimes));
        slot.dwStatus = 0;
        std::memset(slot.adwSnapshotTypes, 0, sizeof(slot.adwSnapshotTypes));
        slot.nSnapshotCount = 0;
        slot.nCurrentSnapshot = 0;
        slot.nAppliedSnapshot = -1;
        slot.dwPredictionFrame = 0;
        slot.nRepeatedSnapshotFrames = 0;
        slot.nLateSnapshotFrames = 0;
        slot.dwPingMs = 0;
        ReleaseMutex(slot.hMutex);
    }
    WaitForSingleObject(g_hNetNotifyMutex, INFINITE);
    g_nPendingNetSoundEffect = -1;
    g_bNetStatusStringDirty = 0;
    g_szNetStatusString[0] = '\0';
    ReleaseMutex(g_hNetNotifyMutex);
    WaitForSingleObject(g_hNetRaceOrderMutex, INFINITE);
    std::memset(g_adwNetRaceOrder, 0, sizeof(g_adwNetRaceOrder));
    ReleaseMutex(g_hNetRaceOrderMutex);
    WaitForSingleObject(g_hJoinedPlayerStackMutex, INFINITE);
    g_nJoinedPlayerStackTop = -1;
    ReleaseMutex(g_hJoinedPlayerStackMutex);
    WaitForSingleObject(g_hDepartedPlayerStackMutex, INFINITE);
    g_nDepartedPlayerStackTop = -1;
    ReleaseMutex(g_hDepartedPlayerStackMutex);
    WaitForSingleObject(g_hNetReadyMutex, INFINITE);
    g_bNetReadyToStart = 0;
    ReleaseMutex(g_hNetReadyMutex);
    WaitForSingleObject(g_hNetPauseMutex, INFINITE);
    g_nNetPauseRequestTicks = 0;
    ReleaseMutex(g_hNetPauseMutex);
    WaitForSingleObject(g_hNetUnpauseMutex, INFINITE);
    g_nNetUnpauseRequestTicks = 0;
    ReleaseMutex(g_hNetUnpauseMutex);
    WaitForSingleObject(g_hNetReceiveMutex, INFINITE);
    g_bNetReceiveEnabled = 0;
    ReleaseMutex(g_hNetReceiveMutex);
    WaitForSingleObject(g_hNetRaceStartMutex, INFINITE);
    g_dwNetRaceStartFrame = ~0u;
    ReleaseMutex(g_hNetRaceStartMutex);
    g_dwNetClockBaseMs = 0;
    g_dwNetBestClockRoundTrip = ~0u;
    for (NetClockSample &sample : g_aNetClockSamples) {
        sample.dwServerFrame = 0;
        sample.dwLocalTime = 0;
    }
    g_bNetGamePaused = 0;
    g_dwNetworkDataReceived = 0;
    return true;
}

bool InitializeNetworkMutexes(int) {
    for (NetPeerSlot &slot : g_aNetPeerSlots) {
        slot.hMutex = CreateMutexA(nullptr, false, nullptr);
    }
    g_hNetNotifyMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetRaceOrderMutex = CreateMutexA(nullptr, false, nullptr);
    g_hJoinedPlayerStackMutex = CreateMutexA(nullptr, false, nullptr);
    g_hDepartedPlayerStackMutex = CreateMutexA(nullptr, false, nullptr);
    g_dwNetLastKeyframeFrame = 0;
    g_dwNetClockBaseMs = 0;
    ResetFrameTimer();
    g_hNetKeyframeMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetReadyMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetPauseMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetUnpauseMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetReceiveMutex = CreateMutexA(nullptr, false, nullptr);
    g_hNetRaceStartMutex = CreateMutexA(nullptr, false, nullptr);
    ResetNetworkState();
    return true;
}

static inline void ApplyLocalNetworkStatus(int state) {
    if ((state & 0x3f) == 3) {
        WaitForSingleObject(g_hNetReadyMutex, INFINITE);
        g_bNetReadyToStart = false;
        ReleaseMutex(g_hNetReadyMutex);
    }
    if ((state & 0x80) != 0) {
        WaitForSingleObject(g_hNetPauseMutex, INFINITE);
        g_nNetPauseRequestTicks = 0;
        ReleaseMutex(g_hNetPauseMutex);
        g_bNetGamePaused = true;
    }
    if ((state & 0x40) != 0) {
        if (g_bNetGamePaused) {
            g_bNetGamePaused = false;
        }
        if (g_dwRacePauseState) {
            g_dwNetworkDataReceived = true;
        }
        WaitForSingleObject(g_hNetUnpauseMutex, INFINITE);
        g_nNetUnpauseRequestTicks = 0;
        ReleaseMutex(g_hNetUnpauseMutex);
    }
    if ((state & 0x3f) == 4) {
        WaitForSingleObject(g_hNetRaceStartMutex, INFINITE);
        unsigned frame = GetElapsedFrameCount();
        g_dwNetRaceStartFrame = g_dwNetRaceStartTimeMs / TICK_PERIOD_MS * TICKS_PER_PERIOD +
                                g_dwNetRaceStartTimeMs % TICK_PERIOD_MS / TICK_INTERVAL_MS;
        if (g_dwNetRaceStartFrame > frame + 90) {
            g_dwNetRaceStartFrame = frame + 90;
        }
        ReleaseMutex(g_hNetRaceStartMutex);
    }
}

static inline void
ReadClientSnapshot(NetPeerSlot &slot, NetStream &stream, unsigned timestamp, bool delta) {
    if ((slot.dwStatus & 0x3f) >= 2) {
        slot.nSnapshotCount++;
        if (delta) {
            ReadNetworkDeltaSnapshot(slot, stream, timestamp);
        } else {
            slot.nCurrentSnapshot = FindOldestNetworkSnapshot(slot);
            slot.adwSnapshotTimes[slot.nCurrentSnapshot] = timestamp;
            slot.adwSnapshotTypes[slot.nCurrentSnapshot] = 0x40;
            DeserializeVehicleState(&slot.aSnapshots[slot.nCurrentSnapshot], &stream);
        }
        int elapsed = static_cast<int>((g_dwNetLastKeyframeFrame - timestamp) * 2u);
        slot.dwPingMs = static_cast<int>(static_cast<unsigned>(elapsed / 3) * 100 +
                                         static_cast<unsigned>(elapsed % 3) * 33);
    } else {
#ifdef ENABLE_PATCHES
        KeyframeState discarded = {};
#else
        KeyframeState discarded;
#endif /* ENABLE_PATCHES */
        if (delta) {
            // The original consumes an inactive peer's delta with an uninitialised reference.
            DeserializeKeyframeDelta(&discarded, &discarded, &stream);
        } else {
            DeserializeVehicleState(&discarded, &stream);
        }
    }
}

void ProcessNetworkDataStream(
    DirectPlayInfo *pInfo, void *pData, DWORD dwSize, DPID dpidFrom, DPID) {
    NetStream stream;
    stream.StreamAttachBuffer(static_cast<unsigned char *>(pData), dwSize);
    WaitForSingleObject(g_hNetReceiveMutex, INFINITE);
    bool receive = g_bNetReceiveEnabled != 0;
    ReleaseMutex(g_hNetReceiveMutex);
    // Every return passes through the NullStub destructor of the stream.
    if (!receive) {
        NullStub();
        return;
    }
    stream.StreamResetRead();
    unsigned timestamp = stream.StreamReadInt24();
    WaitForSingleObject(g_hNetKeyframeMutex, INFINITE);
    g_dwNetLastKeyframeFrame = GetElapsedFrameCount();
    ReleaseMutex(g_hNetKeyframeMutex);
    while (!stream.StreamIsAtEnd()) {
        unsigned header = stream.StreamReadByte();
        int player = header & 0xf;
        NetPeerSlot &slot = g_aNetPeerSlots[player];
        switch (header & 0xe0) {
        case 0: {
            if (dpidFrom != DPID_SERVERPLAYER) {
                NullStub();
                return;
            }
            int state = stream.StreamReadByte();
            int car = stream.StreamReadByte();
            unsigned char red = stream.StreamReadByte();
            unsigned char green = stream.StreamReadByte();
            unsigned char blue = stream.StreamReadByte();
            unsigned owner = stream.StreamReadInt32();
            char name[25];
            bool hasName = (state & 0x3f) <= 2;
            if (hasName) {
                for (int i = 0; i < 24; i++) {
                    name[i] = static_cast<char>(stream.StreamReadByte());
                }
                name[24] = 0;
            }
            if ((state & 0x3f) == 4) {
                g_dwNetRaceStartTimeMs = stream.StreamReadInt24();
            }
            WaitForSingleObject(slot.hMutex, INFINITE);
            if (player != g_dwLocalNetPlayerId && slot.dwStatus != state && (state & 0x3f) == 2) {
                WaitForSingleObject(g_hJoinedPlayerStackMutex, INFINITE);
                g_anJoinedPlayerSlotStack[++g_nJoinedPlayerStackTop] = player;
                ReleaseMutex(g_hJoinedPlayerStackMutex);
                ResetPeerSnapshotsForJoin(slot);
            }
            if (player == g_dwLocalNetPlayerId) {
                ApplyLocalNetworkStatus(state);
            }
            slot.dwLastUpdateTime = timestamp;
            slot.dwStatus = state;
            slot.nCar = car;
            slot.color[0] = red;
            slot.color[1] = green;
            slot.color[2] = blue;
            slot.dpidPlayer = owner;
            if (hasName) {
                std::strcpy(slot.szName, name);
            }
            SendPlayerInfoPacket(pInfo,
                                 static_cast<unsigned char>(player),
                                 static_cast<unsigned char>(car),
                                 red,
                                 green,
                                 blue,
                                 owner,
                                 name,
                                 state,
                                 0x10);
            ReleaseMutex(slot.hMutex);
            break;
        }
        case 0x20:
            if (dpidFrom != DPID_SERVERPLAYER) {
                NullStub();
                return;
            }
            WaitForSingleObject(g_hNetRaceOrderMutex, INFINITE);
            for (int i = 0; i < NET_PEER_SLOT_COUNT; i += 2) {
                unsigned ranks = stream.StreamReadByte();
                g_adwNetRaceOrder[i] = ranks & 0xf;
                g_adwNetRaceOrder[i + 1] = ranks >> 4;
            }
            ReleaseMutex(g_hNetRaceOrderMutex);
            break;
        case 0x40:
        case 0x80:
            WaitForSingleObject(slot.hMutex, INFINITE);
            ReadClientSnapshot(slot, stream, timestamp, (header & 0xe0) == 0x80);
            ReleaseMutex(slot.hMutex);
            break;
        case 0x60:
            HandleClientSessionMessage(static_cast<NetworkSessionMessage *>(pData), dpidFrom);
            NullStub();
            return;
        case 0xc0: {
            if (dpidFrom != DPID_SERVERPLAYER) {
                NullStub();
                return;
            }
            unsigned localTime = GetHighResolutionTime();
            unsigned raceTime = stream.StreamReadInt24();
            unsigned roundTrip = stream.StreamReadInt16();
            WaitForSingleObject(slot.hMutex, INFINITE);
            slot.dwRaceTime = raceTime;
            slot.dwPingMs = roundTrip;
            ReleaseMutex(slot.hMutex);
            if (player == g_dwLocalNetPlayerId) {
                SendTimeSyncReply(pInfo, timestamp);
                SyncNetworkClockFromServer(timestamp, localTime, raceTime, roundTrip);
            }
            break;
        }
        case 0xe0:
            if (dpidFrom != DPID_SERVERPLAYER) {
                NullStub();
                return;
            }
            g_dwLocalNetPlayerId = player;
            LoadNetworkPlayerColors();
            g_bNetMaxPlayers = stream.StreamReadByte();
            g_dwSelectedTrackId = stream.StreamReadByte();
            g_dwChosenWeather = stream.StreamReadByte();
            g_wNetAllowedCarsMask = stream.StreamReadInt16();
            g_bNetGameSettingUnk_10221280 = stream.StreamReadByte();
            g_bNetTireSetting = stream.StreamReadByte();
            g_bNetSuspensionSetting = stream.StreamReadByte();
            SendPlayerJoinPacket(pInfo,
                                 static_cast<unsigned char>(player),
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[0],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[1],
                                 VEHICLE_GET(&(g_aVehicles[0]), Color)[2],
                                 g_GlobalRallyConfig.szPlayerName,
                                 0x10);
            SetSlotStatusThreadSafe(player, 2);
            break;
        default:
            std::fprintf(stderr,
                         "Error: unknown command %02X received from server (player %d).\n",
                         header & 0xe0,
                         player);
            NullStub();
            return;
        }
    }
    NullStub();
}

void DestroyNetworkGroups(void) {
    if (g_pJoinCreateGameMenu != nullptr) {
        unsigned count = g_pJoinCreateGameMenu->GetItemList().GetItemCount();
        // The original advances the index even though removal shifts subsequent entries.
        for (unsigned i = 0; i < count; i++) {
            MenuItemList &list = g_pJoinCreateGameMenu->GetItemList();
            list.RemoveItem(i);
        }
    }
}

HRESULT EnumerateNetworkPlayers(GUID *instance,
                                LPDPENUMPLAYERSCALLBACK2 callback,
                                void *context,
                                DWORD flags) {
    HRESULT result = E_FAIL;
    if (g_pNetworkPlayerListMenu != nullptr) {
        RefreshSessionListSlots();
    }
    for (SessionSlot &slot : g_aSessionSlots) {
        slot.nSeen = 0;
    }
    if (g_DirectPlayInfo.pDirectPlay != nullptr) {
        result = g_DirectPlayInfo.pDirectPlay->EnumPlayers(instance, callback, context, flags);
    }
    PurgeStaleSessionListEntries();
    DWORD ready = GetSessionSlotFlag(g_pDirectPlayInfo->dpidPlayer);
    SendNetworkReadyMessage(g_pDirectPlayInfo, ready);
    return result;
}

BOOL CALLBACK EnumSessionsCallback_AddSessionToList(LPCDPSESSIONDESC2 description,
                                                    DWORD *,
                                                    DWORD flags,
                                                    void *context) {
    if (g_pJoinCreateGameMenu == nullptr || (flags & DPESC_TIMEDOUT)) {
        return false;
    }
    GUID *instance;
    if (g_dwSessionEnumPending) {
        bool available =
            description->dwCurrentPlayers < 8 && !(description->dwFlags & DPSESSION_JOINDISABLED);
        MenuItemList &list = g_pJoinCreateGameMenu->GetItemList();
        list.AddItem(description->lpszSessionNameA,
                     available ? 1 : 0x11,
                     available,
                     &g_rcMenuCenterPanel,
                     true);
        instance = static_cast<GUID *>(
            GlobalLock(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(GUID))));
        if (instance == nullptr) {
            return true;
        }
        *instance = description->guidInstance;
        g_pJoinCreateGameMenu->GetItemList().SetItemData(instance, sizeof(GUID), -1);
    } else {
        // Without enumeration, the original passes the callback flags as the instance pointer.
        instance = std::bit_cast<GUID *>(flags);
    }
    EnumerateNetworkPlayers(instance, EnumPlayersCallback_AddPlayerToList, context, 0);
    return true;
}

HRESULT RefreshNetworkSessionList(IDirectPlay3A *pDirectPlay) {
    if (pDirectPlay == nullptr) {
        return DPERR_INVALIDOBJECT;
    }
    DestroyNetworkGroups();
    DPSESSIONDESC2 description = {};
    description.dwSize = sizeof(description);
    description.guidApplication = g_BossRallyAppGuid;
    g_dwSessionListRefreshing = 1;
    // Without an enumeration, the interface pointer remains the return value.
    // The cast assumes a 32-bit pointer.
    HRESULT result = std::bit_cast<HRESULT>(pDirectPlay);
    if (g_dwSessionEnumPending) {
        result = pDirectPlay->EnumSessions(&description,
                                           0,
                                           EnumSessionsCallback_AddSessionToList,
                                           g_hMainWindow,
                                           DPENUMSESSIONS_AVAILABLE | DPENUMSESSIONS_ASYNC |
                                               DPENUMSESSIONS_RETURNSTATUS);
    }
    EnumerateNetworkPlayers(
        &description.guidInstance, EnumPlayersCallback_AddPlayerToList, g_hMainWindow, 0);
    g_dwSessionListRefreshing = 0;
    if (g_pNetworkSessionJoinMenu != nullptr &&
        g_pJoinCreateGameMenu->GetItemList().GetItemCount() != 0 &&
        !(g_pJoinCreateGameMenu->GetItemList().GetFont(g_dwSelectedSessionIndex).GetFlags() &
          0x10)) {
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10u);
        g_pNetworkSessionJoinMenu->GetFont(0).SetStyle(1);
    } else {
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() | 0x10u);
        g_pNetworkSessionJoinMenu->GetFont(0).SetStyle(0);
    }
    return result;
}

void DisconnectNetworkSession(void) {
    ResetSessionSlotTable();
    KillTimer(g_hMainWindow, g_dwNetworkTimerId);
    if (g_dwLobbyIsHost) {
        StopNetworkWorkerThread();
    }
    CloseAndReleaseDirectPlay();
    if (g_dwServiceProviderIndex != SERVICE_PROVIDER_MODEM &&
        g_dwServiceProviderIndex != SERVICE_PROVIDER_SERIAL &&
        g_pNetworkSessionJoinMenu != nullptr) {
        g_pNetworkSessionJoinMenu->GetFont(0).SetStyle(false);
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10);
    }
    g_dwDirectPlaySessionActive = false;
    g_dwLobbyIsHost = false;
    g_dwNetworkPlayMode = 0;
    g_dwNetworkCarSetupShown = false;
}

LRESULT WINAPI HandleNetworkWindowMessage(HWND, UINT nMessage, WPARAM, LPARAM lParam) {
    if (nMessage == 0x501) {
        char *text = reinterpret_cast<char *>(lParam);
        if (g_pNetworkCarSetupMenu != nullptr) {
            MenuItemList &list = g_pNetworkCarSetupMenu->GetItemList();
            list.AddItem(text, 0, 1, &g_rcNetCarSetupPlayerList, true);
        }
        GlobalUnlock(GlobalHandle(text));
        GlobalFree(GlobalHandle(text));
    } else if (nMessage == WM_TIMER) {
        if (g_pJoinCreateGameMenu != nullptr) {
            RefreshNetworkSessionList(g_DirectPlayInfo.pDirectPlay);
        }
        if (!g_dwLobbyIsHost) {
            LoadNetworkSettings();
        }
    }
    return 0;
}

int GetLocalPlayerIndex(void) {
    return g_dwLocalNetPlayerId;
}

bool InitServiceProviderAndStartTimer(void) {
    InitializeNetworkServiceProvider();
    g_dwNetworkTimerId = SetTimer(g_hMainWindow, NETWORK_TIMER_ID, NETWORK_TIMER_PERIOD, nullptr);
    g_dwDirectPlaySessionActive = true;
    return true;
}

HRESULT CreateServiceProviderAddress(void **ppAddress, DWORD *pdwSize) {
    GUID *pGuid = nullptr;
    HRESULT result = GetSelectedServiceProvider(&pGuid);
    void *address = nullptr;
    DWORD size = 0;
    DPCOMPOUNDADDRESSELEMENT elements[3];
    DWORD count = 1;
    if (SUCCEEDED(result)) {
        elements[0].guidDataType = DPAID_ServiceProvider;
        elements[0].dwDataSize = sizeof(GUID);
        if (IsEqualGUID(*pGuid, DPSPGUID_MODEM)) {
            elements[0].lpData = const_cast<GUID *>(&DPSPGUID_MODEM);
            if (std::strlen(g_szModemDeviceName) > 1) {
                elements[count].guidDataType = DPAID_Modem;
                elements[count].dwDataSize = lstrlenA(g_szModemDeviceName) + 1;
                elements[count++].lpData = g_szModemDeviceName;
            }
            elements[count].guidDataType = DPAID_Phone;
            elements[count].dwDataSize = lstrlenA(g_szModemPhoneNumber) + 1;
            elements[count++].lpData = g_szModemPhoneNumber;
        } else if (IsEqualGUID(*pGuid, DPSPGUID_TCPIP)) {
            elements[0].lpData = const_cast<GUID *>(&DPSPGUID_TCPIP);
            if (std::strlen(g_GlobalRallyConfig.szTcpIpAddress) <= 1) {
                lstrcpyA(g_GlobalRallyConfig.szTcpIpAddress, g_szEmptyUiString);
            }
            elements[1].guidDataType = DPAID_INet;
            elements[1].dwDataSize = lstrlenA(g_GlobalRallyConfig.szTcpIpAddress) + 1;
            elements[1].lpData = g_GlobalRallyConfig.szTcpIpAddress;
            if (std::strlen(g_GlobalRallyConfig.szTcpIpPort) <= 1) {
                lstrcpyA(g_GlobalRallyConfig.szTcpIpPort, g_szEmptyUiString);
            }
            elements[2].guidDataType = DPAID_INetPort;
            // The original supplies the port as a string, including its terminator.
            elements[2].dwDataSize = lstrlenA(g_GlobalRallyConfig.szTcpIpPort) + 1;
            elements[2].lpData = g_GlobalRallyConfig.szTcpIpPort;
            count = 3;
        } else if (IsEqualGUID(*pGuid, DPSPGUID_IPX)) {
            elements[0].lpData = const_cast<GUID *>(&DPSPGUID_IPX);
        } else {
            // The original passes the address of the GUID pointer for an unknown provider.
            elements[0].lpData = &pGuid;
        }
        result = g_pDirectPlayLobby->CreateCompoundAddress(elements, count, nullptr, &size);
        if (result == DPERR_BUFFERTOOSMALL) {
            address = GlobalLock(GlobalAlloc(GHND, size));
            if (address == nullptr) {
                result = E_OUTOFMEMORY;
            } else {
                result = g_pDirectPlayLobby->CreateCompoundAddress(elements, count, address, &size);
                if (SUCCEEDED(result)) {
                    *ppAddress = address;
                    *pdwSize = size;
                    return DP_OK;
                }
            }
        }
    }
    if (address != nullptr) {
        GlobalUnlock(GlobalHandle(address));
        GlobalFree(GlobalHandle(address));
    }
    return result;
}

void InitializeNetworkServiceProvider(void) {
    void *address = nullptr;
    DWORD size = 0;
    HRESULT result;
    char error[0x400];
    KillTimer(g_hMainWindow, g_dwNetworkTimerId);
    CloseAndReleaseDirectPlay();
    result = CreateServiceProviderAddress(&address, &size);
    if (address != nullptr) {
        result = CreateDirectPlayObject(&g_DirectPlayInfo.pDirectPlay);
        g_dwDirectPlayInstanceCount++;
        if (SUCCEEDED(result) && g_DirectPlayInfo.pDirectPlay != nullptr) {
            result = g_DirectPlayInfo.pDirectPlay->InitializeConnection(address, 0);
            if (SUCCEEDED(result)) {
                if (g_dwServiceProviderIndex != SERVICE_PROVIDER_MODEM &&
                    g_dwServiceProviderIndex != SERVICE_PROVIDER_SERIAL) {
                    if (g_pJoinCreateGameMenu != nullptr) {
                        result = RefreshNetworkSessionList(g_DirectPlayInfo.pDirectPlay);
                        if (FAILED(result)) {
                            goto finished;
                        }
                    }
                    g_dwNetworkTimerId =
                        SetTimer(g_hMainWindow, NETWORK_TIMER_ID, NETWORK_TIMER_PERIOD, nullptr);
                    g_dwDirectPlaySessionActive = true;
                }
                if (g_DirectPlayInfo.hPlayerEvent == nullptr) {
                    g_DirectPlayInfo.hPlayerEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                    if (g_DirectPlayInfo.hPlayerEvent != nullptr) {
                        return;
                    }
                    result = E_OUTOFMEMORY;
                } else {
                    return;
                }
            }
        }
    }
finished:
    if (result != DPERR_USERCANCEL) {
        // The message is formatted and not shown.
        std::sprintf(error,
                     "Could not select service provider because of error 0x%08X",
                     static_cast<unsigned>(result));
    }
}

bool InitServiceProviderAndStartSession(void) {
    InitializeNetworkServiceProvider();
    g_dwNetworkTimerId = SetTimer(g_hMainWindow, NETWORK_TIMER_ID, NETWORK_TIMER_PERIOD, nullptr);
    g_dwDirectPlaySessionActive = true;
    if (g_pJoinCreateGameMenu != nullptr) {
        // The result of the enumeration is discarded.
        RefreshNetworkSessionList(g_DirectPlayInfo.pDirectPlay);
    }
    return true;
}

HRESULT CloseAndReleaseDirectPlay(void) {
    DestroyNetworkGroups();
    if (g_pDirectPlayInfo->pDirectPlay != nullptr) {
        if (g_pDirectPlayInfo->dpidPlayer != 0) {
            g_pDirectPlayInfo->pDirectPlay->DestroyPlayer(g_pDirectPlayInfo->dpidPlayer);
            g_pDirectPlayInfo->dpidPlayer = 0;
        }
        g_pDirectPlayInfo->pDirectPlay->Close();
        g_pDirectPlayInfo->pDirectPlay->Release();
        g_pDirectPlayInfo->pDirectPlay = nullptr;
    }
    g_DirectPlayInfo.pDirectPlay = nullptr;
    g_dwDirectPlayInstanceCount--;
    return DP_OK;
}

char g_szNetStatusString[0x400];
int g_bNetStatusStringDirty;

void ProcessPendingNetworkNotifications(void) {
    WaitForSingleObject(g_hNetNotifyMutex, INFINITE);
    if (g_nPendingNetSoundEffect >= 0) {
        PlaySoundChannelOneOnce(g_nPendingNetSoundEffect, 0x200020);
        g_nPendingNetSoundEffect = -1;
    }
    if (g_bNetStatusStringDirty != 0) {
        g_pszDebugOverlayMessage = g_szNetStatusString;
        g_fDebugOverlayMessageTimer = 3.0f;
        g_bNetStatusStringDirty = 0;
    }
    ReleaseMutex(g_hNetNotifyMutex);
}

unsigned ReadPlayerSlotColorRgb(int slotIndex,
                                unsigned char *red,
                                unsigned char *green,
                                unsigned char *blue) {
    NetPeerSlot &slot = g_aNetPeerSlots[slotIndex];
    WaitForSingleObject(slot.hMutex, INFINITE);
    unsigned car = slot.nCar;
    *red = slot.color[0];
    *green = slot.color[1];
    *blue = slot.color[2];
    ReleaseMutex(slot.hMutex);
    return car;
}
void AddNetworkPlayerSlot(int slotIndex) {
    auto &color = g_aVehicles[g_dwNumActiveVehicles].GetColor();
    unsigned car = ReadPlayerSlotColorRgb(slotIndex, &color[0], &color[1], &color[2]);
    g_aVehicles[g_dwNumActiveVehicles].SetVehicleCarId(static_cast<s32>(car));
    Vehicle &namedVehicle = g_aVehicles[g_dwNumActiveVehicles];
    const char *name = GetSlotPlayerName(slotIndex);
    int player = g_dwNumActiveVehicles;
    std::strcpy(namedVehicle.GetPlayerName(), name);
    ReinitPlayerEngineSoundChannel(player, namedVehicle.GetCarModel());
    Vehicle &vehicle = g_aVehicles[g_dwNumActiveVehicles];
    g_dwNumActiveVehicles = static_cast<s32>(static_cast<u32>(g_dwNumActiveVehicles) + 1u);
    vehicle.SetGridIndex(slotIndex);
    g_dwNumVehicles = static_cast<s32>(static_cast<u32>(g_dwNumVehicles) + 1u);
}

void RemoveNetworkPlayerById(int slotIndex) {
    for (int index = 0; index < g_dwNumActiveVehicles; index++) {
        Vehicle &vehicle = g_aVehicles[index];
        if (vehicle.GetGridIndex() == slotIndex) {
            vehicle.ClearDriver();
            StopAudioChannel(index * 2);
            const int count = g_dwNumVehicles;
            for (int racer = 0; racer < count; racer++) {
                if (g_aRacerSlots[racer].GetVehicle() == &vehicle) {
                    g_aRacerSlots[racer].SetVehicle(nullptr);
                }
            }
        }
    }
}

int g_anDepartedPlayerSlotStack[PLAYER_SLOT_STACK_SIZE];
int g_nDepartedPlayerStackTop;
int g_anJoinedPlayerSlotStack[PLAYER_SLOT_STACK_SIZE];
int g_nJoinedPlayerStackTop;

static inline int PopPlayerSlot(HANDLE hMutex, const int *pnStack, int *pnTop) {
    int nSlot;

    WaitForSingleObject(hMutex, INFINITE);
    if (*pnTop < 0) {
        nSlot = -1;
    } else {
        nSlot = pnStack[(*pnTop)--];
    }
    ReleaseMutex(hMutex);
    return nSlot;
}

int PopJoinedPlayerSlot(void) {
    return PopPlayerSlot(
        g_hJoinedPlayerStackMutex, g_anJoinedPlayerSlotStack, &g_nJoinedPlayerStackTop);
}

int PopDepartedPlayerSlot(void) {
    return PopPlayerSlot(
        g_hDepartedPlayerStackMutex, g_anDepartedPlayerSlotStack, &g_nDepartedPlayerStackTop);
}

// The status of a slot of a peer in its low 6 bits.
#define NET_PEER_STATUS_MASK 0x3f

// The status of a slot below which GetNormalizedSlotStatusFlags() gives 0.
#define NET_PEER_STATUS_BASE 4

int GetNormalizedSlotStatusFlags(int nSlot) {
    NetPeerSlot &slot = g_aNetPeerSlots[nSlot];
    // The mutex is taken around a read that takes it again.
    WaitForSingleObject(slot.hMutex, INFINITE);
    int nStatus = (GetSlotStatusThreadSafe(nSlot) & NET_PEER_STATUS_MASK) - NET_PEER_STATUS_BASE;
    ReleaseMutex(slot.hMutex);
    return nStatus > 0 ? nStatus : 0;
}

DPID GetSlotPlayerId(int nSlot) {
    DPID dpidPlayer;

    WaitForSingleObject(g_aNetPeerSlots[nSlot].hMutex, INFINITE);
    dpidPlayer = g_aNetPeerSlots[nSlot].dpidPlayer;
    ReleaseMutex(g_aNetPeerSlots[nSlot].hMutex);
    return dpidPlayer;
}

int GetSlotStatusThreadSafe(int nSlot) {
    int dwStatus;

    WaitForSingleObject(g_aNetPeerSlots[nSlot].hMutex, INFINITE);
    dwStatus = g_aNetPeerSlots[nSlot].dwStatus;
    ReleaseMutex(g_aNetPeerSlots[nSlot].hMutex);
    return dwStatus;
}

void SetSlotStatusThreadSafe(int nSlot, int dwStatus) {
    WaitForSingleObject(g_aNetPeerSlots[nSlot].hMutex, INFINITE);
    g_aNetPeerSlots[nSlot].dwStatus = dwStatus;
    ReleaseMutex(g_aNetPeerSlots[nSlot].hMutex);
}

void SetGlobalStatusString(const char *pszStatus) {
    WaitForSingleObject(g_hNetNotifyMutex, INFINITE);
    strcpy(g_szNetStatusString, pszStatus);
    g_bNetStatusStringDirty = true;
    ReleaseMutex(g_hNetNotifyMutex);
}

void HandlePlayerDisconnect(DPID dpidPlayer) {
    char szStatus[0x400];

    for (int nSlot = 0; nSlot < NET_PEER_SLOT_COUNT; nSlot++) {
        if (GetSlotPlayerId(nSlot) != dpidPlayer ||
            (GetSlotStatusThreadSafe(nSlot) & NET_PEER_STATUS_MASK) == 0) {
            continue;
        }
        WaitForSingleObject(g_hDepartedPlayerStackMutex, INFINITE);
        g_anDepartedPlayerSlotStack[++g_nDepartedPlayerStackTop] = nSlot;
        ReleaseMutex(g_hDepartedPlayerStackMutex);
        SetSlotStatusThreadSafe(nSlot, 0);
        // The status line begins with the code %15.
        sprintf(szStatus, "%%15%s left the game.", g_aNetPeerSlots[nSlot].szName);
        SetGlobalStatusString(szStatus);
    }
}

// A notice of a player is formatted into locked global memory.
static char *FormatPlayerNotice(const char *pszFormat, const char *pszName) {
    char *pszNotice;

    if (pszName == nullptr) {
        pszName = "unknown";
    }
    pszNotice = static_cast<char *>(
        GlobalLock(GlobalAlloc(GHND, lstrlenA(pszFormat) + lstrlenA(pszName) + 1)));
    if (pszNotice != nullptr) {
        wsprintfA(pszNotice, pszFormat, pszName);
    }
    return pszNotice;
}

void HandleDirectPlaySystemMessage(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo) {
    char szMessage[0x104];
    char *pszNotice = nullptr;
    DPMSG_DESTROYPLAYERORGROUP *pDestroy;

    if (pInfo->bIsHost) {
        HandleDirectPlaySystemMessageHost(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
    } else {
        HandleDirectPlaySystemMessageClient(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
    }
    if (!g_dwNullStubEnable) {
        return;
    }
    switch (pMessage->dwType) {
    case DPSYS_CREATEPLAYERORGROUP:
        pszNotice = FormatPlayerNotice(
            "%s joined the game.\r\n",
            reinterpret_cast<DPMSG_CREATEPLAYERORGROUP *>(pMessage)->dpnName.lpszShortNameA);
        if (pszNotice == nullptr) {
            return;
        }
        break;
    case DPSYS_DESTROYPLAYERORGROUP:
        pDestroy = reinterpret_cast<DPMSG_DESTROYPLAYERORGROUP *>(pMessage);
        pszNotice = FormatPlayerNotice("%s left the game.\r\n", pDestroy->dpnName.lpszShortNameA);
        if (pszNotice == nullptr) {
            return;
        }
        for (SessionSlot &slot : g_aSessionSlots) {
            if (slot.dpidPlayer == pDestroy->dpId) {
                slot.dpidPlayer = 0xffffffff;
                slot.nFlag = 0;
                sprintf(szMessage,
                        "Destroy Player message received, ID: %d\n",
                        static_cast<int>(pDestroy->dpId));
                OutputDebugStringA(szMessage);
                break;
            }
        }
        break;
    case DPSYS_SETSESSIONDESC:
        LoadNetworkSettings();
        break;
    default:
        break;
    }
    if (pszNotice == nullptr) {
        return;
    }
    if (g_hMainWindow != nullptr) {
        PostMessageA(g_hMainWindow, WM_NETWORK_NOTICE, 0, reinterpret_cast<LPARAM>(pszNotice));
    } else {
        GlobalUnlock(GlobalHandle(pszNotice));
        GlobalFree(GlobalHandle(pszNotice));
    }
}

HRESULT ReceiveAndDispatchDirectPlayMessages(DirectPlayInfo *pInfo) {
    DPMSG_GENERIC *pMessage = nullptr;
    DWORD dwSize = 0;
    DPID dpidFrom;
    DPID dpidTo;
    HRESULT hResult;

    do {
        do {
            dpidFrom = 0;
            dpidTo = 0;
            hResult =
                pInfo->pDirectPlay->Receive(&dpidFrom, &dpidTo, DPRECEIVE_ALL, pMessage, &dwSize);
            if (hResult == DPERR_BUFFERTOOSMALL) {
                if (pMessage != nullptr) {
                    FreeGlobalBuffer(pMessage);
                }
                pMessage = static_cast<DPMSG_GENERIC *>(GlobalLock(GlobalAlloc(GHND, dwSize)));
                if (pMessage == nullptr) {
                    hResult = E_OUTOFMEMORY;
                }
            }
        } while (hResult == DPERR_BUFFERTOOSMALL);
        if (SUCCEEDED(hResult) && dwSize >= sizeof(DPMSG_GENERIC)) {
            if (dpidFrom == DPID_SYSMSG) {
                HandleDirectPlaySystemMessage(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
            } else {
                HandleMultiplayerSessionMessage(pInfo, pMessage, dwSize, dpidFrom, dpidTo);
            }
        }
    } while (SUCCEEDED(hResult));
    if (pMessage != nullptr) {
        FreeGlobalBuffer(pMessage);
    }
    return DP_OK;
}

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

// The cars g_dwCarMenuIndex steps through.
#define CAR_MENU_CHOICES 32

static inline void FreeSessionDesc(DPSESSIONDESC2 *pDesc) {
    GlobalUnlock(GlobalHandle(pDesc));
    GlobalFree(GlobalHandle(pDesc));
}

HRESULT JoinDirectPlaySession(IDirectPlay3A *pDirectPlay,
                              const GUID *instance,
                              NetworkJoinCredentials *credentials,
                              DirectPlayInfo *pInfo) {
    DPSESSIONDESC2 *description = nullptr;
    if (pDirectPlay == nullptr) {
        return DPERR_INVALIDOBJECT;
    }
    DPSESSIONDESC2 request = {};
    request.dwSize = sizeof(request);
    request.guidInstance = *instance;
    DPCREDENTIALS authentication = {};
    authentication.dwSize = sizeof(authentication);
    DPCREDENTIALS *optionalAuthentication = nullptr;
    if (lstrlenA(credentials->userName)) {
        authentication.lpszUsernameA = credentials->userName;
        optionalAuthentication = &authentication;
    }
    if (lstrlenA(credentials->password)) {
        authentication.lpszPasswordA = credentials->password;
        optionalAuthentication = &authentication;
    }
    if (lstrlenA(credentials->domain)) {
        authentication.lpszDomainA = credentials->domain;
        optionalAuthentication = &authentication;
    }
    HRESULT result = pDirectPlay->SecureOpen(
        &request, DPOPEN_JOIN | DPOPEN_RETURNSTATUS, nullptr, optionalAuthentication);
    if (SUCCEEDED(result)) {
        DPNAME name = {};
        name.dwSize = sizeof(name);
        name.lpszShortNameA = g_GlobalRallyConfig.szPlayerName;
        result = GetDirectPlaySessionDesc(pDirectPlay, &description);
        if (SUCCEEDED(result)) {
            if (description->dwCurrentPlayers >= 8) {
                result = DPERR_CANTADDPLAYER;
            } else {
                DWORD flags = g_bNetSpectator ? DPPLAYER_SPECTATOR : 0;
                IDirectPlay3A *previousInterface = pInfo->pDirectPlay;
                BOOL previousHost = pInfo->bIsHost;
                BOOL previousIgnoreMessages = pInfo->bIgnoreMessages;
                pInfo->pDirectPlay = pDirectPlay;
                pInfo->bIsHost = false;
                pInfo->bIgnoreMessages = (description->dwFlags & DPSESSION_SECURESERVER) != 0;
                DPID player;
                result = pDirectPlay->CreatePlayer(
                    &player, &name, pInfo->hPlayerEvent, nullptr, 0, flags);
                if (FAILED(result)) {
                    pInfo->pDirectPlay = previousInterface;
                    pInfo->bIsHost = previousHost;
                    pInfo->bIgnoreMessages = previousIgnoreMessages;
                } else {
                    pInfo->dpidPlayer = player;
                    g_dwTrackMenuIndex = description->dwUser1;
                    g_dwSelectedTrackId = description->dwUser1;
                    g_dwSelectedWeather = description->dwUser2;
                    g_dwChosenWeather = description->dwUser2;
                    g_dwSelectedCarClass = description->dwUser3;
                    g_dwTotalLapsSetting = description->dwUser4;
                    g_dwTotalLaps = description->dwUser4;
                    // The original copies all 80 bytes, including bytes after a terminator.
                    std::memcpy(g_szSessionName, description->lpszSessionNameA, 80);
                    FreeSessionDesc(description);
                    return DP_OK;
                }
            }
        }
    }
    if (description != nullptr) {
        FreeSessionDesc(description);
    }
    return result;
}

HRESULT CreateDirectPlaySession(IDirectPlay3A *pDirectPlay,
                                NetworkSessionSettings *pSettings,
                                DirectPlayInfo *pInfo) {
    if (g_dwLaunchedFromLobby) {
        return DP_OK;
    }
    if (!pDirectPlay) {
        return DPERR_INVALIDOBJECT;
    }
    DPSESSIONDESC2 description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DPSESSION_KEEPALIVE;
    if (pSettings->bSecureServer) {
        description.dwFlags |= DPSESSION_SECURESERVER;
    }
    description.guidApplication = g_BossRallyAppGuid;
    description.dwMaxPlayers = 8;
    description.lpszSessionNameA = pSettings->szSessionName;
    description.dwUser1 = g_dwSelectedTrackId;
    description.dwUser2 = g_dwChosenWeather;
    description.dwUser3 = g_dwSelectedCarClass;
    description.dwUser4 = g_dwTotalLapsSetting;
    HRESULT result = pDirectPlay->SecureOpen(
        &description, DPOPEN_CREATE | DPOPEN_RETURNSTATUS, nullptr, nullptr);
    if (FAILED(result)) {
        return result;
    }
    const DirectPlayInfo previous = *pInfo;
    pInfo->pDirectPlay = pDirectPlay;
    pInfo->bIsHost = true;
    pInfo->bIgnoreMessages = pSettings->bSecureServer;
    DPNAME name = {};
    name.dwSize = sizeof(name);
    name.lpszShortNameA = g_GlobalRallyConfig.szPlayerName;
    DPID player;
    result = pDirectPlay->CreatePlayer(
        &player, &name, pInfo->hPlayerEvent, nullptr, 0, DPPLAYER_SERVERPLAYER);
    if (FAILED(result)) {
        pInfo->pDirectPlay = previous.pDirectPlay;
        pInfo->bIsHost = previous.bIsHost;
        pInfo->bIgnoreMessages = previous.bIgnoreMessages;
        return result;
    }
    pInfo->dpidPlayer = player;
    return DP_OK;
}

static inline int CompleteNetworkJoinCredentials(NetworkJoinCredentials *) {
    // The credentials callback shares the controller initialiser's constant return.
    return 1;
}

int JoinNetworkSession(void) {
    if (g_DirectPlayInfo.pDirectPlay == nullptr) {
        return 0;
    }
    if (g_pNetworkSessionJoinMenu != nullptr &&
        g_pJoinCreateGameMenu->GetItemList().GetItemCount() > 0) {
        if (!g_dwLaunchedFromLobby) {
            GUID instance;
            HRESULT result = GetSelectedSessionGuid(&instance);
            if (SUCCEEDED(result)) {
                NetworkJoinCredentials credentials = {};
                DWORD userNameSize = sizeof(credentials.localUserName);
                GetUserNameA(credentials.localUserName, &userNameSize);
                result = JoinDirectPlaySession(
                    g_DirectPlayInfo.pDirectPlay, &instance, &credentials, g_pDirectPlayInfo);
                if (result == DPERR_LOGONDENIED) {
                    if (!CompleteNetworkJoinCredentials(&credentials)) {
                        return 0;
                    }
                    result = JoinDirectPlaySession(
                        g_DirectPlayInfo.pDirectPlay, &instance, &credentials, g_pDirectPlayInfo);
                }
            }
            if (FAILED(result)) {
                DisconnectNetworkSession();
                InitializeNetworkServiceProvider();
                char error[1024];
                // The original formats the error without displaying it.
                std::sprintf(error,
                             "Could not join session because of error 0x%08X",
                             static_cast<unsigned>(result));
                return 0;
            }
        }
        g_dwNetworkPlayMode = 1;
        InitializeNetworkMutexes(1);
        LoadNetworkSettings();
    }
    return 1;
}

void HostNetworkSession(void) {
    if (!g_DirectPlayInfo.pDirectPlay) {
        return;
    }
    NetworkSessionSettings settings = {};
    CopySessionNameToBuffer(&settings);
    HRESULT result =
        CreateDirectPlaySession(g_DirectPlayInfo.pDirectPlay, &settings, g_pDirectPlayInfo);
    if (FAILED(result)) {
        char szError[1024];
        /* The original formats the error without displaying it. */
        sprintf(szError,
                "Could not host session because of error 0x%08X",
                static_cast<unsigned>(result));
        return;
    }
    g_dwNetworkPlayMode = 2;
    InitializeNetworkThreading();
    InitializeNetworkMutexes(1);
}

HRESULT UpdateNetworkSessionSettings(void) {
    DPSESSIONDESC2 *description = nullptr;
    if (!g_DirectPlayInfo.pDirectPlay) {
        return DPERR_INVALIDOBJECT;
    }
    HRESULT result = GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &description);
    if (FAILED(result)) {
        if (description) {
            FreeSessionDesc(description);
        }
        return result;
    }
    description->dwUser1 = g_dwSelectedTrackId;
    description->dwUser2 = g_dwChosenWeather;
    description->dwUser3 = g_dwSelectedCarClass;
    description->dwUser4 = g_dwTotalLapsSetting;
    ApplyCarGroupPreset();
    result = g_DirectPlayInfo.pDirectPlay->SetSessionDesc(description, 0);
    FreeSessionDesc(description);
    return FAILED(result) ? result : DP_OK;
}

HRESULT LoadNetworkSettings(void) {
    DPSESSIONDESC2 *pDesc = nullptr;
    DPSESSIONDESC2 descUnused;

    if (g_DirectPlayInfo.pDirectPlay == nullptr) {
        return DPERR_INVALIDOBJECT;
    }
    // The description is cleared and never read.
    memset(&descUnused, 0, sizeof(descUnused));
    HRESULT hResult = GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &pDesc);
    if (FAILED(hResult)) {
        if (pDesc != nullptr) {
            FreeSessionDesc(pDesc);
        }
        return hResult;
    }
    g_dwTrackMenuIndex = g_dwSelectedTrackId = pDesc->dwUser1;
    g_dwSelectedWeather = g_dwChosenWeather = pDesc->dwUser2;
    g_dwSelectedCarClass = pDesc->dwUser3;
    g_dwTotalLaps = g_dwTotalLapsSetting = pDesc->dwUser4;
    ApplyCarGroupPreset();
    int nFirstCar = g_dwCarMenuIndex;
    if (!IsCarSelectable(nFirstCar)) {
        do {
            if (++g_dwCarMenuIndex >= CAR_MENU_CHOICES) {
                g_dwCarMenuIndex = 0;
            }
        } while (g_dwCarMenuIndex != nFirstCar && !IsCarSelectable(g_dwCarMenuIndex));
    }
    if (pDesc->lpszSessionNameA != nullptr) {
        strcpy(g_szSessionName, pDesc->lpszSessionNameA);
    }
    FreeSessionDesc(pDesc);
    return DP_OK;
}

HRESULT GetDirectPlaySessionDesc(IDirectPlay3A *pDirectPlay, DPSESSIONDESC2 **ppDesc) {
    DWORD dwSize;
    DPSESSIONDESC2 *pDesc = nullptr;

    HRESULT hResult = pDirectPlay->GetSessionDesc(nullptr, &dwSize);
    if (hResult == DPERR_BUFFERTOOSMALL) {
        pDesc = static_cast<DPSESSIONDESC2 *>(GlobalLock(GlobalAlloc(GHND, dwSize)));
        if (pDesc == nullptr) {
            hResult = E_OUTOFMEMORY;
        } else {
            hResult = pDirectPlay->GetSessionDesc(pDesc, &dwSize);
            if (SUCCEEDED(hResult)) {
                *ppDesc = pDesc;
                pDesc = nullptr;
            }
        }
    }
    if (pDesc != nullptr) {
        FreeSessionDesc(pDesc);
    }
    return hResult;
}

void CopySessionNameToBuffer(NetworkSessionSettings *pSettings) {
    // A name of one character is not copied.
    if (strlen(g_szSessionName) > 1) {
        strcpy(pSettings->szSessionName, g_szSessionName);
    }
    pSettings->bSecureServer = FALSE;
}

// The player count of GetDirectPlaySessionPlayerCount() without a description.
#define SESSION_PLAYER_COUNT_UNKNOWN 0xffff

DWORD GetDirectPlaySessionPlayerCount(void) {
    DPSESSIONDESC2 *pDesc = nullptr;

    if (FAILED(GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &pDesc))) {
        return SESSION_PLAYER_COUNT_UNKNOWN;
    }
    DWORD dwCount = pDesc->dwCurrentPlayers;
    FreeSessionDesc(pDesc);
    return dwCount;
}
