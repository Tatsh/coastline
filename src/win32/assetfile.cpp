#include <cstdio>

#include "model.h"
#include "win32/boot/app.h"
#include "win32/fileio.h"
#include "win32/podarchive.h"
#include "win32/render.h"

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
    Model *model =
        static_cast<Model *>(g_BossRallyPodArchive.PodArchive::LoadPodByName(source, dst));
    SetPointerRelocationBase(0, reinterpret_cast<unsigned>(model));
    RelocateModelPointers(model);
    return model;
}
