#include "environment.h"

#include <cstring>

#include "error.h"
#include "event.h"
#include "mfc.h"
#include "registry.h"
#include "runtime.h"

/** The size EarEnvironmentRegistration::size must give. */
#define EAR_ENVIRONMENT_REGISTRATION_SIZE 0x40
/** The handle of the default environment. */
#define EAR_DEFAULT_ENVIRONMENT 1
/** The handle a registration requests for a new handle. */
#define EAR_NEW_ENVIRONMENT_HANDLE 0xffffffff
/** The lowest pitch variation range an environment takes. */
#define EAR_MINIMUM_PITCH_VARIATION_RANGE 100

int32_t g_bDefaultEnvironmentRegistered;

void RegisterDefaultEnvironment(void) {
    EarEnvironmentRegistration registration;
    registration.InitializeDefaultEnvironment();
    EarRegisterEnvironmentExportWrapper(&registration);
    g_bDefaultEnvironmentRegistered = 1;
}

void __stdcall OnDefaultEnvironmentRegistered(EarEnvironment *environment) {
}

uint16_t __stdcall
EarRegisterEnvironmentExportWrapper(const EarEnvironmentRegistration *registration) {
    return EAR_DLL_RegisterEnvironment(registration);
}

uint16_t __stdcall EAR_DLL_RegisterEnvironment(const EarEnvironmentRegistration *registration) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    uint32_t size;
    // Only the low word of the size is read; the high word stays as the stack left it.
    std::memmove(&size, registration, sizeof(registration->size));
    if ((size & 0xffff) != EAR_ENVIRONMENT_REGISTRATION_SIZE) {
        SetEarLastError(0x1d);
        return 0;
    }
    g_EnvironmentRegistry.GrowWhenFull();
    EarEnvironmentRegistration copy;
    std::memmove(&copy, registration, EAR_ENVIRONMENT_REGISTRATION_SIZE);
    uint16_t handle = static_cast<uint16_t>(copy.requestedHandle);
    while (g_bPlaybackUpdateLocked) {
    }
    g_bPlaybackUpdateLocked = 1;
    EarEnvironment *environment;
    if (handle == 0) {
        SetEarLastError(0x1f);
    } else {
        int registering = 1;
        if (handle == EAR_DEFAULT_ENVIRONMENT) {
            if (g_EnvironmentRegistry.LookupWordPointerMapValue(EAR_DEFAULT_ENVIRONMENT,
                                                                environment)) {
                ::operator delete(environment);
                g_EnvironmentRegistry.RemoveKey(EAR_DEFAULT_ENVIRONMENT);
                g_bDefaultEnvironmentRegistered = 0;
            }
        } else if (copy.requestedHandle != EAR_NEW_ENVIRONMENT_HANDLE) {
            if (g_EnvironmentRegistry.LookupWordPointerMapValue(handle, environment)) {
                SetEarLastError(0x1e);
                registering = 0;
            }
        } else {
            do {
                handle = GenerateEarObjectHandle();
            } while (g_EnvironmentRegistry.LookupWordPointerMapValue(handle, environment));
        }
        if (registering) {
            // The original does not check the allocation.
            environment =
                static_cast<EarEnvironment *>(MfcAllocateWithNewHandler(sizeof(EarEnvironment)));
            environment->SetEnvironmentInitialGain(copy.initialGain);
            environment->SetReservedRegistrationDwordA(copy.reservedRegistrationDwordA);
            environment->SetReservedRegistrationWord(copy.reservedRegistrationWord);
            environment->SetReservedRegistrationDwordB(copy.reservedRegistrationDwordB);
            environment->SetEnvironmentSoundSpeed(copy.soundSpeed);
            environment->SetReservedRegistrationDwordC(copy.reservedRegistrationDwordC);
            environment->SetEnvironmentPitchVariationRange(
                copy.pitchVariationRange > EAR_MINIMUM_PITCH_VARIATION_RANGE ?
                    copy.pitchVariationRange :
                    EAR_MINIMUM_PITCH_VARIATION_RANGE);
            environment->SetEnvironmentDopplerCrossoverDistance(copy.dopplerCrossoverDistance);
            environment->SetReservedAuxiliaryWordA(copy.reservedAuxiliaryWordA);
            environment->SetReservedAuxiliaryWordB(copy.reservedAuxiliaryWordB);
            environment->SetReservedAuxiliaryWordC(copy.reservedAuxiliaryWordC);
            environment->SetReservedAuxiliaryWordD(copy.reservedAuxiliaryWordD);
            environment->SetReservedAuxiliaryWordE(copy.reservedAuxiliaryWordE);
            g_EnvironmentRegistry.SetWordPointerMapValue(handle, environment);
            if (handle == EAR_DEFAULT_ENVIRONMENT) {
                OnDefaultEnvironmentRegistered(environment);
            }
        }
    }
    g_bPlaybackUpdateLocked = 0;
    return handle;
}

