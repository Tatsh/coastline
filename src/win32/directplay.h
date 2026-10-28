#pragma once

/**
 * @file
 * Network play over DirectPlay: the session and its players, the messages between them, and the
 * threads that receive the messages and exchange the streams of a race.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32types.h"
#else
#include <windows.h>
// The headers below need the types of windows.h.
#include <dplay.h>
#include <dplobby.h>
#endif

#include "win32/assetnames.h"
#include "win32/ghost.h"

/** The DirectPlay object of the game and its local player. */
struct DirectPlayInfo {
    IDirectPlay3A *pDirectPlay; /*!< The DirectPlay object. */
    HANDLE hPlayerEvent;        /*!< Signalled when a message for the local player arrives. */
    DPID dpidPlayer;            /*!< The local player. */
    BOOL bIsHost;               /*!< Whether the game hosts the session. */
    BOOL bIgnoreMessages;
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
 * @ghidraAddress BRD3D: 0x100ab3e0
 * @ghidraAddress BRGlide: 0x100aab80
 */
extern DPID g_dwNetCarSetupSelectedPlayerId;

/**
 * The service providers, indexed by IPX, TCP/IP, modem, and serial in that order.
 *
 * @ghidraAddress BRD3D: 0x10a9bff0
 * @ghidraAddress BRGlide: 0x10ac3080
 */
extern ServiceProvider g_aServiceProviders[SERVICE_PROVIDER_COUNT];

/**
 * The index in g_aServiceProviders of the service provider chosen.
 *
 * @ghidraAddress BRD3D: 0x10aa287c
 * @ghidraAddress BRGlide: 0x10ac5bd4
 */
extern int g_dwServiceProviderIndex;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Report the GUID of the service provider chosen.
 *
 * @param ppGuid Receives the GUID.
 * @return Always 0.
 * @ghidraAddress BRD3D: 0x1003cfc0
 * @ghidraAddress BRGlide: 0x10036650
 */
int GetSelectedServiceProvider(GUID **ppGuid);

/**
 * The time of the worker thread of network play, written at the start of a packet.
 *
 * @ghidraAddress BRD3D: 0x11828d10
 * @ghidraAddress BRGlide: 0x1184c070
 */
extern unsigned g_dwNetWorkerCurrentTime;

/**
 * Send a message of DirectPlay under g_DirectPlaySendCritSec.
 *
 * @param pDirectPlay The DirectPlay object.
 * @param dpidFrom The player that sends.
 * @param dpidTo The player that receives, or `DPID_ALLPLAYERS`.
 * @param dwFlags The flags of the send.
 * @param pData The message.
 * @param dwSize The bytes of the message.
 * @return The result of Send.
 * @ghidraAddress BRD3D: 0x1000c4d0
 * @ghidraAddress BRGlide: 0x10009a00
 */
HRESULT DirectPlaySendLocked(IDirectPlay3A *pDirectPlay,
                             DPID dpidFrom,
                             DPID dpidTo,
                             DWORD dwFlags,
                             void *pData,
                             DWORD dwSize);

/**
 * Send message 6 of the game, which names a player, from the local player to every player.
 *
 * @param pInfo The session.
 * @param dpidPlayer The player the message names.
 * @return The result of the send, or 0 without DirectPlay.
 * @ghidraAddress BRD3D: 0x1003da90
 * @ghidraAddress BRGlide: 0x10037130
 */
HRESULT SendNetworkMessage6(DirectPlayInfo *pInfo, DPID dpidPlayer);

/**
 * Send message 3 of the game from the local player to every player, unless the sends are blocked.
 * The second word of the message is never written.
 *
 * @param pInfo The session.
 * @return The result of the send, or 0 without DirectPlay or with the sends blocked.
 * @ghidraAddress BRD3D: 0x1003d9f0
 * @ghidraAddress BRGlide: 0x10037090
 */
HRESULT SendNetworkMessage3(DirectPlayInfo *pInfo);

/**
 * Send whether the slot of a player is ready from the local player to every player, unless sends
 * are blocked.
 *
 * @param pInfo The session.
 * @param dwReady The flag of the slot of the local player, from GetSessionSlotFlag().
 * @return The result of the send, or 0 without DirectPlay or while sends are blocked.
 * @ghidraAddress BRD3D: 0x1003d950
 * @ghidraAddress BRGlide: 0x10036ff0
 */
HRESULT SendNetworkReadyMessage(DirectPlayInfo *pInfo, DWORD dwReady);

/**
 * Send message 4 of the game, which identifies a player to boot, from the local player to every
 * player, unless sends are blocked.
 *
 * @param pInfo The session.
 * @param dpidPlayer The player.
 * @return The result of the send, or 0 without DirectPlay or while sends are blocked.
 * @ghidraAddress BRD3D: 0x1003da40
 * @ghidraAddress BRGlide: 0x100370e0
 */
HRESULT SendNetworkMessage4(DirectPlayInfo *pInfo, DPID dpidPlayer);

/**
 * Send a state of the race from the local player to every player, unless sends are blocked.
 *
 * @param pInfo The session.
 * @param dwState The state.
 * @return The result of the send, or 0 without DirectPlay or while sends are blocked.
 * @ghidraAddress BRD3D: 0x1003d9a0
 * @ghidraAddress BRGlide: 0x10037040
 */
HRESULT SendNetworkRaceStateMessage(DirectPlayInfo *pInfo, DWORD dwState);

/**
 * Send message 7 of the game, which names a player, from the local player to every player.
 *
 * @param pInfo The session.
 * @param dpidPlayer The player the message names.
 * @return The result of the send, or 0 without DirectPlay.
 * @ghidraAddress BRD3D: 0x1003db00
 * @ghidraAddress BRGlide: 0x100371a0
 */
HRESULT SendNetworkMessage7(DirectPlayInfo *pInfo, DPID dpidPlayer);

/**
 * Send message 6 of the game naming the local player, when there is one.
 *
 * @ghidraAddress BRD3D: 0x1003dae0
 * @ghidraAddress BRGlide: 0x10037180
 */
void SendNetworkMessage6ToSession(void);
#ifdef __cplusplus
}
#endif

