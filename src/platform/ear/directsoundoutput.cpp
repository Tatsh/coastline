#include "directsoundoutput.h"

IDirectSound *EarDirectSoundOutput::GetDirectSoundInterface() {
    return device;
}
