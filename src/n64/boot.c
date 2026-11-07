#include "n64/boot.h"

#include <PR/os_libc.h>

#include "common/game.h"
#include "common/libultra/os/osint.h"
#include "common/model.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/vecmath.h"

#define LOAD_MESSAGE_SIZE 0x50
#define FAULT_MESSAGE 0x10
#define IO_MESSAGES 32
#define FAULT_THREAD_ID 5
#define FAULT_THREAD_PRIORITY 50
#define MAIN_THREAD_ID 3
#define MAIN_THREAD_PRIORITY 10
#define IDLE_THREAD_ID 1
#define IDLE_THREAD_PRIORITY 10
#define BOOT_ARGUMENTS_ADDRESS 0xFFB000
#define BOOT_ARGUMENT_WORDS 16
#define STACK_FILL_VALUE 0x5015A1DBFED15C00ULL
#define BOOT_SCREEN_WIDTH 320
#define BOOT_SCREEN_HEIGHT 240
#if TGR_REGION == TGR_REGION_US
#define VI_MODE_LAN1_MPAL g_ViModeMpalLan1
#define VI_MODE_LAN1 g_ViModeNtscLan1
#elif TGR_REGION == TGR_REGION_EU
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModePalLan1
#else
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModeNtscLan1
#endif
#define PAL_VSTART_KEEP 0xFC00FC00
#define PAL_HAF1_YSCALE 0x6F0
#define PAL_LAN1_YSCALE 0x378
#define BOOT_CLEAR_START 0x800D4000
#define BOOT_CLEAR_END 0x80200000
/* The ROM treats the literal destination separately from g_LoadedTrack's symbol. */
#define TRACK_LOAD_DESTINATION ((void *)0x80025C00)

Model *DecompressModel(void *dst, u32 romOffset) {
    DecompressFromRom(dst, romOffset, NULL);
    RelocateModelPointers(dst);
    return dst;
}

void LoadTrackGeometryFromRom(s32 track) {
    /* The messages stay where the debug display points until it shows them. */
    char tooBig[LOAD_MESSAGE_SIZE];
    union {
        Vec3f vector;
        f32 values[3];
    } axis;
    s32 unrotated;
    f32 length;
    char tooManyInstances[LOAD_MESSAGE_SIZE + 4];
    char headerMismatch[LOAD_MESSAGE_SIZE];

    g_LoadedTrack.size = DmaReadAssetSize(g_aTrackDescriptors[track].trackRom);
    osInvalDCache(&g_LoadedTrack, sizeof(g_LoadedTrack.size));
    if (g_LoadedTrack.size > TRACK_MAX_SIZE) {
        sprintf(tooBig, "Track %d too big (%d vs. %d)", track, g_LoadedTrack.size, TRACK_MAX_SIZE);
        SetDebugMessage(tooBig);
    } else {
        osSyncPrintf("Loading track %d (%d / %d)\n", track, g_LoadedTrack.size, TRACK_MAX_SIZE);
    }
    DecompressFromRom(TRACK_LOAD_DESTINATION, g_aTrackDescriptors[track].trackRom, NULL);
    /* The ROM loads these two words from independent absolute addresses. */
    osSyncPrintf("hmm = %04x %04x\n", *(u16 *)0x80029560, *(u16 *)0x80029568);
    osInvalDCache(TRACK_LOAD_DESTINATION, g_LoadedTrack.size);
    DmaRead(
        g_abTrackSection1Header, g_aTrackDescriptors[track].sectionARom, TRACK_SECTION_HEADER_SIZE);
    DecompressFromRom(g_abTrackSection1Data,
                      g_aTrackDescriptors[track].sectionARom + TRACK_SECTION_HEADER_SIZE,
                      NULL);
    DmaRead(
        g_abTrackSection2Header, g_aTrackDescriptors[track].sectionBRom, TRACK_SECTION_HEADER_SIZE);
    DecompressFromRom(g_abTrackSection2Data,
                      g_aTrackDescriptors[track].sectionBRom + TRACK_SECTION_HEADER_SIZE,
                      NULL);
    g_dwCurrentTrackRomOffset = g_aTrackDescriptors[track].trackRom;
    g_dwTrackTextureRomOffset =
        g_aTrackDescriptors[track].trackRom + DmaReadWord(g_aTrackDescriptors[track].trackRom);
    g_dwStreamDmaPrevSegment = -1;
    g_dwStreamedSnowMode = -1;

    /* Mark the placed models whose matrix holds only a position and a scale, and store the inverse
     * of the scale of each. */
    unrotated = 0;
    for (track = 0; track < g_LoadedTrack.instanceCount; track++) {
        axis.values[0] = 1.0f;
        axis.values[1] = 0.0f;
        axis.values[2] = 0.0f;
        Mtx3x3TransformVec(&axis.vector, &axis.vector, g_LoadedTrack.instances[track].matrix);
        length = Vec3fLength(&axis.vector);
        if (length != 0) {
            length = 1.0f / length;
            if (1.0f == g_LoadedTrack.instances[track].matrix[0][0] * length &&
                1.0f == g_LoadedTrack.instances[track].matrix[1][1] * length &&
                1.0f == g_LoadedTrack.instances[track].matrix[2][2] * length) {
                unrotated++;
                g_LoadedTrack.instances[track].flags |= TRACK_INSTANCE_UNROTATED;
            }
            g_LoadedTrack.instances[track].invScale = length;
        }
    }
    osSyncPrintf("Scalars: %d/%d\n", unrotated, g_LoadedTrack.instanceCount);
    if (g_LoadedTrack.instanceCount > TRACK_MAX_INSTANCES) {
        sprintf(tooManyInstances,
                "ERROR: instances (%d) > MAX_INSTANCES (%d)",
                g_LoadedTrack.instanceCount,
                TRACK_MAX_INSTANCES);
        SetDebugMessage(tooManyInstances);
    }
    if (g_LoadedTrack.headerSize != TRACK_HEADER_SIZE) {
        sprintf(headerMismatch,
                "ERROR: Track header size mismatch! (%d!=%d)",
                g_LoadedTrack.headerSize,
                TRACK_HEADER_SIZE);
        SetDebugMessage(headerMismatch);
    }
    PrepareTrackSections();
}

