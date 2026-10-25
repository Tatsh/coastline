#include <cstring>

#include "platform/win/d3d/renderer.h"

DirectDrawDriver *g_pDirectDrawDriverListHead;

DirectDrawDriver *FindDirectDrawDriverOrDefault(GUID *pGuid) {
    DirectDrawDriver *pPrimary;
    DirectDrawDriver *pDriver = FindDirectDrawDriverByGuid(pGuid, &pPrimary, nullptr);

    return pDriver != nullptr ? pDriver : pPrimary;
}

bool DirectDrawDriver::CompareDirectDrawDriverGuid(const GUID *pGuid) {
    if ((dwFlags & DIRECTDRAW_DRIVER_USABLE) == 0) {
        return false;
    }
    if (pGuid == nullptr) {
        return (dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0;
    }
    return memcmp(&guid, pGuid, sizeof(guid)) == 0;
}

GUID *DirectDrawDriver::GetDirectDrawDriverGuid() {
    return (dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0 ? nullptr : &guid;
}

DirectDrawDriver *
FindDirectDrawDriverByGuid(GUID *pGuid, DirectDrawDriver **ppPrimary, DirectDrawDriver *pFirst) {
    DirectDrawDriver *pDriver = pFirst != nullptr ? pFirst : g_pDirectDrawDriverListHead;

    if (ppPrimary != nullptr) {
        *ppPrimary = pDriver;
    }
    while (pDriver != nullptr) {
        DirectDrawDriver *pNext = pDriver->pNext;

        if (pDriver->CompareDirectDrawDriverGuid(pGuid)) {
            return pDriver;
        }
        if ((pDriver->dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0 && ppPrimary != nullptr) {
            *ppPrimary = pDriver;
        }
        pDriver = pNext;
    }
    return nullptr;
}