class NetStream;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Empty a stream and write the time of the worker thread of network play at its start.
 *
 * @param pStream The stream.
 * @ghidraAddress BRD3D: 0x10071bf0
 * @ghidraAddress BRGlide: 0x1006ab60
 */
void ResetNetworkStreamWithTimestamp(NetStream *pStream);

/**
 * The time network play started counting frames from, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x10220dd8
 * @ghidraAddress BRGlide: 0x1021c908
 */
extern unsigned g_dwNetClockBaseMs;

/**
 * Get the frames of network play since g_dwNetClockBaseMs, at three for each tenth of a second.
 *
 * @return The frames.
 * @ghidraAddress BRD3D: 0x10003460
 * @ghidraAddress BRGlide: 0x100037d0
 */
unsigned GetElapsedFrameCount(void);

/**
 * The frame of the last key frame written to a stream.
 *
 * @ghidraAddress BRD3D: 0x10221310
 * @ghidraAddress BRGlide: 0x1021ce40
 */
extern unsigned g_dwNetLastKeyframeFrame;

/**
 * Empty a stream and write the current frame of network play at its start.
 *
 * @param pStream The stream.
 * @ghidraAddress BRD3D: 0x100048d0
 * @ghidraAddress BRGlide: 0x10004c40
 */
void WriteTimestampToStream(NetStream *pStream);

/**
 * The frame a network race starts on.
 *
 * @ghidraAddress BRD3D: 0x1022af00
 * @ghidraAddress BRGlide: 0x10226a30
 */
extern unsigned g_dwNetRaceStartFrame;

/**
 * Mark the network race started when its start frame has arrived.
 *
 * @ghidraAddress BRD3D: 0x10006160
 * @ghidraAddress BRGlide: 0x100064d0
 */
void CheckNetworkRaceStartTime(void);

/**
 * Destroy the local player of DirectPlay, when there is one.
 *
 * @return The result of DestroyPlayer, or DP_OK without a player.
 * @ghidraAddress BRD3D: 0x1000c4a0
 * @ghidraAddress BRGlide: 0x100099d0
 */
HRESULT DestroyLocalDirectPlayPlayer(void);

/**
 * Limit a value to the range from -1 to 1.
 *
 * @param pValue The value.
 * @ghidraAddress BRD3D: 0x100058d0
 * @ghidraAddress BRGlide: 0x10005c40
 */
void ClampFloat(float *pValue);

/**
 * Limit a coordinate to the range from 0 to 2048.
 *
 * @param pValue The coordinate.
 * @ghidraAddress BRD3D: 0x10005900
 * @ghidraAddress BRGlide: 0x10005c70
 */
void ClampWorldCoordinate(float *pValue);

/**
 * Limit a height to the range from -256 to 256.
 *
 * @param pHeight The height.
 * @ghidraAddress BRD3D: 0x10005930
 * @ghidraAddress BRGlide: 0x10005ca0
 */
void ClampTerrainHeightCoordinate(float *pHeight);

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
 * Whether a lobby launched the game.
 *
 * @ghidraAddress BRD3D: 0x10a9d000
 * @ghidraAddress BRGlide: 0x10ac4090
 */
extern int g_dwLaunchedFromLobby;

/**
 * Whether the lobby made the game the host.
 *
 * @ghidraAddress BRD3D: 0x10aa2884
 * @ghidraAddress BRGlide: 0x10ac5bdc
 */
extern int g_dwLobbyIsHost;

/**
 * Whether an enumeration of sessions is waiting for a reply.
 *
 * @ghidraAddress BRD3D: 0x10aa2898
 * @ghidraAddress BRGlide: 0x10ac5bf0
 */
extern int g_dwSessionEnumPending;

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
 * Whether the critical section of DirectPlay sends exists.
 *
 * @ghidraAddress BRD3D: 0x10277b60
 * @ghidraAddress BRGlide: 0x10273348
 */
extern int g_bDirectPlayReceiveThreadStarted;

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

/** The settings of a network session the game hosts. */
struct NetworkSessionSettings {
    char szSessionName[200]; /*!< The name of the session. */
    BOOL bSecureServer;      /*!< Whether the session is created with DPSESSION_SECURESERVER. */
};

