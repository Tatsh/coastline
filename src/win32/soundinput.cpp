#include "common/soundsampleinput.h"
#include "win32/wavread.h"

int LoadSoundBufferSample(const char *path, SoundBufferNode *node, int) {
    DWORD sampleCount;
    return WaveLoadFile(
        path, &node->dwSampleSize, &sampleCount, &node->pWaveFormat, &node->pSampleData);
}
