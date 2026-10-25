#include "platform/win/wavread.h"

#include <string.h>

int WaveOpenFile(const char *pszPath, HMMIO *phmmio, WAVEFORMATEX **ppFormat, MMCKINFO *pckRiff) {
    HMMIO hmmio;
    MMCKINFO ckFormat;
    PCMWAVEFORMAT pcmFormat;
    WORD wExtraSize;
    int nError;

    *ppFormat = nullptr;
    hmmio = mmioOpenA((LPSTR)pszPath, nullptr, MMIO_ALLOCBUF | MMIO_READ);
    if (hmmio == nullptr) {
        nError = WAVE_ERROR_CANNOT_OPEN;
        goto error;
    }
    nError = (int)mmioDescend(hmmio, pckRiff, nullptr, 0);
    if (nError != 0) {
        goto error;
    }
    if (pckRiff->ckid != FOURCC_RIFF || pckRiff->fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        nError = WAVE_ERROR_NOT_WAVE_FILE;
        goto error;
    }
    ckFormat.ckid = mmioFOURCC('f', 'm', 't', ' ');
    nError = (int)mmioDescend(hmmio, &ckFormat, pckRiff, MMIO_FINDCHUNK);
    if (nError != 0) {
        goto error;
    }
    if (ckFormat.cksize < sizeof(PCMWAVEFORMAT)) {
        nError = WAVE_ERROR_NOT_WAVE_FILE;
        goto error;
    }
    if (mmioRead(hmmio, (HPSTR)&pcmFormat, sizeof(pcmFormat)) != (LONG)sizeof(pcmFormat)) {
        nError = WAVE_ERROR_CANNOT_READ;
        goto error;
    }
    if (pcmFormat.wf.wFormatTag == WAVE_FORMAT_PCM) {
        wExtraSize = 0;
    } else if (mmioRead(hmmio, (HPSTR)&wExtraSize, sizeof(wExtraSize)) !=
               (LONG)sizeof(wExtraSize)) {
        nError = WAVE_ERROR_CANNOT_READ;
        goto error;
    }
    *ppFormat = (WAVEFORMATEX *)GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX) + wExtraSize);
    if (*ppFormat == nullptr) {
        nError = WAVE_ERROR_MEMORY;
        goto error;
    }
    memcpy(*ppFormat, &pcmFormat, sizeof(pcmFormat));
    (*ppFormat)->cbSize = wExtraSize;
    if (wExtraSize != 0 &&
        mmioRead(hmmio, (HPSTR)(*ppFormat + 1), wExtraSize) != (LONG)wExtraSize) {
        nError = WAVE_ERROR_NOT_WAVE_FILE;
        goto error;
    }
    nError = (int)mmioAscend(hmmio, &ckFormat, 0);
    if (nError != 0) {
        goto error;
    }
    *phmmio = hmmio;
    return nError;

error:
    if (*ppFormat != nullptr) {
        GlobalFree(*ppFormat);
        *ppFormat = nullptr;
    }
    if (hmmio != nullptr) {
        mmioClose(hmmio, 0);
        hmmio = nullptr;
    }
    *phmmio = hmmio;
    return nError;
}

int WaveStartDataRead(HMMIO *phmmio, MMCKINFO *pckData, MMCKINFO *pckRiff) {
    // The result of the seek is not checked.
    mmioSeek(*phmmio, (LONG)(pckRiff->dwDataOffset + sizeof(FOURCC)), SEEK_SET);
    pckData->ckid = mmioFOURCC('d', 'a', 't', 'a');
    return (int)mmioDescend(*phmmio, pckData, pckRiff, MMIO_FINDCHUNK);
}

int WaveReadFile(HMMIO hmmio, DWORD dwRead, BYTE *pDest, MMCKINFO *pckData, DWORD *pdwActualRead) {
    MMIOINFO info;
    DWORD dwDataSize;
    int nError;

    // The comparison is assigned rather than the result, as in the original.
    nError = mmioGetInfo(hmmio, &info, 0) != 0;
    if (nError != 0) {
        goto error;
    }
    dwDataSize = dwRead;
    if (dwDataSize > pckData->cksize) {
        dwDataSize = pckData->cksize;
    }
    pckData->cksize -= dwDataSize;
    for (DWORD i = 0; i < dwDataSize; i++) {
        if (info.pchNext == info.pchEndRead) {
            nError = (int)mmioAdvance(hmmio, &info, MMIO_READ);
            if (nError != 0) {
                goto error;
            }
            if (info.pchNext == info.pchEndRead) {
                nError = WAVE_ERROR_CORRUPT_WAVE_FILE;
                goto error;
            }
        }
        pDest[i] = (BYTE)*info.pchNext++;
    }
    nError = (int)mmioSetInfo(hmmio, &info, 0);
    if (nError != 0) {
        goto error;
    }
    *pdwActualRead = dwDataSize;
    return nError;

error:
    *pdwActualRead = 0;
    return nError;
}

int WaveLoadFile(const char *pszPath,
                 DWORD *pdwSize,
                 DWORD *pdwSamples,
                 WAVEFORMATEX **ppFormat,
                 void **ppData) {
    HMMIO hmmio;
    MMCKINFO ckRiff;
    MMCKINFO ckData;
    DWORD dwActualRead;
    int nError;

    *ppData = nullptr;
    *ppFormat = nullptr;
    *pdwSize = 0;
    nError = WaveOpenFile(pszPath, &hmmio, ppFormat, &ckRiff);
    if (nError != 0) {
        goto error;
    }
    nError = WaveStartDataRead(&hmmio, &ckData, &ckRiff);
    if (nError != 0) {
        goto error;
    }
    *ppData = (void *)GlobalAlloc(GMEM_FIXED, ckData.cksize);
    if (*ppData == nullptr) {
        nError = WAVE_ERROR_MEMORY;
        goto error;
    }
    nError = WaveReadFile(hmmio, ckData.cksize, (BYTE *)*ppData, &ckData, &dwActualRead);
    if (nError != 0) {
        goto error;
    }
    *pdwSize = dwActualRead;
    goto close;

error:
    if (*ppData != nullptr) {
        GlobalFree(*ppData);
        *ppData = nullptr;
    }
    if (*ppFormat != nullptr) {
        GlobalFree(*ppFormat);
        *ppFormat = nullptr;
    }

close:
    if (hmmio != nullptr) {
        mmioClose(hmmio, 0);
    }
    return nError;
}