/** The local user and optional authentication fields used when joining a session. */
struct NetworkJoinCredentials {
    char localUserName[200];
    char userName[200];
    char password[200];
    char domain[200];
};

/**
 * @ghidraAddress BRD3D: 0x1022af1c
 * @ghidraAddress BRGlide: 0x10226a4c
 */
extern int g_bNetSpectator;

/**
 * @ghidraAddress BRD3D: 0x1003c260
 * @ghidraAddress BRGlide: 0x100358f0
 */
int JoinNetworkSession(void);

/**
 * @ghidraAddress BRD3D: 0x1003c740
 * @ghidraAddress BRGlide: 0x10035dd0
 */
HRESULT JoinDirectPlaySession(IDirectPlay3A *pDirectPlay,
                              const GUID *instance,
                              NetworkJoinCredentials *credentials,
                              DirectPlayInfo *pInfo);

/**
 * @ghidraAddress BRD3D: 0x1003c5c0
 * @ghidraAddress BRGlide: 0x10035c50
 */
HRESULT CreateDirectPlaySession(IDirectPlay3A *pDirectPlay,
                                NetworkSessionSettings *pSettings,
                                DirectPlayInfo *pInfo);

/**
 * @ghidraAddress BRD3D: 0x1003c150
 * @ghidraAddress BRGlide: 0x100357e0
 */
void HostNetworkSession(void);

/**
 * @ghidraAddress BRD3D: 0x1003cda0
 * @ghidraAddress BRGlide: 0x10036430
 */
HRESULT UpdateNetworkSessionSettings(void);

/**
 * Create every mutex of network play and reset the state of the network.
 *
 * @param nReason Not read. The callers pass 1.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10005b10
 * @ghidraAddress BRGlide: 0x10005e80
 */
bool InitializeNetworkMutexes(int nReason);

/**
 * @ghidraAddress BRD3D: 0x10005960
 * @ghidraAddress BRGlide: 0x10005cd0
 */
bool ResetNetworkState(void);

/**
 * Copy g_szSessionName into the settings of a session when it is longer than one character, and
 * clear whether the session is secure.
 *
 * @param pSettings The settings.
 * @ghidraAddress BRD3D: 0x1003d130
 * @ghidraAddress BRGlide: 0x100367c0
 */
void CopySessionNameToBuffer(NetworkSessionSettings *pSettings);

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
 * Walk the strings of the modem part of an address, and skip any other part.
 *
 * @param guidDataType The type of the part.
 * @param dwDataSize Not read.
 * @param pData The data of the part.
 * @param pContext Not read.
 * @return Always #TRUE, to continue the enumeration.
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
 * @ghidraAddress BRGlide: 0x10036300
 */
HRESULT RefreshNetworkSessionList(IDirectPlay3A *pDirectPlay);

/**
 * @ghidraAddress BRD3D: 0x1003cb10
 * @ghidraAddress BRGlide: 0x100361a0
 */
HRESULT EnumerateNetworkPlayers(GUID *instance,
                                LPDPENUMPLAYERSCALLBACK2 callback,
                                void *context,
                                DWORD flags);

/**
 * @ghidraAddress BRD3D: 0x1003cb90
 * @ghidraAddress BRGlide: 0x10036220
 */
BOOL CALLBACK EnumSessionsCallback_AddSessionToList(LPCDPSESSIONDESC2 description,
                                                    DWORD *timeout,
                                                    DWORD flags,
                                                    void *context);

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
 * @return Always #TRUE, to continue the enumeration.
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
 * @ghidraAddress BRGlide: 0x10ac408c
 */
extern int g_dwDirectPlaySessionActive;

/**
 * Whether the race of a network game has started.
 *
 * @ghidraAddress BRD3D: 0x1022af14
 * @ghidraAddress BRGlide: 0x10226a44
 */
extern int g_bNetRaceStartReached;

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
 * The ticks a request of the end of a pause of a network game has waited, 0 for no request.
 *
 * @ghidraAddress BRD3D: 0x10221314
 * @ghidraAddress BRGlide: 0x1021ce44
 */
extern int g_nNetUnpauseRequestTicks;

/**
 * @ghidraAddress BRD3D: 0x10005090
 * @ghidraAddress BRGlide: 0x10005400
 */
void TickNetworkUnpauseRequest(void);

/**
 * Request the end of a pause from the other players of a network game, when no request waits.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10004c20
 * @ghidraAddress BRGlide: 0x10004f90
 */
bool RequestNetworkUnpause(void);

/**
 * Whether the local player is ready to start a network race.
 *
 * @ghidraAddress BRD3D: 0x1022aaa8
 * @ghidraAddress BRGlide: 0x102265d8
 */
extern int g_bNetReadyToStart;

/**
 * Mark the local player ready to start a network race.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10004bb0
 * @ghidraAddress BRGlide: 0x10004f20
 */
bool SetNetworkReadyToStartFlag(void);

/**
 * Whether the messages received are processed.
 *
 * @ghidraAddress BRD3D: 0x10220dd0
 * @ghidraAddress BRGlide: 0x1021c900
 */
extern int g_bNetReceiveEnabled;

/**
 * Process the messages received.
 *
 * @ghidraAddress BRD3D: 0x10006090
 * @ghidraAddress BRGlide: 0x10006400
 */
