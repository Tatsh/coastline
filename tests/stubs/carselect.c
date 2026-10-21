/*
 * Stubs for the host check of src/carselect.c, kept apart from the headers, so each is a routine
 * that does nothing whatever the prototype it stands in for.
 */
static char heap[1 << 16];

void AddTimingMarker(void) {
}
void AllocateEditorBuffers(void) {
}
void ApplyScreenFade(void) {
}
void CheckVehicleVisibility(void) {
}
void ChooseVehicleCar(void) {
}
void ClearFramebuffersLowRes(void) {
}
void ClearFramebuffersForWideMode(void) {
}
void DecompressFromRom(void) {
}
void DrawMenuFooterButtons(void) {
}
void DrawModelList(void) {
}
void DrawPakErrorMessage(void) {
}
void DrawPauseMenuButtons(void) {
}
void DrawText(void) {
}
void DrawTexturedRect(void) {
}
void DrawTimingBars(void) {
}
void DriveAiVehicle(void) {
}
void EmitTachometerDisplayList(void) {
}
void ExtractVehicleBaseColor(void) {
}
void FadeMusicTo(void) {
}
void FadeSfxTo(void) {
}
void FlushDisplayList(void) {
}
void InitSoundChannel(void) {
}
void InitVehicleColor(void) {
}
void InstallVehicleModel(void) {
}
void LoadAndDecompressAsset(void) {
}
void LoadMenuItemModels(void) {
}
void MarkVisibleCell(void) {
}
void ProcessDataSaveLoadMenu(void) {
}
void ProcessMainMenu(void) {
}
void ProcessPaintShopScreen(void) {
}
void ProcessRacingScreen(void) {
}
void ProcessTrackMenu(void) {
}
void ProcessWeatherSelect(void) {
}
void RenderSpeedometerNeedle(void) {
}
void RenderTrackShadow(void) {
}
void RenderVehicleModel(void) {
}
void ResetHeap(void) {
}
void SetDebugMessage(void) {
}
void StartHighResFrame(void) {
}
void SetFadeTarget(void) {
}
void SetMaterialColor(void) {
}
void SetScissorRect(void) {
}
void SetTextAlignCenter(void) {
}
void SetTextAlignLeft(void) {
}
void SetTextAlignRight(void) {
}
void SetTextCustomColors(void) {
}
void SetTextSize(void) {
}
void SetTextStyleNormal(void) {
}
void SetTrackFromSeason(void) {
}
void SetupCameraMatrix(void) {
}
void ClearDepthBuffer(void) {
}
void SetupFog(void) {
}
void SetupMenuProjection(void) {
}
void SetupWeatherLights(void) {
}
void SetVehicleModelSlot(void) {
}
void SetViewport(void) {
}
void TransformWheelPositions(void) {
}
void TransferFlags(void) {
}
void UpdateFadeTransitions(void) {
}
void UpdateFrameTiming(void) {
}
void UpdatePhysicsStep(void) {
}
void UpdateStickRepeat(void) {
}
int FindFreeSoundChannel(void) {
    return -1;
}
int IsFadingIn(void) {
    return 0;
}
int IsVehicleModelLoaded(void) {
    return 1;
}
int IsVehicleModelValid(void) {
    return 1;
}
int PollVehicleModelDma(void) {
    return 0;
}
int ProcessDecalSaveLoad(void) {
    return 0;
}
unsigned DmaReadAssetSize(void) {
    return 4;
}
void *AllocHeapAligned(void) {
    return heap;
}
void *AllocLightBlock(void) {
    return heap;
}
void *AllocDisplayListBlock(void) {
    return heap;
}
