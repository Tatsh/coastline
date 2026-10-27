#include "platform/win/frontend.h"

const signed char g_abCarShadowImageIds[2] = {99, 98};

const signed char g_abInputDeviceImageIds[4] = {50, 51, 49, 96};

const signed char g_abServiceProviderImageIds[4] = {69, 68, 67, 70};

const signed char g_abSpecularLightingImageIds[2] = {101, 100};

const signed char g_abSuspensionImageIds[3] = {13, 14, 15};

const signed char g_abTireImageIds[3] = {26, 25, 24};

const signed char g_abTransmissionImageIds[2] = {29, 12};

int g_adwCarMenuIndexToCarId[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

const int g_adwCarShadowMenuIndexToValue[2] = {1, 0};

const int g_adwForceFeedbackMenuIndexToValue[2] = {1, 0};

const int g_adwSkidMarksMenuIndexToValue[2] = {1, 0};

const int g_adwSpecularLightingMenuIndexToValue[2] = {1, 0};

int g_adwSuspensionMenuIndexToType[3] = {0, 1, 2};

int g_adwTireMenuIndexToTireType[3] = {0, 1, 2};

int g_adwTrackMenuIndexToTrackId[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

int g_adwTransmissionMenuIndexToType[2] = {1, 0};

int g_adwWeatherMenuIndexToWeather[5] = {0, 1, 2, 3, 4};

const int g_awSkidMarksImageIds[2] = {140, 141};

int g_bRaceSetupShowCarSetup = 1;

int g_dwCapturedKeyBindingValue = -1;
