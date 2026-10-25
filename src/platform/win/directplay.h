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
 * The DirectPlay info other routines read, g_DirectPlayInfo once DirectPlay starts.
 *
 * @ghidraAddress BRD3D: 0x10a9d008
 * @ghidraAddress BRGlide: 0x10ac4098
 */
extern DirectPlayInfo *g_pDirectPlayInfo;

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
 * Receive and handle every waiting message of the local player, growing the buffer of the
 * messages in global memory as a message needs, until the receive fails.
 *
 * @param pInfo The info of the player.
 * @return Always DP_OK.
 * @ghidraAddress BRD3D: 0x1000c350
 * @ghidraAddress BRGlide: 0x10009880
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
 * Walk the strings of the COM port part of an address, and skip any other part.
 *
 * @param guidDataType The type of the part.
 * @param dwDataSize Not read.
 * @param pData The data of the part.
 * @param pContext Not read.
 * @return Always TRUE, to continue the enumeration.
 * @ghidraAddress BRD3D: 0x1003d850
 * @ghidraAddress BRGlide: 0x10036ef0
 */
BOOL WINAPI EnumAddressCallback(REFGUID guidDataType,
                                DWORD dwDataSize,
                                const void *pData,
                                void *pContext);

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
 * The name of an error GetDirectPlayErrorString() does not know, written in hexadecimal.
 *
 * @ghidraAddress BRD3D: 0x10a9bfe0
 * @ghidraAddress BRGlide: 0x10ac3070
 */
extern char g_szDirectPlayErrorBuffer[16];

/**
 * Give the name of a DirectPlay error.
 *
 * @param hResult The error.
 * @return The name of the error, such as "DPERR_PENDING", or the error in hexadecimal in
 * g_szDirectPlayErrorBuffer when the error is not known.
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
 * The ticks a request of a pause of a network game has waited, 0 for no request.
 *
 * @ghidraAddress BRD3D: 0x1022aaf4
 * @ghidraAddress BRGlide: 0x10226624
 */
extern int g_nNetPauseRequestTicks;

/**
 * Request a pause from the other players of a network game, when no request waits.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10004be0
 * @ghidraAddress BRGlide: 0x10004f50
 */
bool RequestNetworkPause(void);

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

/** A peer of a network game. The layout past the mutex is not recovered yet. */
struct NetPeerSlot {
    HANDLE hMutex;          /*!< The mutex of the peer. */
    char abUnknown4[0x974]; /* +0x04 */
};

/** The number of entries of g_aNetPeerSlots. */
#define NET_PEER_SLOT_COUNT 16

/**
 * The peers of a network game.
 *
 * @ghidraAddress BRD3D: 0x10221328
 * @ghidraAddress BRGlide: 0x1021ce58
 */
extern NetPeerSlot g_aNetPeerSlots[NET_PEER_SLOT_COUNT];

/**
 * The mutex of the start of a network race.
 *
 * @ghidraAddress BRD3D: 0x10220cec
 * @ghidraAddress BRGlide: 0x1021c81c
 */
extern HANDLE g_hNetRaceStartMutex;

/**
 * The mutex of the messages received.
 *
 * @ghidraAddress BRD3D: 0x1022131c
 * @ghidraAddress BRGlide: 0x1021ce4c
 */
extern HANDLE g_hNetReceiveMutex;

/**
 * The mutex of a pause of a network game.
 *
 * @ghidraAddress BRD3D: 0x1022af04
 * @ghidraAddress BRGlide: 0x10226a34
 */
extern HANDLE g_hNetPauseMutex;

/**
 * The mutex of the end of a pause of a network game.
 *
 * @ghidraAddress BRD3D: 0x10220ddc
 * @ghidraAddress BRGlide: 0x1021c90c
 */
extern HANDLE g_hNetUnpauseMutex;

/**
 * The mutex of the players that are ready.
 *
 * @ghidraAddress BRD3D: 0x10221324
 * @ghidraAddress BRGlide: 0x1021ce54
 */
extern HANDLE g_hNetReadyMutex;

/**
 * The mutex of the key frames of a network race.
 *
 * @ghidraAddress BRD3D: 0x1022af34
 * @ghidraAddress BRGlide: 0x10226a64
 */
extern HANDLE g_hNetKeyframeMutex;

/**
 * The mutex of the stack of players that joined.
 *
 * @ghidraAddress BRD3D: 0x1022af2c
 * @ghidraAddress BRGlide: 0x10226a5c
 */
extern HANDLE g_hJoinedPlayerStackMutex;

/**
 * The mutex of the stack of players that departed.
 *
 * @ghidraAddress BRD3D: 0x1022af30
 * @ghidraAddress BRGlide: 0x10226a60
 */
