#include "carselect.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "audiomix.h"
#include "camera.h"
#include "controllerscreen.h"
#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "mainmenu.h"
#include "options.h"
#include "paintshop.h"
#include "playerselect.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "texrect.h"
#include "text.h"
#include "timing.h"
#include "trackselect.h"
#include "util.h"
#include "vehiclerender.h"
#include "visgrid.h"
#include "weather.h"
#include "weatherselect.h"

/* The sounds of moving the cursor and of choosing, on the cartridge. */
#define ROM_SCROLL_SOUND ((u32)_scrollSoundSegmentRomStart)
#define ROM_SELECT_SOUND ((u32)_selectSoundSegmentRomStart)

/* The silence after each sound, in bytes. */
#define SOUND_TAIL_BYTES 0x100

/* The time a fade of the screen in or out takes, as its rate. */
#define SCREEN_FADE_RATE 0.2f

/* The height of the cars over their stage. */
#define STAGE_HEIGHT 0.254f

/* The scale of a model on a stage, and the degrees of a radian. */
#define STAGE_MODEL_SCALE 0.001302083372f
#define RAD_TO_DEG 57.29577637f

/* The colours of a progress bar: its frame, its background and its filling. */
#define BAR_FRAME_COLOR 0x0001
#define BAR_BACK_COLOR 0x01C1
#define BAR_FILL_COLOR 0x0781

/* The size of the bars of the speed and acceleration of a car, and their full values. */
#define STAT_BAR_WIDTH 80
#define STAT_BAR_HEIGHT 11
#define FULL_ACCELERATION 500.0
#define FULL_SPEED 10.0

/* The cars the paint shop allows, and the first car whose decals have no custom choice. */
#define PAINT_SHOP_CARS 9

/* The cars the players' views spread apart by when they slide. */
#define SPREAD_ONE_VIEW 8.0f
#define SPREAD_TWO_VIEWS 12.0f

/* The steps of the colour of the paint, of 8 bits a channel. */
#define PAINT_STEP 4

/* The tracks whose names are mirrored ones, past the first five. */
#define MIRRORED_TRACKS 5

/* The unlock that lets a car be switched to its other look. */
#define UNLOCK_OTHER_LOOK 0x8000

/* The buttons that choose. */
#define CHOOSE_BUTTONS (INPUT_A | INPUT_START | INPUT_Z)

/* The car a player has selected, looked up again at each use as the original does. */
#define SELECTED_CAR(player) (g_aCarModels[g_aVehicles[player].selectedCar])

void DrawProgressBar(s32 x, s32 y, s32 width, s32 height, f32 fraction) {
    s32 right, bottom;

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, 0, 0);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE);
    /* The colours fill only the second of the two pixels of each word. */
    gDPSetFillColor(gDisplayListPtr++, BAR_FRAME_COLOR);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, BAR_BACK_COLOR);
    x += 3;
    width -= 6;
    bottom = y + height - 3;
    y += 3;
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, bottom);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, BAR_FILL_COLOR);
    right = x + (s32)((f32)width * fraction);
    gDPFillRectangle(gDisplayListPtr++, x, y, right, bottom);
}

s32 IsCarUnlocked(s32 car) {
    return (!g_dwIsPaintMode || car < PAINT_SHOP_CARS) &&
           (g_pActiveProfile->unlocks & (1 << car)) != 0 && IsVehicleModelValid(car) != 0;
}

/** Light a model on a stage, from an eye, with the four lights of the stage and its ambient light.
 */
static inline void LightStageModel(f32 eyeX, f32 eyeY, f32 eyeZ, f32 upY, f32 upZ) {
    LookAt *lookAt = AllocLightBlock();

    guLookAtReflectF(g_afScratchMatrix, lookAt, eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, upY, upZ);
    gSPLookAt(gDisplayListPtr++, lookAt);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_4);
    gSPLight(gDisplayListPtr++, &g_CameraViewLight1, 1);
    gSPLight(gDisplayListPtr++, &g_CameraViewLight2, 2);
    gSPLight(gDisplayListPtr++, &g_CameraViewLight3, 3);
    gSPLight(gDisplayListPtr++, &g_CameraViewLight4, 4);
    gSPLight(gDisplayListPtr++, &g_CameraViewAmbientLight, 5);
    /* The block is taken and left unused. */
    AllocDisplayListBlock();
}

