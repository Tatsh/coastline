#pragma once

#include <windows.h>
// The headers below need the types of windows.h.
#include <dplay.h>
#include <dplobby.h>

/** The DirectPlay object of the game and its local player. */
struct DirectPlayInfo {
    IDirectPlay3A *pDirectPlay; /*!< The DirectPlay object. */
    HANDLE hPlayerEvent;        /*!< Signalled when a message for the local player arrives. */
    DPID dpidPlayer;            /*!< The local player. */
    BOOL bIsHost;               /*!< Whether the game hosts the session. */
    char abUnknown10[4];        /* +0x10 */
};

/** A service provider of DirectPlay found by EnumConnections. */
struct ServiceProvider {
    char szName[200];       /*!< The short name of the connection. */
    GUID guid;              /*!< The GUID of the service provider. */
    DWORD dwConnectionSize; /*!< The size of the connection data. */
    void *pConnectionData;  /*!< A copy of the connection data, in locked global memory. */
};

/** The number of entries of g_aServiceProviders. */
#define SERVICE_PROVIDER_COUNT 16

/**
 * The DirectPlay object of the game and its local player.
 *
 * @ghidraAddress BRD3D: 0x10277b40
 * @ghidraAddress BRGlide: 0x10273328
 */
extern DirectPlayInfo g_DirectPlayInfo;

/**
 * The DirectPlay object other routines read, pointing at the first member of g_DirectPlayInfo.
 *
 * @ghidraAddress BRD3D: 0x10a9d008
 * @ghidraAddress BRGlide: 0x10ac4098
 */
extern IDirectPlay3A **g_ppDirectPlay;

/**
 * The service providers, indexed by IPX, TCP/IP, modem, and serial in that order.
 *
 * @ghidraAddress BRD3D: 0x10a9bff0
 * @ghidraAddress BRGlide: 0x10ac3080
 */
extern ServiceProvider g_aServiceProviders[SERVICE_PROVIDER_COUNT];

/**
 * The telephone number of a modem connection.
 *
 * @ghidraAddress BRD3D: 0x10a9cdf0
 * @ghidraAddress BRGlide: 0x10ac3e80
 */
extern char g_szModemPhoneNumber[];

/**
 * The device of a modem connection.
 *
 * @ghidraAddress BRD3D: 0x10a9cef8
 * @ghidraAddress BRGlide: 0x10ac3f88
 */
extern char g_szModemDeviceName[];

/**
 * The empty string of the user interface.
 *
 * @ghidraAddress BRD3D: 0x1039b720
 * @ghidraAddress BRGlide: 0x10396f08
 */
extern const char g_szEmptyUiString[];

/**
 * Whether a lobby launched the game.
 *
 * @ghidraAddress BRD3D: 0x10a9d000
 * @ghidraAddress BRGlide: 0x10ac4090
 */
extern bool g_dwLaunchedFromLobby;

/**
 * Whether the lobby made the game the host.
 *
 * @ghidraAddress BRD3D: 0x10aa2884
 * @ghidraAddress BRGlide: 0x10ac5bdc
 */
extern bool g_dwLobbyIsHost;

/**
 * Whether an enumeration of sessions is waiting for a reply.
 *
 * @ghidraAddress BRD3D: 0x10aa2898
 * @ghidraAddress BRGlide: 0x10ac5bf0
 */
extern bool g_dwSessionEnumPending;

/**
 * The timer of the main window that drives network play.
 *
 * @ghidraAddress BRD3D: 0x10a9bfdc
 * @ghidraAddress BRGlide: 0x10ac306c
 */
extern UINT_PTR g_dwNetworkTimerId;

/**
 * The DirectPlay lobby.
 *
 * @ghidraAddress BRD3D: 0x10a9bfd8
 * @ghidraAddress BRGlide: 0x10ac3068
 */
extern IDirectPlayLobby2A *g_pDirectPlayLobby;

/**
 * The GUID of the game for DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1008f538
 * @ghidraAddress BRGlide: 0x10077500
 */
extern const GUID g_BossRallyAppGuid;

/**
 * Whether the critical section of DirectPlay sends exists.
 *
 * @ghidraAddress BRD3D: 0x10277b60
 * @ghidraAddress BRGlide: 0x10273348
 */
