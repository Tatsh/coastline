#include "common/assetdma.h"

void InitDmaRequest(DecompressJob *job, u8 *buffer) {
    job->dst = NULL;
    job->buffer = buffer;
}
