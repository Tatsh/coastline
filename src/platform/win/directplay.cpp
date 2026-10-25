#include "platform/win/directplay.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <objbase.h>

#include "laps.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/frametimer.h"
#include "platform/win/frontend.h"
#include "platform/win/netstream.h"
#include "platform/win/settings.h"
#include "platform/win/timer.h"
#include "trackselect.h"

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

int g_nNetUnpauseRequestTicks;

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

int GetLocalPlayerIndex(void) {
    return g_dwLocalNetPlayerId;
}

bool InitServiceProviderAndStartTimer(void) {
    InitializeNetworkServiceProvider();
    g_dwNetworkTimerId = SetTimer(g_hMainWindow, NETWORK_TIMER_ID, NETWORK_TIMER_PERIOD, nullptr);
    g_dwDirectPlaySessionActive = true;
    return true;
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
