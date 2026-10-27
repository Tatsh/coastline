#ifndef COASTLINE_EAR_ENVIRONMENT_H
#define COASTLINE_EAR_ENVIRONMENT_H

#include <cstdint>

/**
 * @ghidraAddress EAR: 0x100046bd
 * @stub
 */
void RegisterDefaultEnvironment(void);

/** The physical meanings of the reserved registration properties remain unidentified. */
class [[gnu::packed]] EarEnvironment {
public:
    /** @ghidraAddress EAR: 0x1000ea30 */
    void SetReservedRegistrationDwordA(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000ea50 */
    void SetReservedRegistrationWord(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ea70 */
    void SetEnvironmentInitialGain(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ea90 */
    void SetReservedRegistrationDwordB(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000eab0 */
    void SetEnvironmentSoundSpeed(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ead0 */
    void SetReservedRegistrationDwordC(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000eaf0 */
    void SetEnvironmentPitchVariationRange(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb10 */
    void SetEnvironmentDopplerCrossoverDistance(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb30 */
    void SetReservedAuxiliaryWordB(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb50 */
    void SetReservedAuxiliaryWordA(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb70 */
    void SetReservedAuxiliaryWordC(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb90 */
    void SetReservedAuxiliaryWordD(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ebb0 */
    void SetReservedAuxiliaryWordE(std::uint16_t value);

    /** @ghidraAddress EAR: 0x10008a40 */
    std::uint16_t GetEnvironmentInitialGain() const;
    /** @ghidraAddress EAR: 0x10008a60 */
    std::uint16_t GetEnvironmentSoundSpeed() const;
    /** @ghidraAddress EAR: 0x10008a80 */
    std::uint16_t GetEnvironmentDopplerCrossoverDistance() const;
    /** @ghidraAddress EAR: 0x1000d710 */
    std::uint16_t GetEnvironmentPitchVariationRange() const;

private:
    std::uint32_t reservedRegistrationDwordA;
    std::uint16_t reservedRegistrationWord;
    std::uint32_t reservedRegistrationDwordB;
    std::uint16_t soundSpeed;
    [[maybe_unused]] std::uint8_t unwrittenBytes[6]; /* +0x0c */
    std::uint32_t reservedRegistrationDwordC;
    std::uint16_t initialGain;
    std::uint16_t pitchVariationRange;
    std::uint16_t dopplerCrossoverDistance;
    std::uint16_t reservedAuxiliaryWordA;
    std::uint16_t reservedAuxiliaryWordB;
    std::uint16_t reservedAuxiliaryWordC;
    std::uint16_t reservedAuxiliaryWordD;
    std::uint16_t reservedAuxiliaryWordE;
};

class [[gnu::packed]] EarEnvironmentRegistration {
public:
    /** @ghidraAddress EAR: 0x10007220 */
    void InitializeDefaultEnvironment();

private:
    std::uint16_t size;
    std::uint16_t reservedRegistrationWord;
    std::uint16_t soundSpeed;
    std::uint32_t reservedRegistrationDwordA;
    std::uint32_t reservedRegistrationDwordB;
    std::uint32_t reservedRegistrationDwordC;
    std::uint16_t frequency;
    std::uint16_t bitDepth;
    std::uint16_t pitchVariationRange;
    std::uint16_t initialGain;
    std::uint16_t reservedAuxiliaryWordA;
    std::uint16_t reservedAuxiliaryWordB;
    std::uint16_t dopplerCrossoverDistance;
    std::uint16_t reservedAuxiliaryWordC;
    std::uint16_t reservedAuxiliaryWordD;
    std::uint16_t reservedAuxiliaryWordE;
    std::uint8_t unknownBytes[22]; /* +0x26 */
    std::uint32_t requestedHandle;
};

#endif /* COASTLINE_EAR_ENVIRONMENT_H */