void EnableNetworkReceive(void);

/**
 * Stop processing the messages received.
 *
 * @ghidraAddress BRD3D: 0x100060c0
 * @ghidraAddress BRGlide: 0x10006430
 */
void DisableNetworkReceive(void);

/**
 * The slot of the local player of a network game, -1 before one is assigned.
 *
 * @ghidraAddress BRD3D: 0x10094294
 * @ghidraAddress BRGlide: 0x1007b264
 */
extern int g_dwLocalNetPlayerId;

/**
 * Get the slot of the local player of a network game.
 *
 * @return The slot, or -1 before one is assigned.
 * @ghidraAddress BRD3D: 0x10005d30
 * @ghidraAddress BRGlide: 0x100060a0
 */
int GetLocalPlayerIndex(void);

/**
 * Send the request to pause of a network game.
 *
 * @ghidraAddress BRD3D: 0x10004fc0
 * @ghidraAddress BRGlide: 0x10005330
 */
void TickNetworkPauseRequest(void);

/**
 * @ghidraAddress BRD3D: 0x100053f0
 * @ghidraAddress BRGlide: 0x10005760
 */
void MaybeSendAllPlayersReadyPacket(void);

/**
 * @ghidraAddress BRD3D: 0x10005470
 * @ghidraAddress BRGlide: 0x100057e0
 */
int CountActivePlayerSlots(void);

/**
 * @ghidraAddress BRD3D: 0x100046d0
 * @ghidraAddress BRGlide: 0x10004a40
 */
int SendNetworkStreamReliable(DirectPlayInfo *pInfo, NetStream *pStream);

/**
 * @ghidraAddress BRD3D: 0x10004dd0
 * @ghidraAddress BRGlide: 0x10005140
 */
int SendNetworkStreamUnreliable(DirectPlayInfo *pInfo, NetStream *pStream);

/**
 * @ghidraAddress BRD3D: 0x10004760
 * @ghidraAddress BRGlide: 0x10004ad0
 */
int SendPlayerInfoPacket(DirectPlayInfo *pInfo,
                         unsigned char bPlayer,
                         unsigned char bCar,
                         unsigned char bColorR,
                         unsigned char bColorG,
                         unsigned char bColorB,
                         DPID dpidOwner,
                         const char *szPlayerName,
                         int nState,
                         unsigned char bTag);

/**
 * @ghidraAddress BRD3D: 0x1022aefc
 * @ghidraAddress BRGlide: 0x10226a2c
 */
extern unsigned g_dwNetRaceStartTimeMs;

/**
 * Give a message of the main window to the network play.
 *
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @ghidraAddress BRD3D: 0x1003c3a0
 * @ghidraAddress BRGlide: 0x10035a30
 */
LRESULT WINAPI HandleNetworkWindowMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

/**
 * @ghidraAddress BRD3D: 0x100ab408
 * @ghidraAddress BRGlide: 0x100aaba8
 */
extern const RECT g_rcNetCarSetupPlayerList;

/**
 * @ghidraAddress BRD3D: 0x10aa2888
 * @ghidraAddress BRGlide: 0x10ac5be0
 */
extern int g_dwNetworkCarSetupShown;

/** A peer of a network game. The layout is not recovered past the members below. */
struct NetPeerSlot {
    HANDLE hMutex;
    DPID dpidPlayer;
    unsigned dwLastUpdateTime;
    unsigned adwSnapshotTimes[8];
    int dwStatus;
    int nCar;
    unsigned char color[3];
    char abUnknown37; /* +0x37 */
    unsigned adwSnapshotTypes[8];
    KeyframeState aSnapshots[8];
    unsigned nSnapshotCount;
    int nCurrentSnapshot;
    int nAppliedSnapshot;
    unsigned dwPredictionFrame;
    int nRepeatedSnapshotFrames;
    int nLateSnapshotFrames;
    char szName[0x400];
    unsigned dwRaceTime;
    int dwPingMs;
};

struct NetClockSample {
    unsigned dwServerFrame;
    unsigned dwLocalTime;
};

/**
 * @ghidraAddress BRD3D: 0x1022aab0
 * @ghidraAddress BRGlide: 0x102265e0
 */
extern NetClockSample g_aNetClockSamples[8];

/**
 * @ghidraAddress BRD3D: 0x1022af38
 * @ghidraAddress BRGlide: 0x10226a68
 */
extern int g_nNetClockSampleIndex;

/**
 * @ghidraAddress BRD3D: 0x1022af3c
 * @ghidraAddress BRGlide: 0x10226a6c
 */
extern unsigned g_dwNetBestClockRoundTrip;

/**
 * @ghidraAddress BRD3D: 0x1022aaf8
 * @ghidraAddress BRGlide: 0x10226628
 */
extern char g_szSlotPlayerNameBuffer[0x400];

/**
 * @ghidraAddress BRD3D: 0x106909d8
 * @ghidraAddress BRGlide: 0x105ccb80
 */
extern int g_dwNetworkDataReceived;

/**
 * @ghidraAddress BRD3D: 0x1022aef8
 * @ghidraAddress BRGlide: 0x10226a28
 */
extern int g_nPendingNetSoundEffect;