extern HANDLE g_hDepartedPlayerStackMutex;

/**
 * The mutex of the order of a network race.
 *
 * @ghidraAddress BRD3D: 0x1022af28
 * @ghidraAddress BRGlide: 0x10226a58
 */
extern HANDLE g_hNetRaceOrderMutex;

/**
 * The mutex of the notices of network play.
 *
 * @ghidraAddress BRD3D: 0x1022af24
 * @ghidraAddress BRGlide: 0x10226a54
 */
extern HANDLE g_hNetNotifyMutex;

/**
 * Whether the worker thread of the host runs.
 *
 * @ghidraAddress BRD3D: 0x11828d18
 * @ghidraAddress BRGlide: 0x1184c078
 */
extern bool g_bNetWorkerThreadRunning;

/**
 * The event that stops the worker thread of the host.
 *
 * @ghidraAddress BRD3D: 0x11826b00
 * @ghidraAddress BRGlide: 0x11849e60
 */
extern HANDLE g_hNetWorkerStopEvent;

/**
 * The worker thread of the host.
 *
 * @ghidraAddress BRD3D: 0x11828d1c
 * @ghidraAddress BRGlide: 0x1184c07c
 */
extern HANDLE g_hNetWorkerThread;

/**
 * Stop the thread that works for the host of a network game, wait for it, and close it and its
 * stop event.
 *
 * @ghidraAddress BRD3D: 0x10072270
 * @ghidraAddress BRGlide: 0x1006b1e0
 */
void StopNetworkWorkerThread(void);

/**
 * Handle a message of the system of DirectPlay as the host or a client. While the front end is
 * up, a player that joins or departs is posted to the main window as a notice, a player that
 * departs frees its slot of the session, and a new description of the session reloads the
 * settings of the network.
 *
 * @param pInfo The info of the local player.
 * @param pMessage The message.
 * @param dwSize The size of the message.
 * @param dpidFrom The sender, DPID_SYSMSG.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x1000c170
 * @ghidraAddress BRGlide: 0x100096a0
 */