/** Draw a model on a stage, turned and placed. */
static inline void DrawStageModel(Model *model, f32 degrees, f32 x, f32 y) {
    guScaleF(g_afMvpMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
    guRotateF(g_afScratchMatrix, degrees, 0.0f, 0.0f, 1.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, x, y, 0.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(model, g_afMvpMatrix);
}

/** Draw the vehicle of the camera with the car of its slot, and its shadow. */
static inline void DrawStageVehicle(void) {
    RenderTrackShadow(g_pActiveCameraVehicle);
    CheckVehicleVisibility(g_pActiveCameraVehicle);
    RenderVehicleModel(g_pActiveCameraVehicle, 0);
}

void RenderCameraView(Model *model,
                      Model *secondModel,
                      s32 vehicleIndex,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      f32 slide,
                      s32 hideTachometer,
                      s32 slot,
                      s32 secondSlot,
                      f32 spread,
                      f32 angle) {
    VehicleModelLoad *load, *secondLoad;
    CameraFrame *camera;
    f32 offset, otherOffset;

    g_pActiveCameraVehicle = &g_aVehicles[vehicleIndex];
    g_pActiveCameraVehicle->frame.forward.x = cosf(angle);
    g_pActiveCameraVehicle->frame.forward.y = sinf(angle);
    g_pActiveCameraVehicle->frame.forward.z = 0.0f;
    Vec3fCross(&g_pActiveCameraVehicle->frame.side,
               &g_pActiveCameraVehicle->frame.up,
               &g_pActiveCameraVehicle->frame.forward);
    camera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    g_pActiveCameraVehicle->activeCamera = camera;
    g_pActiveCamera = camera;
    /* The heading is taken twice. */
    g_pActiveCameraVehicle->cameraHeading = Atan2f(camera->forward.x, camera->forward.y);
    g_pActiveCameraVehicle->cameraHeading =
        Atan2f(g_pActiveCamera->forward.x, g_pActiveCamera->forward.y);
    SetupCameraMatrix(g_pActiveCamera,
                      g_fCameraFovRadians,
                      g_fCameraProjectionScale * 0.2f,
                      (f32)width,
                      (f32)height);
    SetViewport(x, y, width, height, TRUE);
    MarkVisibleCell(0, 0);

    /* The cars slide apart by the square of the slide. */
    if (slide < 0.0) {
        offset = slide * slide * -spread;
    } else {
        offset = slide * slide * spread;
    }
    g_pActiveCameraVehicle->frame.position.x = offset;
    otherOffset = -offset;
    g_pActiveCameraVehicle->frame.position.y = otherOffset;
    g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    if (model == NULL) {
        TransformWheelPositions(vehicleIndex);
    }
    load = &g_aVehicleModelSlots[slot];
    PollVehicleModelDma(load);
    RenderSpeedometerNeedle(g_pActiveCameraVehicle);
    if (!hideTachometer && !g_dwIsReviewMode) {
        EmitTachometerDisplayList(g_pActiveCameraVehicle);
    }
    if (model != NULL) {
        LightStageModel(11.0f, 11.0f, 1.0f, 0.0f, 1.0f);
        DrawStageModel(model, angle * RAD_TO_DEG, offset, otherOffset);
    } else if (IsVehicleModelLoaded(load)) {
        SetVehicleModelSlot(g_pActiveCameraVehicle, slot);
        DrawStageVehicle();
    }

    /* The second car comes in from the far side as the first slides out. */
    secondLoad = &g_aVehicleModelSlots[secondSlot];
    PollVehicleModelDma(secondLoad);
    if (offset == 0.0f) {
        return;
    }
    if (secondModel != NULL) {
        angle *= RAD_TO_DEG;
        LightStageModel(0.0f, -1.0f, 15.0f, 1.0f, 0.0f);
        guScaleF(g_afMvpMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
        guRotateF(g_afScratchMatrix, angle, 0.0f, 0.0f, 1.0f);
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
        /* Each side of the stage has its own call. */
        if (0.0f < offset) {
            guTranslateF(g_afScratchMatrix, offset - spread, -(offset - spread), 0.0f);
        } else {
            guTranslateF(g_afScratchMatrix, offset + spread, -(offset + spread), 0.0f);
        }
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
        DrawModelList(secondModel, g_afMvpMatrix);
        return;
    }
    if (!IsVehicleModelLoaded(secondLoad) && PollVehicleModelDma(secondLoad)) {
        return;
    }
    SetVehicleModelSlot(g_pActiveCameraVehicle, secondSlot);
    /* Each side of the stage has its own stores. */
    if (0.0f < offset) {
        g_pActiveCameraVehicle->frame.position.x = offset - spread;
        g_pActiveCameraVehicle->frame.position.y = -(offset - spread);
        g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    } else {
        g_pActiveCameraVehicle->frame.position.x = offset + spread;
        g_pActiveCameraVehicle->frame.position.y = -(offset + spread);
        g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    }
    TransformWheelPositions(vehicleIndex);
    DrawStageVehicle();
    if (IsVehicleModelLoaded(load)) {
        SetVehicleModelSlot(g_pActiveCameraVehicle, slot);
    }
}

void FormatLapTime(char *out, f32 seconds) {
    s32 hundredths = (s32)(seconds * 100.0f);
    s32 whole = hundredths / 100;

    sprintf(out, "%d'%02d\"%02d", whole / 60, whole - whole / 60 * 60, hundredths - whole * 100);
}

/** Play a sound of the screen on a free channel. */
static inline void PlayCarSelectSound(s8 *sample, u32 length) {
    s16 channel = FindFreeSoundChannel();

    if (channel != -1) {
        InitSoundChannel(channel, sample, length, 0);
    }
}

/** Load a sound of the screen and silence its end. */
static inline s8 *LoadCarSelectSound(u32 rom, u32 *size) {
    s8 *buffer;

    *size = DmaReadAssetSize(rom);
    buffer = AllocHeapAligned(*size + SOUND_TAIL_BYTES);
    DecompressFromRom(buffer, rom, NULL);
    return buffer;
}

/** Set a vehicle on a stage, facing along x. */
static inline void PlaceOnStage(Vehicle *vehicle) {
    vehicle->frame.forward.x = 1.0f;
    vehicle->frame.forward.y = 0.0f;
    vehicle->frame.forward.z = 0.0f;
    vehicle->frame.side.x = 0.0f;
    vehicle->frame.side.y = 1.0f;
    vehicle->frame.side.z = 0.0f;
    vehicle->frame.up.x = 0.0f;
    vehicle->frame.up.y = 0.0f;
    vehicle->frame.up.z = 1.0f;
    vehicle->frame.position.x = 0.0f;
    vehicle->frame.position.y = 0.0f;
    vehicle->frame.position.z = STAGE_HEIGHT;
}

/** Set up the screen: the players' cars, the stages, the options and the sounds. */
static inline void StartCarSelect(void) {
    PlayerProfile *profile;
    CameraFrame *camera;
    Vehicle *vehicle;
    s32 i, car;

    g_dwCarSelectStartTime = g_dwElapsedTimeMs;
    SetFadeTarget(1.0f, SCREEN_FADE_RATE);
    FadeSfxTo(1.0f, SCREEN_FADE_RATE);
    FadeMusicTo(1.0f, SCREEN_FADE_RATE);
    profile = g_aVehicles[0].profile;
    g_dwResultsSeason = profile->season;
    g_dwResultsRound = profile->round;
    g_dwCarSelectWeather = g_dwCurrentWeather;
    g_bSeasonResultsShown = FALSE;
    g_dwVehicleViewModeIndex = 0;
    g_afCarSelectAngle[0] = 0.0f;
    g_afCarSelectAngle[1] = 1.0f;
    g_dwResultsChampionshipFlags = profile->championshipFlags;
    g_dwResultsTrack = g_dwSelectedTrackId;
    for (i = 0, vehicle = g_aVehicles; i < (s32)g_dwNumLocalPlayers; i++, vehicle++) {
        if (!g_dwIsReviewMode) {
            InitVehicleColor(vehicle);
            vehicle->driver = UpdatePhysicsStep;
        }
        g_adwCarSelectModelSlots[i] = i;
        g_adwCarSelectState[i] = CAR_SELECT_CAR;
        vehicle->groundInstanceCount = 0;
        vehicle->nearbyInstanceCount = 0;
        g_afCarSelectSlide[i] = 0.0f;
        g_dwPakDialogActive = FALSE;
    }
    /* The vehicles of no player are the computer's. */
    if (!g_dwIsReviewMode && i < MAX_VEHICLES) {
        for (; i < MAX_VEHICLES; i++) {
            InitVehicleColor(&g_aVehicles[i]);
            g_aVehicles[i].driver = DriveAiVehicle;
        }
    }
    g_dwNumActiveVehicles = 2;
    if (!g_dwIsReviewMode) {
        for (i = 0, vehicle = g_aVehicles; i < (s32)g_dwNumLocalPlayers; i++, vehicle++) {
            ChooseVehicleCar(vehicle);
            car = vehicle->carModel;
            vehicle->selectedCar = car;
            while (!IsCarUnlocked(car)) {
                car = (car + 1) % g_dwCarCount;
            }
            vehicle->carModel = car;
            vehicle->selectedCar = car;
            InstallVehicleModel(vehicle, i, car);
        }
    }
    PlaceOnStage(&g_aVehicles[0]);
    PlaceOnStage(&g_aVehicles[1]);

    /* The trackside camera looks at the stage from above a corner, farther in two views. */
    camera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE];
    if (g_dwNumLocalPlayers == 2 && !g_dwIsReviewMode) {
        camera->position.x = 15.0f;
        camera->position.y = 15.0f;
        camera->position.z = 7.5f;
    } else {
        camera->position.x = 10.0f;
        camera->position.y = 10.0f;
        camera->position.z = 5.0f;
    }
    Vec3fScale(&camera->forward, &camera->position, -1.0f);
    Vec3fNormalize(&camera->forward);
    camera->up.x = 0.0f;
    camera->up.y = 0.0f;
    camera->up.z = 1.0f;
    Vec3fCross(&camera->side, &camera->up, &camera->forward);
    Vec3fCross(&camera->up, &camera->forward, &camera->side);
    memcpy(&g_aVehicles[1].cameras[VEHICLE_CAMERA_TRACKSIDE], camera, sizeof(CameraFrame));

    ResetHeap();
    LoadMenuItemModels(g_apHandlingOptions, &g_dwHandlingOptionCount);
    LoadMenuItemModels(g_apTransmissionOptions, &g_dwTransmissionOptionCount);
    LoadMenuItemModels(g_apTiresOptions, &g_dwTiresOptionCount);
    LoadMenuItemModels(g_apSuspensionOptions, &g_dwSuspensionOptionCount);
    LoadMenuItemModels(g_apDecalOptions, &g_dwDecalListCount);
    LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_TitleBarTextureDesc, AllocHeapAligned);
    g_pScrollSoundBuf = LoadCarSelectSound(ROM_SCROLL_SOUND, &g_dwScrollSoundSize);
    g_pSelectSoundBuf = LoadCarSelectSound(ROM_SELECT_SOUND, &g_dwSelectSoundSize);
    for (i = 0; i < SOUND_TAIL_BYTES; i++) {
        g_pScrollSoundBuf[g_dwScrollSoundSize + i] = 0;
        g_pSelectSoundBuf[g_dwSelectSoundSize + i] = 0;
    }
    AllocateEditorBuffers();
    g_dwTitleRedrawFrames = 2;
    g_bCarSelectInitialized = TRUE;
    g_aapCarSelectOptions[1] = g_apHandlingOptions;
    g_aapCarSelectOptions[0] = g_apHandlingOptions;
    g_anCarSelectCursor[1] = 0;
    g_anCarSelectCursor[0] = 0;
    g_dwResultsPage = RESULTS_PAGE_PLAYER_1;
}

/** The models of the option a player chose and of the one the cursor left, or NULL for cars. */
static inline Model *OptionModel(s32 player, s32 option) {
    if (g_adwCarSelectState[player] == CAR_SELECT_CAR ||
        g_adwCarSelectState[player] == CAR_SELECT_READY) {
        return NULL;
    }
    return g_aapCarSelectOptions[player][option]->model;
}

/** Draw the stage of a player in a rectangle of the screen. */
static inline void
DrawPlayerStage(s32 player, s32 vehicleIndex, s32 slot, s32 y, s32 height, f32 spread) {
    Model *first, *second;

    first = OptionModel(player, g_anCarSelectCursor[player]);
    second = OptionModel(player, g_anCarSelectPrevCursor[player]);
    RenderCameraView(first,
                     second,
                     vehicleIndex,
                     0,
                     y,
                     g_dwScreenWidth,
                     height,
                     g_afCarSelectSlide[player],
                     g_adwCarSelectState[player] == CAR_SELECT_READY,
                     slot,
                     slot ^ 2,
                     spread,
                     g_afCarSelectAngle[player]);
}

/** Draw the title of the screen. */
static inline void DrawCarSelectTitle(char *text) {
    const char *prefix;
    s32 track;

    DrawTexturedRect(&g_TitleBarTextureDesc,
                     22,
                     9,
                     280,
                     42,
                     0,
                     0,
                     0,
                     0xFF,
                     0xFF,
                     0xFF,
                     0xFF,
                     0xFF,
                     TEXRECT_BLEND_TEXTURE_2);
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    if (g_dwIsReviewMode) {
        if (g_dwResultsPage == RESULTS_PAGE_PLAYER_1 || g_dwResultsPage == RESULTS_PAGE_PLAYER_2) {
            /* The track and the weather of the race, its name marked when it is mirrored. */
            prefix = g_dwResultsTrack < MIRRORED_TRACKS ? "" : "M-";
            track = g_dwResultsTrack < MIRRORED_TRACKS ? g_dwResultsTrack :
                                                         g_dwResultsTrack - MIRRORED_TRACKS;
            sprintf(text,
                    "%%ry%s%s/%s",
                    prefix,
                    g_apTrackMenuItems[track]->name,
                    g_apWeatherMenuItems[g_dwCarSelectWeather]->name);
        } else {
            sprintf(text, "%%ry%s", g_SeasonTable[g_dwResultsSeason].name);
        }
        DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    } else if (g_dwIsPaintMode) {
        DrawText("%ryPaint car select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    } else {
        DrawText("%ryCar Select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    }
}

/** Draw the place and the times of the race of a player. */
static inline void DrawRaceResults(s32 player, char *time) {
    Vehicle *vehicle = &g_aVehicles[player];
    PlayerProfile *profile;
    s32 place, points, lap, y;

    if ((s32)g_dwNumLocalPlayers >= 2) {
        if (vehicle->lap < g_dwTotalLaps) {
            sprintf(g_szResultsBuf, "%%ryP%d: Did not finish", player + 1);
        } else {
            sprintf(g_szResultsBuf,
                    "%%ryP%d: %s Place",
                    player + 1,
                    g_apszPlaceNames[vehicle->racePosition]);
        }
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        profile = vehicle->profile;
        place = profile->seasonPlaces[g_dwResultsSeason][g_dwResultsRound];
        points = g_abRacePositionPoints[place];
        sprintf(g_szResultsBuf,
                "%%ry%s Place - %d Point%s",
                g_apszPlaceNames[place],
                points,
                points == 1 ? "" : "s");
    } else if (vehicle->lap < g_dwTotalLaps) {
        sprintf(g_szResultsBuf, "%%ryDid not finish");
    } else {
        sprintf(g_szResultsBuf, "%%ry%s Place", g_apszPlaceNames[vehicle->racePosition]);
    }
    DrawText(g_szResultsBuf, g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(10);

    /* A line for each lap, then the time of the race. */
    for (lap = 0, y = 79; lap <= g_dwTotalLaps; lap++, y += 10) {
        SetTextAlignLeft();
        if (lap == g_dwTotalLaps) {
            y += 2;
            if (vehicle->totalTime == g_pActiveProfile->raceRecords[g_dwResultsTrack]) {
                DrawText("  %yyRECORD TIME!", g_dwScreenWidth * 11 / 16, y);
            }
            sprintf(g_szResultsBuf, "Race time:");
            FormatLapTime(time, vehicle->totalTime);
        } else {
            sprintf(g_szResultsBuf, "Lap %d time:", lap + 1);
            if (lap < vehicle->lap) {
                FormatLapTime(time, g_aVehicles[player].lapTimes[lap]);
            } else {
                sprintf(time, "--    ");
            }
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 5 / 16, y);
        if (lap < vehicle->lap && lap == vehicle->bestLap) {
            if (vehicle->bestLapTime == g_pActiveProfile->lapRecords[g_dwResultsTrack]) {
                DrawText("  %yyRECORD LAP!", g_dwScreenWidth * 11 / 16, y);
            } else {
                DrawText("  %yyBEST LAP!", g_dwScreenWidth * 11 / 16, y);
            }
        }
        SetTextAlignRight();
        DrawText(time, g_dwScreenWidth * 11 / 16, y);
    }
}

/** Draw the races of the season, their times, places and points, and the total. */
static inline void DrawSeasonResults(void) {
    Season *season = &g_SeasonTable[g_dwResultsSeason];
    const char *place;
    s32 race, track, shift, total, y;
    u8 position;

    DrawText("%rySeason Results", g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(10);

    /* The columns move right when a name is of the longer, mirrored tracks. */
    shift = 0;
    for (race = 0; race < season->raceCount; race++) {
        if ((season->races[race][0] < MIRRORED_TRACKS) ==
            ((g_dwResultsChampionshipFlags & 1) != 0)) {
            shift = g_dwScreenWidth / 16;
            break;
        }
    }

    total = 0;
    for (race = 0, y = 79; race < season->raceCount; race++, y += 10) {
        track = season->races[race][0];
        if (g_dwResultsChampionshipFlags & 1) {
            track = track < MIRRORED_TRACKS ? track + MIRRORED_TRACKS : track - MIRRORED_TRACKS;
        }
        SetTextAlignLeft();
        sprintf(g_szResultsBuf,
                "%s/%s",
                g_apTrackMenuItems[track]->name,
                g_apWeatherMenuItems[season->races[race][1]]->name);
        DrawText(g_szResultsBuf, g_dwScreenWidth * 3 / 16 - shift, y);
        SetTextAlignCenter();
        if (race <= g_dwResultsRound) {
            FormatLapTime(g_szResultsBuf, g_pActiveProfile->seasonTimes[g_dwResultsSeason][race]);
        } else {
            sprintf(g_szResultsBuf, "--");
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 17 / 32 + shift, y);
        if (race <= g_dwResultsRound) {
            place = g_apszPlaceNames[g_pActiveProfile->seasonPlaces[g_dwResultsSeason][race]];
        } else {
            place = "--";
        }
        DrawText(place, g_dwScreenWidth * 21 / 32 + shift, y);
        SetTextAlignRight();
        /* A race not yet run shows the dashes of its time again. */
        if (race <= g_dwResultsRound) {
            position = g_pActiveProfile->seasonPlaces[g_dwResultsSeason][race];
            sprintf(g_szResultsBuf, "%d PTS", g_abRacePositionPoints[position]);
            total +=
                g_abRacePositionPoints[g_pActiveProfile->seasonPlaces[g_dwResultsSeason][race]];
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 13 / 16 + shift, y);
    }
    y += 2;
    sprintf(g_szResultsBuf, "TOTAL:    %d PTS ", total);
    DrawText(g_szResultsBuf, g_dwScreenWidth * 13 / 16 + shift, y);
}

/** Draw the lines of the message of the result of the season. */
static inline void DrawSeasonMessage(void) {
    s32 line, y;

    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(12);
    y = 98 - g_dwSeasonResultMsgLineCount * 120 / 20;
    SetTextAlignCenter();
    for (line = 0; line < g_dwSeasonResultMsgLineCount; line++, y += 12) {
        DrawText(g_apszSeasonResultMsgLines[line], g_dwScreenWidth / 2, y);
    }
}

/** Write what a player is choosing: the name of the car, ready, or the option and its name. */
static inline void FormatPlayerChoice(
    char *text, s32 player, const char *ready, const char *carFormat, const char *optionFormat) {
    s32 state = g_adwCarSelectState[player];

    if (state == CAR_SELECT_CAR) {
        sprintf(text, carFormat, g_aCarModels[g_aVehicles[player].selectedCar].name);
    } else if (state == CAR_SELECT_READY) {
        sprintf(text, ready);
    } else {
        sprintf(text,
                optionFormat,
                g_apszCarSelectCategory[player],
                g_aapCarSelectOptions[player][g_anCarSelectCursor[player]]->name);
    }
}

/** Draw the advice about the rumble pak a player sees while choosing decals other than loading. */
static inline void DrawRumbleNote(s32 player, s32 y) {
    s32 x;

    if (g_adwCarSelectState[player] != CAR_SELECT_DECALS ||
        g_anCarSelectCursor[player] == DECALS_LOAD || g_afCarSelectSlide[player] != 0.0f) {
        return;
    }
    x = g_dwScreenWidth * 11 / 16;
    DrawText("%wwIf you wish to use", x, y + 19);
    DrawText("%wwthe Rumble Pak, you", x, y + 28);
    DrawText("%wwshould ensure that it", x, y + 37);
    DrawText("%wwis plugged in before", x, y + 45);
    DrawText("%wwpressing A.", x, y + 54);
}

/** Draw the engine, gearbox, acceleration and top speed of the car of a player. */
static inline void DrawCarStats(char *text, s32 player, s32 y, s32 barY) {
    SetTextAlignRight();
    DrawText("%wwAcceleration", g_dwScreenWidth / 4 - 5, y + 2);
    DrawText("%wwMax Speed", g_dwScreenWidth / 4 - 5, y - 6);
    SetTextAlignCenter();
    DrawText(
        g_apszEngineLayouts[SELECTED_CAR(player).engineLayout], g_dwScreenWidth * 3 / 4, y + 2);
    sprintf(text,
            "%%ww%d-Speed %s",
            SELECTED_CAR(player).gearCount,
            g_apszDriveTypes[SELECTED_CAR(player).driveType]);
    DrawText(text, g_dwScreenWidth * 3 / 4, y - 6);
    DrawProgressBar(g_dwScreenWidth / 2,
                    barY - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).acceleration / FULL_ACCELERATION));
    DrawProgressBar(g_dwScreenWidth / 2,
                    barY - 6,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).maxSpeed / FULL_SPEED));
}

/** Draw what the two players are choosing, their advice, and the stats of their cars. */
static inline void DrawTwoPlayerChoices(char *text) {
    SetTextSize(17);
    FormatPlayerChoice(text, 0, "%%ryREADY!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight / 4);
    FormatPlayerChoice(text, 1, "%%ryREADY!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 3 / 5);
    SetTextSize(8);
    SetTextAlignLeft();
    DrawRumbleNote(0, g_dwScreenHeight / 4);
    DrawRumbleNote(1, g_dwScreenHeight * 3 / 5);
    SetTextAlignRight();
    if (g_adwCarSelectState[0] == CAR_SELECT_CAR) {
        DrawCarStats(text, 0, g_dwScreenHeight / 4, g_dwScreenHeight / 2);
    }
    if (g_adwCarSelectState[1] == CAR_SELECT_CAR) {
        DrawCarStats(text, 1, g_dwScreenHeight * 3 / 5, g_dwScreenHeight * 3 * 2 / 5);
    }
}

/** Draw what the player is choosing, the advice, and the stats of the car. */
static inline void DrawOnePlayerChoices(char *text) {
    s32 player = 0;

    SetTextSize(20);
    if (g_dwIsPaintMode) {
        sprintf(text, "%%ry%%i%s", g_aCarModels[g_aVehicles[0].selectedCar].name);
        DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 22 / 64);
        return;
    }
    FormatPlayerChoice(text, 0, "%%ryLET'S GO!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);
    SetTextSize(8);
    SetTextAlignCenter();
    if (g_adwCarSelectState[0] == CAR_SELECT_DECALS && g_anCarSelectCursor[0] != DECALS_LOAD &&
        g_afCarSelectSlide[0] == 0.0f) {
        DrawText("%wwIf you wish to use the Rumble Pak, you should",
                 g_dwScreenWidth / 2,
                 g_dwScreenHeight * 3 / 8 - 6);
        DrawText("%wwensure that it is plugged in before pressing A.",
                 g_dwScreenWidth / 2,
                 g_dwScreenHeight * 3 / 8 + 2);
    }
    if (g_adwCarSelectState[0] != CAR_SELECT_CAR) {
        return;
    }
    SetTextAlignRight();
    SetTextSize(8);
    DrawText("%wwMax Speed", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 - 6);
    DrawText("%wwAcceleration", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 + 2);
    SetTextAlignCenter();
    DrawText(g_apszEngineLayouts[SELECTED_CAR(player).engineLayout],
             g_dwScreenWidth * 11 / 16,
             g_dwScreenHeight * 3 / 8 - 6);
    sprintf(text,
            "%%ww%d-Speed %s",
            SELECTED_CAR(player).gearCount,
            g_apszDriveTypes[SELECTED_CAR(player).driveType]);
    DrawText(text, g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 8 + 2);
    DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10,
                    g_dwScreenHeight * 3 / 4 - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).acceleration / FULL_ACCELERATION));
    DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10,
                    g_dwScreenHeight * 3 / 4 - 6,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).maxSpeed / FULL_SPEED));
}