/**
 * @ghidraAddress BRD3D: 0x10005e70
 * @ghidraAddress BRGlide: 0x100061e0
 */
char *GetSlotPlayerName(int nSlot);

/**
 * @ghidraAddress BRD3D: 0x10004910
 * @ghidraAddress BRGlide: 0x10004c80
 */
int SendTimeSyncReply(DirectPlayInfo *pInfo, unsigned timestamp);

/**
 * @ghidraAddress BRD3D: 0x100034a0
 * @ghidraAddress BRGlide: 0x10003810
 */
void SyncNetworkClockFromServer(unsigned serverFrame,
                                unsigned localTime,
                                unsigned echoedFrame,
                                unsigned roundTrip);

/**
 * @ghidraAddress BRD3D: 0x100049c0
 * @ghidraAddress BRGlide: 0x10004d30
 */
void LoadNetworkPlayerColors(void);

/**
 * @ghidraAddress BRD3D: 0x10004590
 * @ghidraAddress BRGlide: 0x10004900
 */
int SendPlayerJoinPacket(DirectPlayInfo *pInfo,
                         unsigned char player,
                         unsigned char red,
                         unsigned char green,
                         unsigned char blue,
                         const char *name,
                         unsigned char tag);

/**
 * @ghidraAddress BRD3D: 0x10004a90
 * @ghidraAddress BRGlide: 0x10004e00
 */
unsigned WaitForPlayerIdAssignment(void);

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
 * @ghidraAddress BRD3D: 0x10005ee0
 * @ghidraAddress BRGlide: 0x10006250
 */
void CopyPlayerNameToSlotBuffer(int slotIndex, const char *name);

/**
 * Get the time of the last round trip to the player of a slot, under the mutex of the slot.
 *
 * @param nSlot The slot.
 * @return The time in milliseconds, and 0 for a negative time.
 * @ghidraAddress BRD3D: 0x10005f90
 * @ghidraAddress BRGlide: 0x10006300
 */
int GetPlayerPingMs(int nSlot);

/**
 * The order of the players of a network race.
 *
 * @ghidraAddress BRD3D: 0x102212d0
 * @ghidraAddress BRGlide: 0x1021ce00
 */
extern int g_adwNetRaceOrder[NET_PEER_SLOT_COUNT];

/**
 * Get an entry of the order of a network race.
 *
 * @param nIndex The index of the entry.
 * @return The entry.
 * @ghidraAddress BRD3D: 0x10005e40
 * @ghidraAddress BRGlide: 0x100061b0
 */
int GetNetworkRaceOrderEntry(int nIndex);

/**
 * The player of the session in a slot of a peer, read under the mutex of the slot.
 *
 * @param nSlot The slot.
 * @return The player.
 * @ghidraAddress BRD3D: 0x10005cf0
 * @ghidraAddress BRGlide: 0x10006060
 */
DPID GetSlotPlayerId(int nSlot);

/**
 * The status of a slot of a peer, read under the mutex of the slot.
 *
 * @param nSlot The slot.
 * @return The status.
 * @ghidraAddress BRD3D: 0x10004a10
 * @ghidraAddress BRGlide: 0x10004d80
 */
int GetSlotStatusThreadSafe(int nSlot);

/**
 * The status of a slot of a peer in its low 6 bits, less 4, under the mutex of the slot.
 *
 * @param nSlot The slot.
 * @return The status less 4, or 0 for a status of 4 or less.
 * @ghidraAddress BRD3D: 0x10005f40
 * @ghidraAddress BRGlide: 0x100062b0
 */
int GetNormalizedSlotStatusFlags(int nSlot);

/**
 * The number of players in the network session.
 *
 * @return The number, or 0xffff when the description of the session is not available.
 * @ghidraAddress BRD3D: 0x1000c670
 * @ghidraAddress BRGlide: 0x10009ba0
 */
DWORD GetDirectPlaySessionPlayerCount(void);

/**
 * Set the status of a slot of a peer under the mutex of the slot.
 *
 * @param nSlot The slot.
 * @param dwStatus The status.
 * @ghidraAddress BRD3D: 0x10004a50
 * @ghidraAddress BRGlide: 0x10004dc0
 */
void SetSlotStatusThreadSafe(int nSlot, int dwStatus);

/**
 * The line of status of network play the game shows. Its size is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10220e80
 * @ghidraAddress BRGlide: 0x1021c9b0
 */
extern char g_szNetStatusString[];

/**
 * Whether g_szNetStatusString changed since it was shown.
 *
 * @ghidraAddress BRD3D: 0x1022af08
 * @ghidraAddress BRGlide: 0x10226a38
 */
extern int g_bNetStatusStringDirty;

/**
 * @ghidraAddress BRD3D: 0x100060f0
 * @ghidraAddress BRGlide: 0x10006460
 */
void ProcessPendingNetworkNotifications(void);

/**
 * @ghidraAddress BRD3D: 0x10005de0
 * @ghidraAddress BRGlide: 0x10006150
 */
unsigned ReadPlayerSlotColorRgb(int slotIndex,
                                unsigned char *red,
                                unsigned char *green,
                                unsigned char *blue);

/**
 * @ghidraAddress BRD3D: 0x1002f130
 * @ghidraAddress BRGlide: 0x1001c6a0
 */