extern bool g_bDirectPlayReceiveThreadStarted;

/**
 * The critical section of DirectPlay sends.
 *
 * @ghidraAddress BRD3D: 0x10277b28
 * @ghidraAddress BRGlide: 0x10273310
 */
extern CRITICAL_SECTION g_DirectPlaySendCritSec;

/**
 * The event that stops the receive thread.
 *
 * @ghidraAddress BRD3D: 0x10277b5c
 * @ghidraAddress BRGlide: 0x10273344
 */
extern HANDLE g_hDirectPlayReceiveStopEvent;

/**
 * The identifier of the receive thread.
 *
 * @ghidraAddress BRD3D: 0x10277b58
 * @ghidraAddress BRGlide: 0x10273340
 */
extern DWORD g_dwDirectPlayReceiveThreadId;

/**
 * The thread that receives the messages of the local player.
 *
 * @ghidraAddress BRD3D: 0x10277b54
 * @ghidraAddress BRGlide: 0x1027333c
 */
extern HANDLE g_hDirectPlayReceiveThread;

/**
 * Create DirectPlay, connecting through a lobby when one launched the game, and start the receive
 * thread and the network timer.
 *
 * @return Whether DirectPlay started.
 * @ghidraAddress BRD3D: 0x1003bd80
 * @ghidraAddress BRGlide: 0x10035400
 */
bool InitializeDirectPlay(void);

/**
 * Clear a DirectPlay info, create its player event, and start the thread that receives the
 * messages of its player.
 *
 * @param hInstance The module of the game.
 * @param pInfo The info to set up.
 * @return DP_OK, or E_OUTOFMEMORY when an event or the thread cannot be created.
 * @ghidraAddress BRD3D: 0x1000c5d0
 * @ghidraAddress BRGlide: 0x10009b00
 */
HRESULT CreateDirectPlayReceiveThread(HINSTANCE hInstance, DirectPlayInfo *pInfo);

/**
 * Stop the receive thread, destroy the local player, and close and release DirectPlay.
 *
 * @param pInfo The info to shut down, or null to stop the thread alone.
 * @return Always DP_OK.
 * @ghidraAddress BRD3D: 0x1000c510
 * @ghidraAddress BRGlide: 0x10009a40
 */
HRESULT ShutdownDirectPlaySession(DirectPlayInfo *pInfo);

/**
 * Receive the messages of the local player until the stop event is signalled.
 *
 * @param pParameter The DirectPlayInfo of the player.
 * @return Never returns. The thread exits with code 0.
 * @ghidraAddress BRD3D: 0x1000c440
 * @ghidraAddress BRGlide: 0x10009970
 */
DWORD WINAPI DirectPlayReceiveThreadProc(void *pParameter);

/**
 * Receive and handle every waiting message of the local player.
 *
 * @param pInfo The info of the player.
 * @return Always DP_OK.
 * @ghidraAddress BRD3D: 0x1000c350
 */
HRESULT ReceiveAndDispatchDirectPlayMessages(DirectPlayInfo *pInfo);

/**
 * Connect to the session of the lobby that launched the game, and create the local player in it.
 * The name of the player and of the session are copied from the lobby.
 *
 * @param pInfo The info to fill, whose player event is already created.
 * @return DP_OK, DPERR_NOTLOBBIED when no lobby launched the game, or another error.
 * @ghidraAddress BRD3D: 0x10038cb0
 * @ghidraAddress BRGlide: 0x10032320
 */
HRESULT ConnectFromDirectPlayLobby(DirectPlayInfo *pInfo);

/**
 * The name of the network session.
 *
 * @ghidraAddress BRD3D: 0x10a9d018
 * @ghidraAddress BRGlide: 0x10ac40a8
 */
extern char g_szSessionName[];

/**
 * Enumerate the parts of the address of the local player for the lobby.
 *
 * @param hWnd The main window.
 * @param pLobby The lobby.
 * @return The result of EnumAddress.
 * @ghidraAddress BRD3D: 0x1003d8a0
 * @ghidraAddress BRGlide: 0x10036f40
 */
HRESULT EnumeratePlayerAddress(HWND hWnd, IDirectPlayLobby2A *pLobby);