/** Run the dialog of the controller pak that loads the decals of a player. */
static inline void RunPakDialog(void) {
    s32 result = g_dwPakDialogResult;
    s32 player;
    s32 previous;

    /* The original stores the result back on every path but that of a cancel, changed or not. */
    if (result == 0) {
        if (!g_bPakDialogDone) {
            result = ProcessDecalSaveLoad(&g_aVehicleModels[g_dwPakDialogPlayerIdx],
                                          (u8)g_dwPakDialogPlayerIdx,
                                          9,
                                          1,
                                          &g_bPakDialogDone);
        }
        g_dwPakDialogResult = result;
    } else if (result == PAK_DIALOG_CANCELLED) {
        g_bPakDialogDone = TRUE;
    } else {
        g_dwPakDialogResult = result;
        DrawPakErrorMessage(result, 9, 1);
        if (g_aVehicles[g_dwPakDialogPlayerIdx].input->buttons & (INPUT_A | INPUT_B)) {
            TransferFlags(g_aVehicles[g_dwPakDialogPlayerIdx].input, INPUT_A | INPUT_B);
            g_bPakDialogDone = TRUE;
        }
    }
    player = g_dwPakDialogPlayerIdx;
    if (!g_bPakDialogDone) {
        return;
    }
    result = g_dwPakDialogResult;
    if (result != 0) {
        /* The custom decals are out of reach without loaded ones. */
        g_anCarSelectOptionCount[player] = g_dwDecalListCount - 1;
        g_dwPakDialogResult = 0;
    } else {
        /* Loaded decals add the custom choice, and the cursor moves to it. The original clears
         * the result by storing the zero it read. */
        g_dwPakDialogResult = result;
        g_anCarSelectOptionCount[player] = g_dwDecalListCount;
        previous = g_anCarSelectCursor[player];
        g_anCarSelectPrevCursor[player] = previous;
        g_anCarSelectCursor[player] = (previous + 1) % g_dwDecalListCount;
        g_afCarSelectSlide[player] = 1.0f;
        g_dwPakDialogPlayerIdx = player;
        ExtractVehicleBaseColor(&g_aVehicles[player], &g_aVehicleModels[player]);
    }
}

