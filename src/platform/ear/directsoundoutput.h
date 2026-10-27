#ifndef COASTLINE_EAR_DIRECTSOUNDOUTPUT_H
#define COASTLINE_EAR_DIRECTSOUNDOUTPUT_H

#include <cstdint>

#include <dsound.h>
#include <windows.h>

class EarDirectSoundOutput {
public:
    /** @ghidraAddress EAR: 0x1001024e */
    EarDirectSoundOutput();
    /** @ghidraAddress EAR: 0x10010593 */
    ~EarDirectSoundOutput();
    /** @ghidraAddress EAR: 0x10010630 */
    IDirectSound *GetDirectSoundInterface();

private:
    void FailInitialization(std::uint32_t error);

    IDirectSound *device;
    IDirectSoundBuffer *primaryBuffer;
};

/** @ghidraAddress EAR: 0x1003b99c */
extern EarDirectSoundOutput *g_pEarDirectSoundOutput;

/** @ghidraAddress EAR: 0x10010146 */
void ShutdownDirectSoundOutput(void);
/** @ghidraAddress EAR: 0x1001018d */
int InitializeDirectSoundOutput(void);

#endif
