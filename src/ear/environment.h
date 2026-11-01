#ifndef COASTLINE_EAR_ENVIRONMENT_H
#define COASTLINE_EAR_ENVIRONMENT_H

#include <cstdint>

class EarEnvironment;
class EarEnvironmentRegistration;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attempt to register default environment 1 and set the default-registration marker.
 * Registration failures remain available through the EAR error API.
 *
 * @ghidraAddress EAR: 0x100046bd
 */
void RegisterDefaultEnvironment(void);

/**
 * Do nothing with the default environment once it is registered.
 *
 * @param environment The borrowed registered default environment, unused by this callback.
 * @ghidraAddress EAR: 0x100100f6
 */
void __stdcall OnDefaultEnvironmentRegistered(EarEnvironment *environment);

/**
 * Forward an environment registration to EAR_DLL_RegisterEnvironment().
 *
 * @param registration A non-null readable registration of 0x40 bytes, borrowed during the call.
 * @return The requested or generated handle, including an existing handle on duplicate error;
 * zero for an unavailable engine, incorrect size, or zero requested handle.
 * @ghidraAddress EAR: 0x1000e6c0
 */
uint16_t __stdcall
EarRegisterEnvironmentExportWrapper(const EarEnvironmentRegistration *registration);

/**
 * Register an environment under the requested handle, truncated to 16 bits, or generate a new
 * handle for 0xffffffff. Handle 1 replaces the registered default environment. Other duplicate
 * handles are refused with error 0x1e and returned unchanged.
 *
 * @param registration A non-null readable registration of 0x40 bytes, copied during the call.
 * Its size field must be 0x40. Pitch variation is clamped to a minimum of 100 during registration.
 * @return The requested or generated handle, including an existing handle on duplicate error;
 * zero for an unavailable engine, incorrect size, or zero requested handle.
 * @ghidraAddress EAR: 0x1000e6d0
 */
uint16_t __stdcall EAR_DLL_RegisterEnvironment(const EarEnvironmentRegistration *registration);

/**
 * Forward an environment shutdown to EAR_DLL_ShutDownEnvironment().
 *
 * @param environment The handle, truncated to 16 bits for lookup, or zero for every environment.
 * @return One on successful removal; zero for an unavailable engine, empty registry, or missing
 * handle, with error 0x1f recorded for registry lookup failures.
 * @ghidraAddress EAR: 0x1000f310
 */
int __stdcall EarShutDownEnvironmentExportWrapper(uint32_t environment);

/**
 * Release one environment or every environment after waiting for playback updates.
 * Requests for handle 1 or all environments repeat default registration when the call acquired
 * the registry mutation lock, even if lookup of handle 1 fails.
 *
 * @param environment The handle, truncated to 16 bits for lookup, or zero for every environment.
 * @return One on successful removal; zero for an unavailable engine, empty registry, or missing
 * handle, with error 0x1f recorded for registry lookup failures.
 * @ghidraAddress EAR: 0x1000f320
 */
int __stdcall EAR_DLL_ShutDownEnvironment(uint32_t environment);

/**
 * Default-registration marker, set after a registration attempt even when registration fails.
 *
 * @ghidraAddress EAR: 0x100360f0
 */
extern int32_t g_bDefaultEnvironmentRegistered;
#ifdef __cplusplus
}
#endif

/**
 * Spatial playback environment properties copied during registration.
 * Direct setters store values without validation. Reserved properties have unidentified meanings.
 * Borrowed environment pointers become invalid after replacement or shutdown.
 */
class [[gnu::packed]] EarEnvironment {
public:
    /**
     * Store the first reserved registration dword.
     * @param value The uninterpreted 32-bit property value.
     * @ghidraAddress EAR: 0x1000ea30
     */
    void SetReservedRegistrationDwordA(std::uint32_t value);
    /**
     * Store the reserved registration word.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000ea50
     */
    void SetReservedRegistrationWord(std::uint16_t value);
    /**
     * Store initial attenuation used by spatial suppression.
     * @param value The gain in units of 1/10000 of full amplitude.
     * @ghidraAddress EAR: 0x1000ea70
     */
    void SetEnvironmentInitialGain(std::uint16_t value);
    /**
     * Store the second reserved registration dword.
     * @param value The uninterpreted 32-bit property value.
     * @ghidraAddress EAR: 0x1000ea90
     */
    void SetReservedRegistrationDwordB(std::uint32_t value);
    /**
     * Store sound speed for Doppler calculations.
     * @param value The speed in centimetres per millisecond; distance-delay processing requires
     * a nonzero value.
     * @ghidraAddress EAR: 0x1000eab0
     */
    void SetEnvironmentSoundSpeed(std::uint16_t value);
    /**
     * Store the third reserved registration dword.
     * @param value The uninterpreted 32-bit property value.
     * @ghidraAddress EAR: 0x1000ead0
     */
    void SetReservedRegistrationDwordC(std::uint32_t value);
    /**
     * Store the default random pitch half-range.
     * @param value The variation in units of 1/10000 of base frequency, without clamping.
     * @ghidraAddress EAR: 0x1000eaf0
     */
    void SetEnvironmentPitchVariationRange(std::uint16_t value);
    /**
     * Store the distance threshold for smoothing Doppler direction changes.
     * @param value The nonnegative threshold in centimetres.
     * @ghidraAddress EAR: 0x1000eb10
     */
    void SetEnvironmentDopplerCrossoverDistance(std::uint16_t value);
    /**
     * Store reserved auxiliary property B.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000eb30
     */
    void SetReservedAuxiliaryWordB(std::uint16_t value);
    /**
     * Store reserved auxiliary property A.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000eb50
     */
    void SetReservedAuxiliaryWordA(std::uint16_t value);
    /**
     * Store reserved auxiliary property C.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000eb70
     */
    void SetReservedAuxiliaryWordC(std::uint16_t value);
    /**
     * Store reserved auxiliary property D.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000eb90
     */
    void SetReservedAuxiliaryWordD(std::uint16_t value);
    /**
     * Store reserved auxiliary property E.
     * @param value The uninterpreted 16-bit property value.
     * @ghidraAddress EAR: 0x1000ebb0
     */
    void SetReservedAuxiliaryWordE(std::uint16_t value);