/** Leave the screen once it has faded out, to the next screen or back. */
static inline void LeaveCarSelect(void) {
    s32 i;

    g_bCarSelectInitialized = FALSE;
    ClearFramebuffersForWideMode();
    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        g_aSoundEffectChannels[i].volume = 0;
        g_aSoundEffectChannels[i].mixVolume = 0;
        g_aSoundEffectChannels[i].step = 0;
        g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
    }
    if (g_bCarSelectDone) {
        if (g_dwIsReviewMode) {
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                /* The next race of the season, or after its last race, the end of the season. */
                g_dwNumLocalPlayers = 1;
                g_dwContinueFromResults = g_pActiveProfile->round != 0;
                if (g_dwResultsSeason == g_pActiveProfile->season) {
                    SetTrackFromSeason();
                    g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_SEASON;
                    SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
                } else {
                    ClearFramebuffersLowRes();
                    g_dwGameMode = GAME_MODE_AFTER_SEASON;
                    SetCurrentScreenCallback(ProcessRacingScreen);
                }
            } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK && g_dwSaveAvailable) {
                g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_TIME_ATTACK;
                SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
            } else {
                SetCurrentScreenCallback(ProcessMainMenu);
            }
        } else if (g_dwIsPaintMode) {
            SetCurrentScreenCallback(ProcessPaintShopScreen);
        } else {
            ClearFramebuffersLowRes();
            SetCurrentScreenCallback(ProcessRacingScreen);
        }
    } else if (g_dwIsReviewMode) {
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && !g_bSeasonResultsShown) {
            ProcessSeasonResults();
        }
        SetCurrentScreenCallback(ProcessMainMenu);
    } else if (g_dwIsPaintMode) {
        SetCurrentScreenCallback(ProcessMainMenu);
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        g_dwContinueFromResults = TRUE;
        SetCurrentScreenCallback(ProcessTrackMenu);
    } else {
        SetCurrentScreenCallback(ProcessWeatherSelect);
    }
}

