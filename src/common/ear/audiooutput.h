#ifndef COASTLINE_EAR_AUDIOOUTPUT_H
#define COASTLINE_EAR_AUDIOOUTPUT_H

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <cstdint>

#include <dsound.h>
#include <windows.h>

/** Windows DirectSound output with a continuously playing 22050 Hz stereo 16-bit primary buffer. */
class EarDirectSoundOutput {
public:
    /**
     * Initialise COM, a DirectSound device, and its primary buffer using the assigned game window.
     * Failed initialisation records an EAR error and clears the device reference.
     * Failure cleanup uninitialises COM even before successful COM initialisation; any primary
     * buffer remains allocated until destruction.
     * @ghidraAddress EAR: 0x1001024e
     */
    EarDirectSoundOutput();
    /**
     * Stop and release available primary-buffer and device resources, then uninitialise COM.
     * @ghidraAddress EAR: 0x10010593
     */
    ~EarDirectSoundOutput();
    /**
     * @return The borrowed DirectSound interface, valid until destruction, or null after failure.
     * @ghidraAddress EAR: 0x10010630
     */
    IDirectSound *GetDirectSoundInterface();

private:
    void FailInitialization(std::uint32_t error);

    IDirectSound *device;
    IDirectSoundBuffer *primaryBuffer;
};

/**
 * Allocated shared Windows output instance, or null before creation or after shutdown.
 * ShutdownEarAudioOutput() deletes the instance and invalidates borrowed interfaces.
 * @ghidraAddress EAR: 0x1003b99c
 */
/** @cond */
extern EarDirectSoundOutput *g_pEarDirectSoundOutput;
/** @endcond */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Release and clear the shared Windows output instance or close the SDL mixer.
 * @ghidraAddress EAR: 0x10010146
 */
void ShutdownEarAudioOutput(void);
/**
 * Create shared Windows output or open the SDL mixer; existing Windows output must be shut down.
 * @return One when an output interface is available; zero with EAR error 3 on initialisation
 * failure. Native Windows allocation failure is unchecked.
 * @ghidraAddress EAR: 0x1001018d
 */
int InitializeEarAudioOutput(void);
#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_AUDIOOUTPUT_H */
