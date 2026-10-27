#include "environment.h"

#include <cstring>

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
    reservedDefaultWordA = 22050;
    reservedDefaultWordB = 16;
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