/** Raise or lower a part of the colour of the paint by a step, within a byte. */
static inline void StepPaintPart(u8 *part, s32 up) {
    if (up) {
        *part = *part < 0x100 - PAINT_STEP ? *part + PAINT_STEP : 0xFF;
    } else {
        *part = *part < PAINT_STEP ? 0 : *part - PAINT_STEP;
    }
}

/** Change the colour of the paint of a vehicle with up or down and a C button for each part. */
static inline void AdjustPaint(Vehicle *vehicle, u32 direction) {
    s32 up = direction == INPUT_UP;

    if (vehicle->input->buttons & INPUT_C_LEFT) {
        StepPaintPart(&vehicle->color[0], up);
    }
    if (vehicle->input->buttons & INPUT_C_UP) {
        StepPaintPart(&vehicle->color[1], up);
    }
    if (vehicle->input->buttons & INPUT_C_RIGHT) {
        StepPaintPart(&vehicle->color[2], up);
    }
    SetMaterialColor(
        vehicle->model, vehicle->color[0] >> 3, vehicle->color[1] >> 3, vehicle->color[2] >> 3);
}

/** Make the options of a step those a player chooses from, with its name. */
static inline void
SetCarSelectOptions(s32 player, MenuItem **options, s32 count, const char *name, s32 cursor) {
    g_anCarSelectCursor[player] = cursor;
    g_aapCarSelectOptions[player] = options;
    g_anCarSelectOptionCount[player] = count;
    g_apszCarSelectCategory[player] = name;
}

