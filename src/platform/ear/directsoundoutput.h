#ifndef COASTLINE_EAR_DIRECTSOUNDOUTPUT_H
#define COASTLINE_EAR_DIRECTSOUNDOUTPUT_H

#include <dsound.h>
#include <windows.h>

class EarDirectSoundOutput {
public:
    /**
     * @ghidraAddress EAR: 0x1001024e
     * @stub
     */
    EarDirectSoundOutput();
    /**
     * @ghidraAddress EAR: 0x10010593
     * @stub
     */
    ~EarDirectSoundOutput();
    /** @ghidraAddress EAR: 0x10010630 */
    IDirectSound *GetDirectSoundInterface();

private:
    IDirectSound *device;
    IDirectSoundBuffer *primaryBuffer;
};

/** @ghidraAddress EAR: 0x1003b99c */
extern EarDirectSoundOutput *g_pEarDirectSoundOutput;

#endif
