// The variables of the race engine that the Windows libraries define in their platform units, and
// the console in its data. Each has the type, the size, and the first value of the Windows
// definition.
#include "common/frontendstate.h"
#include "common/racephases.h"
#include "common/render.h"
#include "common/session.h"
#include "common/settings.h"
#include "common/snapshot.h"
#include "common/trackskidmarks.h"
#include "common/vehicledeform.h"

int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];
int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];
// The path of BossRally.cfg. A path of the host may be longer than a path of Windows.
char g_szConfigFilePath[4096];

// The front end, the game session, and the network session.
int g_dwChosenCar;
int g_dwInNetworkCarSetup;
char g_szSeasonSaveFileName[FRONT_END_SAVE_PATH_SIZE];
char g_szTimeAttackSaveFileName[FRONT_END_SAVE_PATH_SIZE];
int g_dwFrontEndAtRootMenu;
int g_dwSelectedTransmission;
int g_dwTireMenuIndex = 1;
int g_dwSuspensionMenuIndex = 1;
int g_dwCarMenuIndex = 1;
int g_dwTotalLapsSetting = 3;
int g_dwAccumUnlockedCarMask;
int g_dwAccumUnlockedTrackMask;
int g_dwHandlingTypeMenuIndex = 4;
s16 g_wVehicleDeformMaxDent = 0x100;
int g_bRaceStartReached;
int g_dwRunBenchmark;
int g_dwNetworkPlayMode;
const char g_szEmptyUiString[1] = {0};
int g_dwNullStubEnable = 1;