/** Handle the buttons of a player on the screen. */
static inline void HandleCarSelectInput(s32 player) {
    Vehicle *vehicle = &g_aVehicles[player];
    s32 previous, state, page, i;
    s32 playSound = TRUE;
    s32 *slot = &g_adwCarSelectModelSlots[player];

    if (g_dwIsReviewMode) {
        vehicle->input->buttons &= ~(INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT);
    } else {
        UpdateStickRepeat(vehicle->input);
    }
    state = g_adwCarSelectState[player];
    if (state == CAR_SELECT_READY) {
        if (vehicle->input->buttons & INPUT_B) {
            g_adwCarSelectState[player] = state - 1;
            TransferFlags(vehicle->input, INPUT_B);
        }
        return;
    }
    if (g_afCarSelectSlide[player] != 0.0f) {
        return;
    }

    /* Left and right turn to another car, which loads into the other slot. */
    if (state == CAR_SELECT_CAR) {
        previous = vehicle->selectedCar;
        if (vehicle->input->buttons & INPUT_LEFT) {
            do {
                vehicle->selectedCar = (vehicle->selectedCar + g_dwCarCount - 1) % g_dwCarCount;
            } while (!IsCarUnlocked(vehicle->selectedCar));
        }
        if (previous != vehicle->selectedCar) {
            g_afCarSelectSlide[player] = 1.0f;
        } else if (vehicle->input->buttons & INPUT_RIGHT) {
            do {
                vehicle->selectedCar = (vehicle->selectedCar + 1) % g_dwCarCount;
            } while (!IsCarUnlocked(vehicle->selectedCar));
            if (previous != vehicle->selectedCar) {
                g_afCarSelectSlide[player] = -1.0f;
            }
        }
        if (previous != vehicle->selectedCar) {
            PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
            *slot ^= 2;
            LoadVehicleModel(&g_aVehicleModelSlots[*slot], *slot, vehicle->selectedCar, player);
        }
    }

    /* While choosing the car or its decals, C-down switches the look, and up and down paint it. */
    state = g_adwCarSelectState[player];
    if (state == CAR_SELECT_CAR || state == CAR_SELECT_DECALS) {
        if (vehicle->input->buttons & INPUT_C_DOWN) {
            TransferFlags(vehicle->input, INPUT_C_DOWN);
            if (vehicle->profile->unlocks & UNLOCK_OTHER_LOOK) {
                vehicle->otherLook ^= 1;
            }
        }
        if (vehicle->input->buttons & INPUT_UP) {
            AdjustPaint(vehicle, INPUT_UP);
        }
        if (vehicle->input->buttons & INPUT_DOWN) {
            AdjustPaint(vehicle, INPUT_DOWN);
        }
    }

    /* Left and right move through the options of the other steps. */
    if (g_adwCarSelectState[player] != CAR_SELECT_CAR) {
        if (vehicle->input->buttons & INPUT_LEFT) {
            previous = g_anCarSelectCursor[player];
            g_anCarSelectPrevCursor[player] = previous;
            g_anCarSelectCursor[player] = (previous + g_anCarSelectOptionCount[player] - 1) %
                                          g_anCarSelectOptionCount[player];
            g_afCarSelectSlide[player] = 1.0f;
            PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
        } else if (vehicle->input->buttons & INPUT_RIGHT) {
            previous = g_anCarSelectCursor[player];
            g_anCarSelectPrevCursor[player] = previous;
            g_anCarSelectCursor[player] = (previous + 1) % g_anCarSelectOptionCount[player];
            g_afCarSelectSlide[player] = -1.0f;
            PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
        }
    }

    if (vehicle->input->buttons & INPUT_B) {
        /* B leaves the screen from the car, and steps back from the options. */
        TransferFlags(vehicle->input, INPUT_B);
        state = g_adwCarSelectState[player];
        if (state == CAR_SELECT_CAR) {
            g_bCarSelectDone = FALSE;
            FadeSfxTo(0.0f, SCREEN_FADE_RATE);
            SetFadeTarget(0.0f, SCREEN_FADE_RATE);
        } else {
            g_adwCarSelectState[player] = --state;
            switch (state) {
            case CAR_SELECT_HANDLING:
                SetCarSelectOptions(player,
                                    g_apHandlingOptions,
                                    g_dwHandlingOptionCount,
                                    "Handling",
                                    vehicle->profile->handling);
                break;
            case CAR_SELECT_TRANSMISSION:
                SetCarSelectOptions(player,
                                    g_apTransmissionOptions,
                                    g_dwTransmissionOptionCount,
                                    "Transmission",
                                    vehicle->profile->transmission);
                break;
            case CAR_SELECT_TIRES:
                SetCarSelectOptions(player,
                                    g_apTiresOptions,
                                    g_dwTiresOptionCount,
                                    "Tires",
                                    vehicle->profile->tires);
                break;
            case CAR_SELECT_SUSPENSION:
                SetCarSelectOptions(player,
                                    g_apSuspensionOptions,
                                    g_dwSuspensionOptionCount,
                                    "Suspension",
                                    vehicle->profile->suspension);
                break;
            case CAR_SELECT_DECALS:
                /* The options stay those of the suspension. */
                g_anCarSelectCursor[player] = 0;
                g_apszCarSelectCategory[player] = "Decals";
                break;
            }
            g_anCarSelectPrevCursor[player] = g_anCarSelectCursor[player];
        }
    } else if (vehicle->input->buttons & CHOOSE_BUTTONS) {
        TransferFlags(vehicle->input, CHOOSE_BUTTONS);
        switch (g_adwCarSelectState[player]) {
        case CAR_SELECT_CAR:
            vehicle->carModel = vehicle->selectedCar;
            SetCarSelectOptions(player,
                                g_apHandlingOptions,
                                g_dwHandlingOptionCount,
                                "Handling",
                                vehicle->profile->handling);
            break;
        case CAR_SELECT_HANDLING:
            vehicle->profile->handling = g_anCarSelectCursor[player];
            SetCarSelectOptions(player,
                                g_apTransmissionOptions,
                                g_dwTransmissionOptionCount,
                                "Transmission",
                                vehicle->profile->transmission);
            break;
        case CAR_SELECT_TRANSMISSION:
            vehicle->profile->transmission = g_anCarSelectCursor[player];
            SetCarSelectOptions(
                player, g_apTiresOptions, g_dwTiresOptionCount, "Tires", vehicle->profile->tires);
            break;
        case CAR_SELECT_TIRES:
            vehicle->profile->tires = g_anCarSelectCursor[player];
            SetCarSelectOptions(player,
                                g_apSuspensionOptions,
                                g_dwSuspensionOptionCount,
                                "Suspension",
                                vehicle->profile->suspension);
            break;
        case CAR_SELECT_SUSPENSION:
            /* The decals show the car, loaded at once into both slots. */
            vehicle->profile->suspension = g_anCarSelectCursor[player];
            g_anCarSelectCursor[player] = 0;
            g_aapCarSelectOptions[player] = g_apDecalOptions;
            g_anCarSelectOptionCount[player] = vehicle->carModel < PAINT_SHOP_CARS ?
                                                   g_dwDecalListCount - 1 :
                                                   g_dwDecalListCount - 2;
            g_apszCarSelectCategory[player] = "Decals";
            *slot = player ^ 2;
            PlayCarSelectSound(g_pSelectSoundBuf, g_dwSelectSoundSize);
            playSound = FALSE;
            LoadVehicleModel(&g_aVehicleModelSlots[*slot], *slot, vehicle->carModel, player);
            while (PollVehicleModelDma(&g_aVehicleModelSlots[*slot])) {
            }
            LoadVehicleModel(
                &g_aVehicleModelSlots[*slot ^ 2], *slot ^ 2, vehicle->carModel, player);
            break;
        case CAR_SELECT_DECALS:
            if (g_anCarSelectCursor[player] == DECALS_LOAD) {
                g_dwPakDialogActive = TRUE;
                g_bPakDialogDone = FALSE;
                g_dwPakDialogResult = 0;
                g_dwPakDialogPlayerIdx = player;
                goto sound;
            }
            vehicle->customDecals = g_anCarSelectCursor[player] == DECALS_CUSTOM;
            break;
        }
        g_anCarSelectPrevCursor[player] = g_anCarSelectCursor[player];

        /* Start, the paint shop and the results skip the rest; the results turn a page. */
        if ((vehicle->input->buttons & INPUT_START) || g_dwIsPaintMode || g_dwIsReviewMode) {
            g_anCarSelectOptionCount[player] = g_dwDecalListCount - 1;
            g_anCarSelectCursor[player] = 0;
            g_anCarSelectPrevCursor[player] = 0;
            if (g_dwIsReviewMode) {
                previous = g_dwResultsPage;
                page = previous + 1;
                if (page == RESULTS_PAGE_PLAYER_2 && g_dwNumLocalPlayers != 2) {
                    page++;
                }
                if (page == RESULTS_PAGE_SEASON && g_dwGameMode != GAME_MODE_CHAMPIONSHIP) {
                    page = RESULTS_PAGE_END;
                }
                g_dwResultsPage = page;
                if (page == RESULTS_PAGE_MESSAGE) {
                    ProcessSeasonResults();
                    g_bSeasonResultsShown = TRUE;
                    if (g_dwSeasonResultMsgLineCount == 0) {
                        g_dwResultsPage++;
                    }
                }
                if (g_dwResultsPage == RESULTS_PAGE_END) {
                    /* Both are ready, and the last page stays in view as the screen fades. */
                    g_adwCarSelectState[1] = CAR_SELECT_READY;
                    g_adwCarSelectState[0] = CAR_SELECT_READY;
                    g_dwResultsPage = previous;
                }
            } else {
                g_adwCarSelectState[player] = CAR_SELECT_READY;
            }
        } else {
            g_adwCarSelectState[player]++;
        }

        /* Once every player is ready, the screen fades out. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            if (g_adwCarSelectState[i] != CAR_SELECT_READY) {
                goto sound;
            }
        }
        g_bCarSelectDone = TRUE;
        FadeSfxTo(0.0f, SCREEN_FADE_RATE);
        if (!g_dwIsPaintMode) {
            FadeMusicTo(0.0f, SCREEN_FADE_RATE);
        }
        SetFadeTarget(0.0f, SCREEN_FADE_RATE);
    } else {
        return;
    }
sound:
    if (playSound) {
        PlayCarSelectSound(g_pSelectSoundBuf, g_dwSelectSoundSize);
    }
}

void ProcessCarSelectScreen(void) {
    char text[96], time[32];
    s32 player, i;

    if (!g_bCarSelectInitialized) {
        StartCarSelect();
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    g_dwWeatherActive = FALSE;
    g_dwNightMode = FALSE;
    g_dwRainMode = FALSE;
    g_dwSnowMode = FALSE;
    SetupFog();
    SetupWeatherLights();
    AddTimingMarker(0, 0, 0, 0xC8, 0xFF);
    g_aViewportRects[0].vehicleIndex = 0;
    g_aViewportRects[1].vehicleIndex = 1;
    /* Set each frame, so that the title keeps being drawn. */
    g_dwTitleRedrawFrames = 2;
    ClearDepthBuffer();
    if (g_dwIsReviewMode) {
        DrawTexturedRect(&g_BgTextureDesc,
                         0,
                         0,
                         320,
                         240,
                         0,
                         0,
                         0,
                         0xFF,
                         0,
                         0x82,
                         0x8C,
                         0xFF,
                         TEXRECT_BLEND_SHADED_OPAQUE);
    } else {
        DrawTexturedRect(&g_BgTextureDesc,
                         0,
                         0,
                         320,
                         240,
                         0,
                         0,
                         0,
                         0xFF,
                         0x50,
                         0,
                         0,
                         0xFF,
                         TEXRECT_BLEND_SHADED_OPAQUE);
    }
    if (g_dwTitleRedrawFrames == 0 && IsFadeComplete()) {
        SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    }

    /* The stages turn, the second the other way, and the cars slide to rest. */
    g_afCarSelectAngle[0] += g_fFrameDeltaTime;
    g_afCarSelectAngle[1] += 6.283185482f - g_fFrameDeltaTime;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        while (6.283185482f < g_afCarSelectAngle[player]) {
            g_afCarSelectAngle[player] -= 6.283185482f;
        }
    }
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (g_afCarSelectSlide[player] < 0.0f) {
            g_afCarSelectSlide[player] += g_fFrameDeltaTime + g_fFrameDeltaTime;
            if (0.0f < g_afCarSelectSlide[player]) {
                g_afCarSelectSlide[player] = 0.0f;
            }
        } else if (0.0f < g_afCarSelectSlide[player]) {
            g_afCarSelectSlide[player] -= g_fFrameDeltaTime + g_fFrameDeltaTime;
            if (g_afCarSelectSlide[player] < 0.0f) {
                g_afCarSelectSlide[player] = 0.0f;
            }
        }
    }

    /* The slots of the cars shown when ready, and with the decals. */
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (g_adwCarSelectState[player] == CAR_SELECT_READY) {
            if (!g_dwIsPaintMode && !g_dwIsReviewMode) {
                g_adwCarSelectModelSlots[player] =
                    (g_aVehicles[player].customDecals << 1) ^ (player ^ 2);
            }
        } else if (g_adwCarSelectState[player] == CAR_SELECT_DECALS) {
            if (g_anCarSelectCursor[player] == DECALS_DEFAULT) {
                g_adwCarSelectModelSlots[player] = player ^ 2;
            } else if (g_anCarSelectCursor[player] == DECALS_CUSTOM) {
                g_adwCarSelectModelSlots[player] = player;
            } else if (g_anCarSelectCursor[player] == DECALS_LOAD) {
                g_adwCarSelectModelSlots[player] =
                    g_anCarSelectPrevCursor[player] != 0 ? player ^ 2 : player;
            }
        }
    }
    g_dwSnowMode = FALSE;
    g_dwWeatherActive = FALSE;
    g_dwRainMode = FALSE;
    g_dwNightMode = FALSE;

    if (g_dwIsReviewMode) {
        i = g_dwResultsPage == RESULTS_PAGE_PLAYER_2;
        DrawPlayerStage(0, i, g_adwCarSelectModelSlots[i], 59, g_dwScreenHeight, SPREAD_TWO_VIEWS);
    } else if (g_dwNumLocalPlayers == 1) {
        DrawPlayerStage(0, 0, g_adwCarSelectModelSlots[0], 45, g_dwScreenHeight, SPREAD_ONE_VIEW);
    } else {
        DrawPlayerStage(
            0, 0, g_adwCarSelectModelSlots[0], 0, g_dwScreenHeight - 24, SPREAD_TWO_VIEWS);
        DrawPlayerStage(1, 1, g_adwCarSelectModelSlots[1], 71, g_dwScreenHeight, SPREAD_TWO_VIEWS);
    }
    SetupMenuProjection((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    if (g_dwTitleRedrawFrames != 0 || !IsFadeComplete()) {
        DrawCarSelectTitle(text);
        if (IsFadeComplete()) {
            g_dwTitleRedrawFrames--;
        }
    }

    if (g_dwIsReviewMode) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(20);
        if (g_dwResultsPage == RESULTS_PAGE_PLAYER_1 || g_dwResultsPage == RESULTS_PAGE_PLAYER_2) {
            DrawRaceResults(g_dwResultsPage != RESULTS_PAGE_PLAYER_1, time);
        } else if (g_dwResultsPage == RESULTS_PAGE_MESSAGE) {
            DrawSeasonMessage();
        } else {
            DrawSeasonResults();
        }
    } else if (g_dwNumLocalPlayers == 2) {
        DrawTwoPlayerChoices(text);
    } else {
        DrawOnePlayerChoices(text);
    }

    if (g_dwTitleRedrawFrames != 0 || IsFadingOut() || IsFadingIn()) {
        if (g_dwIsReviewMode) {
            DrawPauseMenuButtons();
        } else {
            DrawMenuFooterButtons();
        }
    }
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    if (g_dwPakDialogActive) {
        RunPakDialog();
    }
    FlushDisplayList();

    if (IsFadingOut()) {
        if (IsFadeComplete()) {
            LeaveCarSelect();
        }
        return;
    }
    if (g_dwPakDialogActive) {
        if (g_bPakDialogDone) {
            g_bPakDialogDone = FALSE;
            g_dwPakDialogActive = FALSE;
        }
        return;
    }
    /* In the paint shop, the player of the screen steers the first vehicle. */
    if (g_dwIsPaintMode) {
        g_aVehicles[0].input->buttons = g_aVehicles[g_dwScreenPlayerIdx].input->buttons;
        g_aVehicles[0].input->stickX = g_aVehicles[g_dwScreenPlayerIdx].input->stickX;
        g_aVehicles[0].input->stickY = g_aVehicles[g_dwScreenPlayerIdx].input->stickY;
    }
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        HandleCarSelectInput(player);
    }
}

void ProcessRaceResultsScreen(void) {
    g_dwIsReviewMode = TRUE;
    ProcessCarSelectScreen();
    g_dwIsReviewMode = FALSE;
}