s32 IsTrackIndexValid(s32 track) {
    return track < g_dwNumTracks && g_aTrackDescriptors[track].trackRomEnd != 0;
}

void WaitForVblankMessage(void) {
    osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
}

void StubFunction(s32 unused) {
}

void SetDebugMessage(const char *message) {
    g_pszDebugMessage = message;
    StubFunction(0);
}

void FaultThread(void *arg) {
    OSMesg message;
#ifndef __GNUC__
    static OSThread *g_pFaultedThread;
#endif

    osSetEventMesg(OS_EVENT_FAULT, &g_FaultMsgQueue, (OSMesg)FAULT_MESSAGE);
    g_dwFaultState = 0;
    for (;;) {
        osRecvMesg(&g_FaultMsgQueue, &message, OS_MESG_BLOCK);
        osSyncPrintf("\n=> faultproc - got a fault message...\n");
        g_pFaultedThread = __osGetCurrFaultedThread();
        if (g_pFaultedThread != NULL) {
            StubFunction((s32)g_pFaultedThread);
        }
    }
}

void BootIdleThread(void *arg) {
    struct BootArgs {
        s32 argc;
        char **argv;
    };
    /**
     * @ghidraAddress USA: 0x8028b2ec
     * @ghidraAddress Asia: 0x8028AE3C
     * @ghidraAddress EUR: 0x8028d59c
     * @ghidraAddress JPN: 0x8028cfcc
     */
    static struct BootArgs args = {1, g_apszBootArgv};
    char *p;
    u32 i;
    u32 arguments[BOOT_ARGUMENT_WORDS];
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    s32 unused[2];
#endif
    u64 *stackFill;

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    (void)unused;
#endif
    for (i = 0; i < BOOT_ARGUMENT_WORDS; i++) {
        osPiRawReadIo(BOOT_ARGUMENTS_ADDRESS + i * sizeof(u32), &arguments[i]);
    }

    p = (char *)arguments;
    while (*p != '\0') {
        while (*p != '\0' && *p == ' ') {
            *p++ = '\0';
        }
        if (*p != '\0') {
            g_apszBootArgv[args.argc] = p;
            args.argc++;
        }
        while (*p != '\0' && *p != ' ') {
            p++;
        }
    }
    g_ppszMainArgv = args.argv;
    g_pdwMainArgc = &args.argc;

    osCreateViManager(OS_PRIORITY_VIMGR);
    g_dwScreenWidth = BOOT_SCREEN_WIDTH;
    g_dwScreenHeight = BOOT_SCREEN_HEIGHT;
#if TGR_REGION == TGR_REGION_EU
    memcpy(&g_ViModePalHaf1, &osViModePalHaf1, sizeof(OSViMode));
    memcpy(&g_ViModePalLan1, &osViModePalLan1, sizeof(OSViMode));
    g_ViModePalHaf1.fldRegs[0].vStart =
        (osViModePalHaf1.fldRegs[0].vStart & PAL_VSTART_KEEP) | 0x390000 | 0x25B;
    g_ViModePalHaf1.fldRegs[1].vStart =
        (osViModePalHaf1.fldRegs[1].vStart & PAL_VSTART_KEEP) | 0x3B0000 | 0x25D;
    g_ViModePalHaf1.fldRegs[0].yScale = PAL_HAF1_YSCALE;
    g_ViModePalHaf1.fldRegs[1].yScale = PAL_HAF1_YSCALE;
    g_ViModePalLan1.fldRegs[0].vStart =
        (osViModePalLan1.fldRegs[0].vStart & PAL_VSTART_KEEP) | 0x390000 | 0x25B;
    g_ViModePalLan1.fldRegs[1].vStart =
        (osViModePalLan1.fldRegs[1].vStart & PAL_VSTART_KEEP) | 0x3B0000 | 0x25D;
    g_ViModePalLan1.fldRegs[0].yScale = PAL_LAN1_YSCALE;
    g_ViModePalLan1.fldRegs[1].yScale = PAL_LAN1_YSCALE;
#endif /* TGR_REGION == TGR_REGION_EU */
    if (osTvType == OS_TV_MPAL) {
        osViSetMode(&VI_MODE_LAN1_MPAL);
    } else {
        osViSetMode(&VI_MODE_LAN1);
    }
    osViBlack(TRUE);
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    g_dwDisplayBlanked = TRUE;
    for (i = BOOT_CLEAR_START / 4; i < BOOT_CLEAR_END / 4; i++) {
        ((u32 *)0)[i] = 0;
    }
#endif
    osCreatePiManager(OS_PRIORITY_PIMGR, &g_PiMgrCmdQueue, g_aPiMgrCmdMsgBuf, IO_MESSAGES);
    osCreateMesgQueue(&g_FaultMsgQueue, g_aFaultMsgBuf, 1);
    osCreateThread(&g_FaultThreadObj,
                   FAULT_THREAD_ID,
                   FaultThread,
                   args.argv,
                   &g_adwFaultThreadStack[FAULT_STACK_WORDS],
                   FAULT_THREAD_PRIORITY);
    osStartThread(&g_FaultThreadObj);

    for (i = 0, stackFill = (u64 *)g_adwMainThreadStack; i < MAIN_STACK_WORDS / 2;
         i++, stackFill++) {
        *stackFill = STACK_FILL_VALUE;
    }
    for (i = 0, stackFill = (u64 *)g_adwRspDoneThreadStack; i < THREAD_STACK_WORDS / 2;
         i++, stackFill++) {
        *stackFill = STACK_FILL_VALUE;
    }
    for (i = 0; i < THREAD_STACK_WORDS / 2; i++) {
        ((u64 *)g_adwRdpDoneThreadStack)[i] = STACK_FILL_VALUE;
    }
    osCreateThread(&g_MainGameThreadObj,
                   MAIN_THREAD_ID,
                   (void (*)(void *))InitGameAndShowMenus,
                   arg,
                   &g_adwMainThreadStack[MAIN_STACK_WORDS],
                   MAIN_THREAD_PRIORITY);
    osStartThread(&g_MainGameThreadObj);
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
    for (;;) {
    }
}

void OsBootEntry(void) {
#if TGR_REGION == TGR_REGION_EU
    /* The PAL release forces the television standard before osInitialize() reads it. */
    osTvType = OS_TV_PAL;
    osViClock = VI_PAL_CLOCK;
#endif
    osInitialize();
    osCreateThread(&g_BootIdleThreadObj,
                   IDLE_THREAD_ID,
                   BootIdleThread,
                   NULL,
                   &g_adwIdleThreadStack[IDLE_STACK_WORDS],
                   IDLE_THREAD_PRIORITY);
    osStartThread(&g_BootIdleThreadObj);
}