void AddNetworkPlayerSlot(int slotIndex);

/**
 * @ghidraAddress BRD3D: 0x1002f230
 * @ghidraAddress BRGlide: 0x1001c7a0
 */
void RemoveNetworkPlayerById(int slotIndex);

/**
 * Set the line of status of network play under the mutex of the notices.
 *
 * @param pszStatus The line.
 * @ghidraAddress BRD3D: 0x10003530
 * @ghidraAddress BRGlide: 0x100038a0
 */
void SetGlobalStatusString(const char *pszStatus);

/** The number of entries of g_anDepartedPlayerSlotStack and g_anJoinedPlayerSlotStack. */
#define PLAYER_SLOT_STACK_SIZE 16

/**
 * The slots of the players that departed, for the race to remove.
 *
 * @ghidraAddress BRD3D: 0x10221288
 * @ghidraAddress BRGlide: 0x1021cdb8
 */
extern int g_anDepartedPlayerSlotStack[PLAYER_SLOT_STACK_SIZE];

/**
 * The index of the top of g_anDepartedPlayerSlotStack.
 *
 * @ghidraAddress BRD3D: 0x10221318
 * @ghidraAddress BRGlide: 0x1021ce48
 */
extern int g_nDepartedPlayerStackTop;

/**
 * The slots of the players that joined, for the race to add.
 *
 * @ghidraAddress BRD3D: 0x10220d90
 * @ghidraAddress BRGlide: 0x1021c8c0
 */
extern int g_anJoinedPlayerSlotStack[PLAYER_SLOT_STACK_SIZE];

/**
 * The index of the top of g_anJoinedPlayerSlotStack.
 *
 * @ghidraAddress BRD3D: 0x10220dd4
 * @ghidraAddress BRGlide: 0x1021c904
 */
extern int g_nJoinedPlayerStackTop;

/**
 * Pop the slot of a player that joined, under the mutex of the stack.
 *
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d40
 * @ghidraAddress BRGlide: 0x100060b0
 */
int PopJoinedPlayerSlot(void);

/**
 * Pop the slot of a player that departed, under the mutex of the stack.
 *
 * @return The slot, or -1 when the stack is empty.
 * @ghidraAddress BRD3D: 0x10005d90
 * @ghidraAddress BRGlide: 0x10006100
 */
int PopDepartedPlayerSlot(void);

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
extern int g_bNetWorkerThreadRunning;

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
 * The identifier of the worker thread of network play.
 *
 * @ghidraAddress BRD3D: 0x11826b04
 * @ghidraAddress BRGlide: 0x11849e64
 */
extern DWORD g_dwNetWorkerThreadId;

/**
 * The time of the next tick of the worker thread of network play, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x11826b48
 * @ghidraAddress BRGlide: 0x11849ea8
 */
extern DWORD g_dwNetWorkerNextTickTime;

/**
 * Move the players of a network game that are ready to the start.
 *
 * @ghidraAddress BRD3D: 0x100716e0
 * @ghidraAddress BRGlide: 0x1006a650
 */
void PromoteReadyNetworkPlayersToStart(void);

/**
 * Rank the players of a network game by their progress.
 *
 * @ghidraAddress BRD3D: 0x10071870
 * @ghidraAddress BRGlide: 0x1006a7e0
 */
void RankNetworkPlayersByProgress(void);

/**
 * Empty the streams of every player of a network game.
 *
 * @ghidraAddress BRD3D: 0x10071b80
 * @ghidraAddress BRGlide: 0x1006aaf0
 */
void ResetAllNetworkPlayerStreams(void);

/**
 * Write the packets of the status of the players of a network game into their streams.
 *
 * @ghidraAddress BRD3D: 0x10071c10
 * @ghidraAddress BRGlide: 0x1006ab80
 */
void BuildNetworkPlayerStatusPackets(void);

/**
 * Send the streams of the players of a network game.
 *
 * @param pInfo The session.
 * @ghidraAddress BRD3D: 0x10072170
 * @ghidraAddress BRGlide: 0x1006b0e0
 */
int FlushNetworkPlayerStreams(DirectPlayInfo *pInfo);

/**
 * Do the work of the host of a network game once a second, sleeping until each second. The loop
 * never ends: it reads no stop event, so the thread never returns.
 *
 * @param pParameter Not read.
 * @return Never.
 * @ghidraAddress BRD3D: 0x10071680
 * @ghidraAddress BRGlide: 0x1006a5f0
 */
DWORD WINAPI NetworkWorkerThreadProc(LPVOID pParameter);

/**
 * Create the mutexes of the slots of the players of a network game.
 *
 * @ghidraAddress BRD3D: 0x10071560
 * @ghidraAddress BRGlide: 0x1006a4d0
 */
void CreateNetworkPlayerSlotMutexes(void);

/**
 * @ghidraAddress BRD3D: 0x100713c0
 * @ghidraAddress BRGlide: 0x1006a330
 */
void ResetAllPlayerSlots(void);

/**
 * @ghidraAddress BRD3D: 0x1178feec
 * @ghidraAddress BRGlide: 0x117b324c
 */
extern DWORD g_dwNetSlotResetTime;