    /**
     * @return The stored initial gain in units of 1/10000 of full amplitude.
     * @ghidraAddress EAR: 0x10008a40
     */
    std::uint16_t GetEnvironmentInitialGain() const;
    /**
     * @return The stored Doppler sound speed in centimetres per millisecond.
     * @ghidraAddress EAR: 0x10008a60
     */
    std::uint16_t GetEnvironmentSoundSpeed() const;
    /**
     * @return The stored Doppler crossover threshold in centimetres.
     * @ghidraAddress EAR: 0x10008a80
     */
    std::uint16_t GetEnvironmentDopplerCrossoverDistance() const;
    /**
     * @return The stored pitch half-range in units of 1/10000 of base frequency.
     * @ghidraAddress EAR: 0x1000d710
     */
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

/**
 * Packed 0x40-byte environment registration copied by EAR_DLL_RegisterEnvironment().
 * Initialisation assigns defaults; public fields can be adjusted before registration.
 */
class [[gnu::packed]] EarEnvironmentRegistration {
public:
    /**
     * Set default environment properties, clear reserved bytes, and request handle 1.
     * Defaults include gain 6000, pitch variation 1500, sound speed 33, and zero crossover
     * distance.
     * @ghidraAddress EAR: 0x10007220
     */
    void InitializeDefaultEnvironment();

    /** Registration byte count; registration requires 0x40. */
    std::uint16_t size;
    /** Uninterpreted registration property, defaulting to 16000. */
    std::uint16_t reservedRegistrationWord;
    /** Doppler sound speed in centimetres per millisecond, defaulting to 33. */
    std::uint16_t soundSpeed;
    /** First uninterpreted 32-bit property, defaulting to zero. */
    std::uint32_t reservedRegistrationDwordA;
    /** Second uninterpreted 32-bit property, defaulting to zero. */
    std::uint32_t reservedRegistrationDwordB;
    /** Third uninterpreted 32-bit property, defaulting to 500. */
    std::uint32_t reservedRegistrationDwordC;
    /** Nominal frequency in hertz, defaulting to 22050; not transferred into EarEnvironment. */
    std::uint16_t frequency;
    /** Nominal sample bit depth, defaulting to 16; not transferred into EarEnvironment. */
    std::uint16_t bitDepth;
    /** Pitch variation half-range in units of 1/10000, clamped to at least 100 on registration. */
    std::uint16_t pitchVariationRange;
    /** Suppression initial gain in units of 1/10000 of full amplitude, defaulting to 6000. */
    std::uint16_t initialGain;
    /** Uninterpreted auxiliary property A, defaulting to zero. */
    std::uint16_t reservedAuxiliaryWordA;
    /** Uninterpreted auxiliary property B, defaulting to zero. */
    std::uint16_t reservedAuxiliaryWordB;
    /** Doppler crossover distance in centimetres, defaulting to zero. */
    std::uint16_t dopplerCrossoverDistance;
    /** Uninterpreted auxiliary property C, defaulting to zero. */
    std::uint16_t reservedAuxiliaryWordC;
    /** Uninterpreted auxiliary property D, defaulting to zero. */
    std::uint16_t reservedAuxiliaryWordD;
    /** Uninterpreted auxiliary property E, defaulting to zero. */
    std::uint16_t reservedAuxiliaryWordE;
    /** Unrecovered registration bytes cleared by default initialisation. */
    std::uint8_t unknownBytes[22]; /* +0x26 */
    /**
     * Requested handle truncated to 16 bits, with 1 replacing the default and 0xffffffff
     * allocating a new handle.
     */
    std::uint32_t requestedHandle;
};

#endif /* COASTLINE_EAR_ENVIRONMENT_H */