void HandleDirectPlaySystemMessage(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * Free the slots of the network players a player of the session controls.
 *
 * @param dpidOwner The player.
 * @ghidraAddress BRD3D: 0x10071480
 * @ghidraAddress BRGlide: 0x1006a3f0
 */
void ClearNetworkPlayerSlotsByOwner(DPID dpidOwner);

/**
 * Remove a player that departed from a race of the network.
 *
 * @param dpidPlayer The player.
 * @ghidraAddress BRD3D: 0x10005fe0
 * @ghidraAddress BRGlide: 0x10006350
 */
void HandlePlayerDisconnect(DPID dpidPlayer);

/**
 * Handle the packets of a secure message as the host.
 *
 * @param pInfo The info of the local player.
 * @param pData The data of the message.
 * @param dwSize The size of the data.
 * @param dpidFrom The sender.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x100360f0
 * @ghidraAddress BRGlide: 0x1002f790
 */
void ProcessMultiplayerPackets(
    DirectPlayInfo *pInfo, void *pData, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * Handle the packets of a secure message as a client.
 *
 * @param pInfo The info of the local player.
 * @param pData The data of the message.
 * @param dwSize The size of the data.
 * @param dpidFrom The sender.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x10003580
 * @ghidraAddress BRGlide: 0x100038f0
 */
void ProcessNetworkDataStream(
    DirectPlayInfo *pInfo, void *pData, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * Handle a message of the system of DirectPlay for the host of the session. A player that departs
 * while the front end is down frees its slots and exits the race, and the data of a secure
 * message goes to ProcessMultiplayerPackets().
 *
 * @param pInfo The info of the local player.
 * @param pMessage The message.
 * @param dwSize The size of the message.
 * @param dpidFrom The sender, DPID_SYSMSG.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x1000c000
 * @ghidraAddress BRGlide: 0x10009530
 */
void HandleDirectPlaySystemMessageHost(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * Handle a message of the system of DirectPlay for a client of the session. A player that departs
 * while the front end is down exits the race, and the data of a secure message goes to
 * ProcessNetworkDataStream().
 *
 * @param pInfo The info of the local player.
 * @param pMessage The message.
 * @param dwSize The size of the message.
 * @param dpidFrom The sender, DPID_SYSMSG.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x1000bea0
 * @ghidraAddress BRGlide: 0x100093d0
 */
void HandleDirectPlaySystemMessageClient(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * Read the settings of the network session again after its description changed.
 *
 * @ghidraAddress BRD3D: 0x1003ce80
 * @ghidraAddress BRGlide: 0x10036510
 */
void LoadNetworkSettings(void);

/** A player in a slot of the session. */
struct SessionSlot {
    DPID dpidPlayer; /*!< The player, or 0xffffffff for a free slot. */
    int nFlag;       /*!< Cleared when the player departs. Its meaning is not recovered yet. */
    int nSeen;       /*!< Cleared when the table is reset. Its meaning is not recovered yet. */
};

/**
 * Whether the sends of network play are blocked.
 *
 * @ghidraAddress BRD3D: 0x10aa288c
 * @ghidraAddress BRGlide: 0x10ac5be4
 */
extern bool g_dwNetworkSendBlocked;

/**
 * Free every slot of the session and unblock the sends.
 *
 * @ghidraAddress BRD3D: 0x100586a0
 * @ghidraAddress BRGlide: 0x10051550
 */
void ResetSessionSlotTable(void);

/** A network player of a race. The layout is not recovered past the members below. */
struct NetPlayerSlot {
    HANDLE hMutex;           /*!< The mutex of the slot. */
    DPID dpidOwner;          /*!< The player of the session that controls the slot. */
    char abUnknown8[0x24];   /* +0x08 */
    int dwState;             /*!< The state of the slot in its low 6 bits, 0 for free. */
    char abUnknown30[0x93c]; /* +0x30 */
};

/** The number of entries of g_aNetPlayerSlots. */
#define NET_PLAYER_SLOT_COUNT 16

/**
 * The network players of a race.
 *
 * @ghidraAddress BRD3D: 0x11786828
 * @ghidraAddress BRGlide: 0x117a9b88
 */
extern NetPlayerSlot g_aNetPlayerSlots[NET_PLAYER_SLOT_COUNT];

/**
 * Destroy the groups of players of the session.
 *
 * @ghidraAddress BRD3D: 0x1003d070
 * @ghidraAddress BRGlide: 0x10036700
 */
void DestroyNetworkGroups(void);

/**
 * Destroy the groups and the local player, close and release DirectPlay, and count one DirectPlay
 * object less.
 *
 * @return Always DP_OK.
 * @ghidraAddress BRD3D: 0x1003c550
 * @ghidraAddress BRGlide: 0x10035be0
 */
HRESULT CloseAndReleaseDirectPlay(void);

/** The number of entries of g_aSessionSlots. */
#define SESSION_SLOT_COUNT 8

/**
 * The players in the slots of the session.
 *
 * @ghidraAddress BRD3D: 0x10aa2538
 * @ghidraAddress BRGlide: 0x10ac5890
 */
extern SessionSlot g_aSessionSlots[SESSION_SLOT_COUNT];

/**
 * The message the main window receives with a notice of network play in locked global memory as
 * the second parameter. The window frees the notice.
 */
#define WM_NETWORK_NOTICE 0x501

/**
 * Handle a message another player of the session sent. The return, if any, is not recovered yet.
 *
 * @param pInfo The info of the local player.
 * @param pMessage The message.
 * @param dwSize The size of the message.
 * @param dpidFrom The sender.
 * @param dpidTo The receiver.
 * @ghidraAddress BRD3D: 0x1000baf0
 * @ghidraAddress BRGlide: 0x10009010
 */
void HandleMultiplayerSessionMessage(
    DirectPlayInfo *pInfo, DPMSG_GENERIC *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * End the network play of a game. The host stops its worker thread, the session of DirectPlay
 * shuts down, and every mutex of network play is closed.
 *
 * @param nReason Not read. The callers pass 1.
 * @return Whether the session of DirectPlay shut down.
 * @ghidraAddress BRD3D: 0x10005be0
 * @ghidraAddress BRGlide: 0x10005f50
 */
bool ShutdownNetworkSubsystem(int nReason);

/**
 * End network play, free the connection data of the service providers, write the count of
 * DirectPlay objects left to the debugger, and release the lobby.
 *
 * @ghidraAddress BRD3D: 0x1003bfd0
 * @ghidraAddress BRGlide: 0x10035660
 */
void ShutdownDirectPlay(void);

/**
 * Free the connection data of every service provider.
 *
 * @ghidraAddress BRD3D: 0x1003cfe0
 * @ghidraAddress BRGlide: 0x10036670
 */
void FreeServiceProviderConnectionData(void);

/**
 * The number of DirectPlay objects that exist.
 *
 * @ghidraAddress BRD3D: 0x10a9d004
 * @ghidraAddress BRGlide: 0x10ac4094
 */
extern int g_dwDirectPlayInstanceCount;

/**
 * Exit the session of DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1003bf60
 */
void DisconnectNetworkSession(void);