/**
 * @ghidraAddress BRD3D: 0x11826af8
 * @ghidraAddress BRGlide: 0x11849e58
 */
extern unsigned g_dwNetNextFinishPosition;

/**
 * @ghidraAddress BRD3D: 0x1003db50
 * @ghidraAddress BRGlide: 0x100371f0
 */
HRESULT SendNetworkFinishPositionMessage(DirectPlayInfo *pInfo, DPID dpidPlayer, DWORD dwPosition);

/**
 * Start the thread that works for the host of a network game.
 *
 * @ghidraAddress BRD3D: 0x10071630
 * @ghidraAddress BRGlide: 0x1006a5a0
 */
void StartNetworkWorkerThread(void);

/**
 * Create the mutexes of the slots of the players and start the worker thread of a network game.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10071550
 * @ghidraAddress BRGlide: 0x1006a4c0
 */
bool InitializeNetworkThreading(void);

/**
 * The time the network session started, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x1178fee8
 * @ghidraAddress BRGlide: 0x117b3248
 */
extern DWORD g_dwNetSessionStartTime;

/**
 * Get the time since the network session started.
 *
 * @return The time in milliseconds.
 * @ghidraAddress BRD3D: 0x100713a0
 * @ghidraAddress BRGlide: 0x1006a310
 */
DWORD GetElapsedTimeSinceStart(void);

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
 * Remove a player that departed from a race of the network. Each slot of a peer of the player
 * with a status is pushed on g_anDepartedPlayerSlotStack and cleared, and the line of status
 * reports that the player left.
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
 * @ghidraAddress BRD3D: 0x10036030
 * @ghidraAddress BRGlide: 0x1002f6d0
 */
int FindNetworkSlotByPlayerId(DPID dpidPlayer);

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
 * Get the description of the session of DirectPlay in a buffer of locked global memory.
 *
 * @param pDirectPlay The DirectPlay object.
 * @param ppDesc Receives the description, for the caller to unlock and free.
 * @return The result of GetSessionDesc, or `E_OUTOFMEMORY` when the buffer cannot be allocated.
 * @ghidraAddress BRD3D: 0x1003d0b0
 * @ghidraAddress BRGlide: 0x10036740
 */
HRESULT GetDirectPlaySessionDesc(IDirectPlay3A *pDirectPlay, DPSESSIONDESC2 **ppDesc);

/**
 * Read the track, the weather, the class of car, the laps, and the name of the network session
 * from its description, and choose a car of the class that can be chosen.
 *
 * @return `DP_OK`, the error of GetDirectPlaySessionDesc(), or `DPERR_INVALIDOBJECT` without
 * DirectPlay.
 * @ghidraAddress BRD3D: 0x1003ce80
 * @ghidraAddress BRGlide: 0x10036510
 */
HRESULT LoadNetworkSettings(void);

/** A player in a slot of the session. */
struct SessionSlot {
    DPID dpidPlayer; /*!< The player, or 0xffffffff for a free slot. */
    int nFlag;       /*!< Set for the host when the slot is taken, cleared when the player departs,
                          and sent by SendNetworkReadyMessage(). */
    int nSeen;       /*!< Whether an enumeration of the players found the player. */
};

/**
 * Whether the sends of network play are blocked.
 *
 * @ghidraAddress BRD3D: 0x10aa288c
 * @ghidraAddress BRGlide: 0x10ac5be4
 */
extern int g_dwNetworkSendBlocked;

/**
 * Free every slot of the session and unblock the sends.
 *
 * @ghidraAddress BRD3D: 0x100586a0
 * @ghidraAddress BRGlide: 0x10051550
 */
void ResetSessionSlotTable(void);

