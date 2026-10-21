/*
 * Stubs for the host check of src/racemenu.c. They are kept apart from the headers, so each is a
 * routine that does nothing whatever the prototype it stands in for.
 */
static char heap[1 << 16];

void AddTimingMarker(void) {
}
void AdvanceModelAnimations(void) {
}
void ApplyScreenFade(void) {
}
void ClearFramebuffersForWideMode(void) {
}
void DecompressFromRom(void) {
}
void DecompressModel(void) {
}
void DrawText(void) {
}
void DrawTexturedRect(void) {
}
void DrawTimingBars(void) {
}
void FadeMusicTo(void) {
}
void FadeSfxTo(void) {
}
void FlushDisplayList(void) {
}
void FormatLapTime(void) {
}
void InitVehicleColor(void) {
}
void InstallVehicleModel(void) {
}
void LoadAndDecompressAsset(void) {
}
void PreparePakDataScreen(void) {
}
void LoadVehicleModel(void) {
}
void osViBlack(void) {
}
void ProcessMainMenu(void) {
}
void ProcessPlayerCountSelect(void) {
}
void ProcessCarSelectScreen(void) {
}
void ProcessWeatherSelect(void) {
}
void RenderCameraView(void) {
}
void ResetHeap(void) {
}
void SetCurrentScreenCallback(void) {
}
void SetDebugMessage(void) {
}
void StartLowResFrame(void) {
}
void StartHighResFrame(void) {
}
void SetFadeTarget(void) {
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
void ClearDepthBuffer(void) {
}
void SetupFog(void) {
}
void SetupMenuProjection(void) {
}
void SetupWeatherLights(void) {
}
void SetViewport(void) {
}
void StartChampionshipRace(void) {
}
void StartTimeAttack(void) {
}
void UpdateFadeTransitions(void) {
}
void UpdateFrameTiming(void) {
}
void UpdatePhysicsStep(void) {
}
int PollVehicleModelDma(void) {
    return 0;
}
int ProcessPakDataScreen(void) {
    return 0;
}
unsigned DmaReadAssetSize(void) {
    return 4;
}
void *AllocHeapAligned(void) {
    return heap;
}
