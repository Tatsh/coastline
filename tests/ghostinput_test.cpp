#include <cassert>
#include <cmath>

#include "common/inputtranslate.c"
#include "common/playerinput.c"
#include "common/vehiclephysics.h"
#include "common/vehiclesetup.cpp"

RallyConfig::RallyConfig() = default;
RallyConfig g_GlobalRallyConfig;
u32 g_dwRacePauseState;
s32 gInstantReplayActive;
s32 g_dwGameMode = GAME_MODE_TIME_ATTACK;
Vehicle g_aVehicles[MAX_VEHICLES];
u8 g_abPlayerColorsRgb[20][3];
PlayerProfile g_aSeasonSaveBuffers[2];
s32 g_dwNumTracks = 15;

void InitPlayerProfileDefaults(PlayerProfile *, s32) {
}

VehicleCarChoice ChooseVehicleCarId(Vehicle *, s32 *car) {
    *car = 1;
    return CAR_CHOICE_SELECTED;
}

extern "C" int *GetSdlRaceStarted(void) {
    static int started = 1;
    return &started;
}

extern "C" void RunRaceGameScreen(void) {
}

s32 IsCurrentScreen(ScreenCallback callback) {
    return callback == RunRaceGameScreen;
}

void guMtxIdentF(float matrix[4][4]) {
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            matrix[row][column] = row == column ? 1.0f : 0.0f;
        }
    }
}

int main() {
    g_GlobalRallyConfig.pActiveInputBindings = g_GlobalRallyConfig.aInputBindingsKeyboard;
    auto &input = g_aPlayerInputState[1];
    input.InitPlayerInputState();
    auto &vehicle = g_aVehicles[1];
    vehicle.InitPlayerVehicleSlot();
    u8 recording[]{2, 1, 0, 1, 2, 1, 0, 0, 0xba, 0x01, 0x46, 0x24};
    input.playbackBuffer = recording;
    input.playbackPosition = RECORDING_HEADER_SIZE;
    input.playbackLimit = sizeof(recording);
    input.TranslateControllerInput();
    assert(input.steeringInput == -70 && std::abs(input.steering + 1.0f) < 0.00001f);
    assert((input.buttons & INPUT_RECORDED_ACTIONS) == INPUT_ACCELERATE);
    assert(input.playbackPosition == 10);
    assert(ShouldStepVehiclePhysics(&vehicle));
    g_dwRacePauseState = 1;
    input.TranslateControllerInput();
    assert(input.steeringInput == 70);
    assert((input.buttons & INPUT_RECORDED_ACTIONS) == (INPUT_BRAKE | INPUT_GEAR_DOWN));
    assert(input.playbackPosition == 10);
    g_dwRacePauseState = 0;
    input.TranslateControllerInput();
    assert(input.playbackPosition == 12);
    input.TranslateControllerInput();
    assert(input.steeringInput == 0 && (input.buttons & INPUT_RECORDED_ACTIONS) == 0);
    assert(input.playbackPosition == 12);
    input.playbackLimit = RECORDING_HEADER_SIZE;
    assert(!ShouldStepVehiclePhysics(&vehicle));
    input.playbackLimit = sizeof(recording);
    gInstantReplayActive = 1;
    assert(!ShouldStepVehiclePhysics(&vehicle));
    gInstantReplayActive = 0;
    input.playbackPosition = sizeof(recording) - 1;
    input.TranslateControllerInput();
    assert(input.steeringInput == 0 && (input.buttons & INPUT_RECORDED_ACTIONS) == 0);
}