/**
 * Create the DirectPlay object.
 *
 * @param ppDirectPlay Receives the object, or null.
 * @return The result of CoCreateInstance.
 * @ghidraAddress BRD3D: 0x1003c520
 * @ghidraAddress BRGlide: 0x10035bb0
 */
HRESULT CreateDirectPlayObject(IDirectPlay3A **ppDirectPlay);

/**
 * Create the DirectPlay lobby and enumerate the address of the local player.
 *
 * @param ppLobby Receives the lobby on success.
 * @return DP_OK, or the error of the creation.
 * @ghidraAddress BRD3D: 0x1003d7b0
 * @ghidraAddress BRGlide: 0x10036e50
 */
HRESULT CreateDirectPlayLobby(IDirectPlayLobby2A **ppLobby);

/**
 * Enumerate the sessions of the selected service provider again.
 *
 * @param pDirectPlay The DirectPlay object.
 * @return The result of the enumeration.
 * @ghidraAddress BRD3D: 0x1003cc70
 */
HRESULT RefreshNetworkSessionList(IDirectPlay3A *pDirectPlay);

/**
 * Store a service provider of IPX, TCP/IP, modem, or serial in g_aServiceProviders, and skip any
 * other.
 *
 * @param pGuidServiceProvider The GUID of the service provider.
 * @param pConnection The connection data.
 * @param dwConnectionSize The size of the connection data.
 * @param pName The name of the connection.
 * @param dwFlags Not read.
 * @param pContext Not read.
 * @return Always TRUE, to continue the enumeration.
 * @ghidraAddress BRD3D: 0x1003c430
 * @ghidraAddress BRGlide: 0x10035ac0
 */
BOOL WINAPI EnumConnectionsCallback_StoreServiceProvider(const GUID *pGuidServiceProvider,
                                                         void *pConnection,
                                                         DWORD dwConnectionSize,
                                                         const DPNAME *pName,
                                                         DWORD dwFlags,
                                                         void *pContext);

/**
 * Show an error of network play with its DirectPlay error name.
 *
 * @param pszFormat A wsprintfA format of the message, with one %s for the error name.
 * @param hResult The error.
 * @ghidraAddress BRD3D: 0x1003dbc0
 * @ghidraAddress BRGlide: 0x10037260
 */
void ShowNetworkErrorMessage(const char *pszFormat, HRESULT hResult);

/**
 * Give the name of a DirectPlay error.
 *
 * @param hResult The error.
 * @return The name of the error, such as "DPERR_PENDING".
 * @ghidraAddress BRD3D: 0x1003dc10
 * @ghidraAddress BRGlide: 0x100372b0
 */
const char *GetDirectPlayErrorString(HRESULT hResult);

/**
 * Initialise DirectPlay for the main window.
 *
 * @ghidraAddress BRD3D: 0x1000c6d0
 * @ghidraAddress BRGlide: 0x10009c00
 */
void InitializeDirectPlayForDesktopWindow(void);

/**
 * Whether a DirectPlay session is open.
 *
 * @ghidraAddress BRD3D: 0x10a9cffc
 */
extern bool g_dwDirectPlaySessionActive;

/**
 * Whether the race of a network game has started.
 *
 * @ghidraAddress BRD3D: 0x1022af14
 */
extern bool g_bNetRaceStartReached;

/**
 * Ask the other players of a network game to pause.
 *
 * @ghidraAddress BRD3D: 0x10004be0
 */
void RequestNetworkPause(void);

/**
 * Send the request to pause of a network game.
 *
 * @ghidraAddress BRD3D: 0x10004fc0
 */
void TickNetworkPauseRequest(void);

/**
 * Give a message of the main window to the network play.
 *
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @ghidraAddress BRD3D: 0x1003c3a0
 */
void WINAPI HandleNetworkWindowMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

/**
 * End the network play of a game.
 *
 * @param nReason 1 as the game ends. Its meaning is not recovered yet.
 * @ghidraAddress BRD3D: 0x10005be0
 */
void ShutdownNetworkSubsystem(int nReason);

/**
 * Release DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1003bfd0
 */
void ShutdownDirectPlay(void);

/**
 * Leave the session of DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1003bf60
 */
void DisconnectNetworkSession(void);
