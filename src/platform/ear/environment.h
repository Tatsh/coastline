#ifndef COASTLINE_EAR_ENVIRONMENT_H
#define COASTLINE_EAR_ENVIRONMENT_H

#include <cstdint>

/** The physical meanings of the reserved registration properties remain unidentified. */
class [[gnu::packed]] EarEnvironment {
public:
    /** @ghidraAddress EAR: 0x1000ea30 */
    void __thiscall SetReservedRegistrationDwordA(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000ea50 */
    void __thiscall SetReservedRegistrationWord(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ea70 */
    void __thiscall SetEnvironmentInitialGain(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ea90 */
    void __thiscall SetReservedRegistrationDwordB(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000eab0 */
    void __thiscall SetEnvironmentSoundSpeed(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ead0 */
    void __thiscall SetReservedRegistrationDwordC(std::uint32_t value);
    /** @ghidraAddress EAR: 0x1000eaf0 */
    void __thiscall SetEnvironmentPitchVariationRange(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb10 */
    void __thiscall SetEnvironmentDopplerCrossoverDistance(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb30 */
    void __thiscall SetReservedAuxiliaryWordB(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb50 */
    void __thiscall SetReservedAuxiliaryWordA(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb70 */
    void __thiscall SetReservedAuxiliaryWordC(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000eb90 */
    void __thiscall SetReservedAuxiliaryWordD(std::uint16_t value);
    /** @ghidraAddress EAR: 0x1000ebb0 */
    void __thiscall SetReservedAuxiliaryWordE(std::uint16_t value);

    /** @ghidraAddress EAR: 0x10008a40 */
    std::uint16_t __thiscall GetEnvironmentInitialGain() const;
    /** @ghidraAddress EAR: 0x10008a60 */
    std::uint16_t __thiscall GetEnvironmentSoundSpeed() const;
    /** @ghidraAddress EAR: 0x10008a80 */
    std::uint16_t __thiscall GetEnvironmentDopplerCrossoverDistance() const;
    /** @ghidraAddress EAR: 0x1000d710 */
    std::uint16_t __thiscall GetEnvironmentPitchVariationRange() const;

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
    void __thiscall InitializeDefaultEnvironment();

private:
    std::uint16_t size;
    std::uint16_t reservedRegistrationWord;
    std::uint16_t soundSpeed;
    std::uint32_t reservedRegistrationDwordA;
    std::uint32_t reservedRegistrationDwordB;
    std::uint32_t reservedRegistrationDwordC;
    std::uint16_t reservedDefaultWordA;
    std::uint16_t reservedDefaultWordB;
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

#endif
