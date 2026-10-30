#include <cstdio>

#include "common/fileio.h"
#include "common/model.h"
#include "common/podarchive.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/app.h"
#include "win32/render.h"
#else
#include "common/fatalerror.h"
#include "common/relocation.h"
#endif

void ReadEntireFile(void *buffer, const char *path, int size) {
    if (!CHK_FileExists(path)) {
        char message[512];
        std::sprintf(message, "File %s missing", path);
        ShowFatalErrorScreen(message);
    }
    GameFile *file = CHK_FReadOpen(path);
    if (size < 0) {
        size = GetFileLength(file);
    }
    CHK_FRead(buffer, 1, size, file);
    CHK_FClose(file);
}

Model *DecompressModel(void *dst, const char *source) {
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    // The SDL game reads the model from the import of BossRally.pod. As LoadPodByName(), the read
    // trusts the buffer to fit the entry.
    if (!ReadImportedPodEntry(source, dst)) {
        FatalErrorWithMessage("GetNumForName: %s not found!", source);
    }
    Model *model = static_cast<Model *>(dst);
#else
    Model *model =
        static_cast<Model *>(g_BossRallyPodArchive.PodArchive::LoadPodByName(source, dst));
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SetPointerRelocationBase(0, reinterpret_cast<unsigned>(model));
#else
    // The offsets of the model are from its start, and the start is its offset from the anchor.
    SetPointerRelocationBase(0, coastline::EncodePtr32(model));
#endif
    RelocateModelPointers(model);
    return model;
}
