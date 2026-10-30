#include "common/damagetexture.h"

#include <cstring>

int g_anDamageTextureEntries[DAMAGE_TEXTURE_COUNT];

void ClearDamageTextureEntries(void) {
    memset(g_anDamageTextureEntries, 0, sizeof(g_anDamageTextureEntries));
}

void CopyDamageTextureEntries(int *pnEntries) {
    memcpy(pnEntries, g_anDamageTextureEntries, sizeof(g_anDamageTextureEntries));
}