int __stdcall EarShutDownEnvironmentExportWrapper(uint32_t environment) {
    return EAR_DLL_ShutDownEnvironment(environment);
}

int __stdcall EAR_DLL_ShutDownEnvironment(uint32_t environment) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_EnvironmentRegistry.IsEmpty()) {
        SetEarLastError(0x1f);
        return 0;
    }
    int locked = 0;
    int shutDown = 1;
    while (g_bPlaybackUpdateLocked) {
    }
    if (!g_bRegistryMutationLocked) {
        locked = 1;
        g_bRegistryMutationLocked = locked;
    }
    EarEnvironment *found;
    if (environment != 0) {
        if (g_EnvironmentRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(environment),
                                                            found)) {
            ::operator delete(found);
            g_EnvironmentRegistry.RemoveKey(static_cast<uint16_t>(environment));
        } else {
            SetEarLastError(0x1f);
            shutDown = 0;
        }
    } else {
        POSITION position = g_EnvironmentRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_EnvironmentRegistry.GetNextAssoc(position, key, value);
            ::operator delete(value);
        } while (position);
        g_EnvironmentRegistry.RemoveAll();
    }
    if ((environment == EAR_DEFAULT_ENVIRONMENT || environment == 0) && locked) {
        RegisterDefaultEnvironment();
    }
    if (locked) {
        g_bRegistryMutationLocked = 0;
    }
    return shutDown;
}

void EarEnvironment::SetReservedRegistrationDwordA(std::uint32_t value) {
    reservedRegistrationDwordA = value;
}

void EarEnvironment::SetReservedRegistrationWord(std::uint16_t value) {
    reservedRegistrationWord = value;
}

void EarEnvironment::SetEnvironmentInitialGain(std::uint16_t value) {
    initialGain = value;
}

void EarEnvironment::SetReservedRegistrationDwordB(std::uint32_t value) {
    reservedRegistrationDwordB = value;
}

void EarEnvironment::SetEnvironmentSoundSpeed(std::uint16_t value) {
    soundSpeed = value;
}

void EarEnvironment::SetReservedRegistrationDwordC(std::uint32_t value) {
    reservedRegistrationDwordC = value;
}

void EarEnvironment::SetEnvironmentPitchVariationRange(std::uint16_t value) {
    pitchVariationRange = value;
}

void EarEnvironment::SetEnvironmentDopplerCrossoverDistance(std::uint16_t value) {
    dopplerCrossoverDistance = value;
}

void EarEnvironment::SetReservedAuxiliaryWordB(std::uint16_t value) {
    reservedAuxiliaryWordB = value;
}

void EarEnvironment::SetReservedAuxiliaryWordA(std::uint16_t value) {
    reservedAuxiliaryWordA = value;
}

void EarEnvironment::SetReservedAuxiliaryWordC(std::uint16_t value) {
    reservedAuxiliaryWordC = value;
}

void EarEnvironment::SetReservedAuxiliaryWordD(std::uint16_t value) {
    reservedAuxiliaryWordD = value;
}

void EarEnvironment::SetReservedAuxiliaryWordE(std::uint16_t value) {
    reservedAuxiliaryWordE = value;
}

std::uint16_t EarEnvironment::GetEnvironmentInitialGain() const {
    return initialGain;
}

std::uint16_t EarEnvironment::GetEnvironmentSoundSpeed() const {
    return soundSpeed;
}

std::uint16_t EarEnvironment::GetEnvironmentDopplerCrossoverDistance() const {
    return dopplerCrossoverDistance;
}

std::uint16_t EarEnvironment::GetEnvironmentPitchVariationRange() const {
    return pitchVariationRange;
}

void EarEnvironmentRegistration::InitializeDefaultEnvironment() {
    size = 64;
    reservedRegistrationDwordA = 0;
    reservedRegistrationWord = 16000;
    reservedRegistrationDwordB = 0;
    soundSpeed = 33;
    reservedRegistrationDwordC = 500;
    frequency = 22050;
    bitDepth = 16;
    pitchVariationRange = 1500;
    initialGain = 6000;
    reservedAuxiliaryWordA = 0;
    reservedAuxiliaryWordB = 0;
    dopplerCrossoverDistance = 0;
    reservedAuxiliaryWordC = 0;
    reservedAuxiliaryWordD = 0;
    reservedAuxiliaryWordE = 0;
    requestedHandle = 1;
    std::memset(unknownBytes, 0, sizeof(unknownBytes));
}
