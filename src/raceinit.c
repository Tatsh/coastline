#include "raceinit.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RECORDING_SELF this
#define RECORDING_OPTION(offset) ((s32)(s8)input->playbackBuffer[offset])
void Vehicle::SetupFromRecording(void) {
#else
#define RECORDING_SELF vehicle
#define RECORDING_OPTION(offset) (vehicle->input->playbackBuffer[offset])
void SetupFromRecording(Vehicle *vehicle) {
#endif
    RECORDING_SELF->steeringMode = RECORDING_OPTION(2);
    RECORDING_SELF->chosenTransmission = RECORDING_OPTION(3);
    RECORDING_SELF->tireType = RECORDING_OPTION(4);
    RECORDING_SELF->suspensionStiffness = RECORDING_OPTION(5);
    RECORDING_SELF->input->controllerLayout = RECORDING_SELF->input->playbackBuffer[6];
}
