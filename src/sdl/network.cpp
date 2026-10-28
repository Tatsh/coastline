// The network play of the PC release ran over DirectPlay, which the SDL game does not have. These
// are the routines of src/win32/directplay.cpp the rest of the game calls, each returning what it
// returns outside a session. g_dwNetworkPlayMode remains 0, and the game does not take a network
// path.

#include "win32/directplay.h"
#include "win32/frametimer.h"
#include "win32/netstream.h"

int g_bNetSpectator;
int g_dwNetworkPlayMode;
const char g_szEmptyUiString[1] = {0};
int g_bNetRaceStartReached;
int g_dwNetworkDataReceived;

void AddNetworkPlayerSlot(int slotIndex) {
}

void CheckNetworkRaceStartTime(void) {
}

void CopyPlayerNameToSlotBuffer(int slotIndex, const char *name) {
}

void DisableNetworkReceive(void) {
}

void EnableNetworkReceive(void) {
}

DWORD GetDirectPlaySessionPlayerCount(void) {
    return 0;
}

int GetLocalPlayerIndex(void) {
    return 0;
}

int GetNetworkPeerChannelStatus(int nSlot) {
    return 0;
}

int GetNetworkPlayerSlotStatus(int nSlot) {
    return 0;
}

int GetNetworkRaceOrderEntry(int nIndex) {
    return nIndex;
}

int GetNormalizedSlotStatusFlags(int nSlot) {
    return 0;
}

int GetPlayerPingMs(int nSlot) {
    return 0;
}

int PopDepartedPlayerSlot(void) {
    return -1;
}

int PopJoinedPlayerSlot(void) {
    return -1;
}

void ProcessPendingNetworkNotifications(void) {
}

void RemoveNetworkPlayerById(int slotIndex) {
}

bool RequestNetworkUnpause(void) {
    return false;
}

void ResetAllPlayerSlots(void) {
}

bool ResetNetworkState(void) {
    g_dwNetworkPlayMode = 0;
    return true;
}

void SendNetworkMessage6ToSession(void) {
}

void TickNetworkUnpauseRequest(void) {
}

unsigned WaitForPlayerIdAssignment(void) {
    return 0;
}

int g_dwLocalNetPlayerId = -1;
NetPeerSlot g_aNetPeerSlots[NET_PEER_SLOT_COUNT];
unsigned g_dwNetRaceStartCountdownTime;
unsigned g_dwNetClockBaseMs;
HANDLE g_hNetKeyframeMutex;

// The routines below have the logic of their originals in src/win32/directplay.cpp, for the
// conversions of the states of vehicles in netstream.cpp.

unsigned GetElapsedFrameCount(void) {
    unsigned dwElapsed = GetFrameTickCounter() - g_dwNetClockBaseMs;
    return dwElapsed / TICK_PERIOD_MS * TICKS_PER_PERIOD +
           dwElapsed % TICK_PERIOD_MS / TICK_INTERVAL_MS;
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

void MaybeSendAllPlayersReadyPacket(void) {
}

bool RequestNetworkPause(void) {
    return true;
}

unsigned SendKeyframePacket(KeyframeState *state) {
    return false;
}

unsigned SendDeltaKeyframePacket(KeyframeState *state, KeyframeState *base) {
    return false;
}

bool SetNetworkReadyToStartFlag(void) {
    return true;
}

void TickNetworkPauseRequest(void) {
}