/** A network player of a race. */
struct NetPlayerSlot {
    HANDLE hMutex;  /*!< The mutex of the slot. */
    DPID dpidOwner; /*!< The player of the session that controls the slot. */
    DWORD dwLastUpdateTime;
    unsigned adwSnapshotTimes[8];
    int dwState; /*!< The state of the slot in its low 6 bits, 0 for free. */
    int nStatusValue;
    unsigned char bStatusOptionA;
    unsigned char bStatusOptionB;
    unsigned char bStatusOptionC;
    char abUnknown37; /* +0x37 */
    unsigned adwSnapshotTypes[8];
    KeyframeState aSnapshots[8];
    int nCurrentSnapshot;
    char szPlayerName[0x400];
    unsigned dwLastTimeSync;
    unsigned dwRaceTime;
    unsigned dwTimeSyncValue;
    unsigned dwFinishTime;
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
 * The network players of a race as each peer reports them, by peer and then by player.
 *
 * @ghidraAddress BRD3D: 0x1178fef8
 * @ghidraAddress BRGlide: 0x117b3258
 */
extern NetPlayerSlot g_aNetPeerSlotMatrix[NET_PLAYER_SLOT_COUNT][NET_PLAYER_SLOT_COUNT];

/**
 * @ghidraAddress BRD3D: 0x1178fef0
 * @ghidraAddress BRGlide: 0x117b3250
 */
extern int g_bNetRaceStarted;

/**
 * The state of a network player of a race, under the mutex of its slot.
 *
 * @param nSlot The slot.
 * @return The state.
 * @ghidraAddress BRD3D: 0x100714d0
 * @ghidraAddress BRGlide: 0x1006a440
 */
int GetNetworkPlayerSlotStatus(int nSlot);

/**
 * The state of a network player as its own peer reports it, under the mutex of the entry.
 *
 * @param nSlot The slot of the player and of its peer.
 * @return The state.
 * @ghidraAddress BRD3D: 0x10071510
 * @ghidraAddress BRGlide: 0x1006a480
 */
int GetNetworkPeerChannelStatus(int nSlot);

/**
 * Remove entries from the menu of network sessions.
 *
 * @ghidraAddress BRD3D: 0x1003d070
 * @ghidraAddress BRGlide: 0x10036700
 */
void DestroyNetworkGroups(void);

/**
 * Start the service provider of network play chosen.
 *
 * @ghidraAddress BRD3D: 0x1003c020
 * @ghidraAddress BRGlide: 0x100356b0
 */
void InitializeNetworkServiceProvider(void);

/**
 * @ghidraAddress BRD3D: 0x1003d480
 * @ghidraAddress BRGlide: 0x10036b20
 */
HRESULT CreateServiceProviderAddress(void **ppAddress, DWORD *pdwSize);

/**
 * Start the service provider chosen and the timer of network play of the main window, and mark the
 * DirectPlay session open.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1003c230
 * @ghidraAddress BRGlide: 0x100358c0
 */
bool InitServiceProviderAndStartTimer(void);

/**
 * Start the service provider chosen and the timer of network play as
 * InitServiceProviderAndStartTimer() does, then enumerate the sessions again while the list of
 * sessions exists.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1003c1e0
 * @ghidraAddress BRGlide: 0x10035870
 */
bool InitServiceProviderAndStartSession(void);

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
 * Mark the slot of a player of the session as found, or give the player the first free slot.
 * The flag of a new slot is set for the host and cleared for another player. A player is dropped
 * when no slot is free.
 *
 * @param dpidPlayer The player.
 * @ghidraAddress BRD3D: 0x1003c9f0
 * @ghidraAddress BRGlide: 0x10036080
 */
void MarkSessionListEntrySeen(DPID dpidPlayer);
/**
 * @ghidraAddress BRD3D: 0x1003caa0
 * @ghidraAddress BRGlide: 0x10036130
 */
BOOL CALLBACK EnumPlayersCallback_AddPlayerToList(
    DPID dpidPlayer, DWORD dwPlayerType, LPCDPNAME pName, DWORD dwFlags, void *pContext);

/**
 * Free each slot of the session whose player was not found since the last purge.
 *
 * @ghidraAddress BRD3D: 0x1003ca70
 * @ghidraAddress BRGlide: 0x10036100
 */
void PurgeStaleSessionListEntries(void);

/**
 * Get the flag of the slot of a player of the session.
 *
 * @param dpidPlayer The player.
 * @return The flag, or 0 when no slot has the player.
 * @ghidraAddress BRD3D: 0x100586d0
 * @ghidraAddress BRGlide: 0x10051580
 */
int GetSessionSlotFlag(DPID dpidPlayer);

/**
 * The message the main window receives with a notice of network play in locked global memory as
 * the second parameter. The window frees the notice.
 */
#define WM_NETWORK_NOTICE 0x501

/**
 * @ghidraAddress BRD3D: 0x10a9dd28
 * @ghidraAddress BRGlide: 0x10ac4db0
 */
extern char g_szNetworkChatText[];

/**
 * @ghidraAddress BRD3D: 0x1003d210
 * @ghidraAddress BRGlide: 0x100368a0
 */
HRESULT SendNetworkChatMessage(HWND hWindow, DirectPlayInfo *pInfo, bool bNotice);

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
    DirectPlayInfo *pInfo, void *pMessage, DWORD dwSize, DPID dpidFrom, DPID dpidTo);

/**
 * @ghidraAddress BRD3D: 0x1003d180
 * @ghidraAddress BRGlide: 0x10036810
 */
HRESULT GetDirectPlayPlayerName(IDirectPlay3A *pDirectPlay, DPID dpidPlayer, DPNAME **ppName);

/**
 * @ghidraAddress BRD3D: 0x1003d3a0
 * @ghidraAddress BRGlide: 0x10036a30
 */
HRESULT FormatNetworkChatLine(
    IDirectPlay3A *pDirectPlay, DPID dpidPlayer, const char *text, char **ppLine, bool bNotice);

/**
 * @ghidraAddress BRD3D: 0x10aa2894
 * @ghidraAddress BRGlide: 0x10ac5bec
 */
extern int g_dwNetworkDataStreamActive;
/**
 * @ghidraAddress BRD3D: 0x10aa2890
 */
extern int g_dwNetworkEscapePending;
/**
 * @ghidraAddress BRD3D: 0x10058700
 * @ghidraAddress BRGlide: 0x100515b0
 */
int ToggleSelectedSessionSlotFlag(void);

/**
 * @ghidraAddress BRD3D: 0x100aac48
 * @ghidraAddress BRGlide: 0x100aa3e8
 */
extern const char *g_apszPlaceNames[8];

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
 * @ghidraAddress BRGlide: 0x100355f0
 */
void DisconnectNetworkSession(void);
#ifdef __cplusplus
}
#endif
