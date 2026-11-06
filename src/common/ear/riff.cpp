// The reading of the wave files of EAR in the SDL game, in place of the multimedia I/O of Windows.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "wavechunks.h"

/** A file opened from a path, read through a stream, or a file in memory. */
struct EarRiffFile {
    std::ifstream stream;         /*!< The file of a path. */
    const char *memory = nullptr; /*!< The file in memory, or a null pointer for a path. */
    LONG size = 0;                /*!< The bytes of the file in memory. */
    LONG position = 0;            /*!< The position in the file in memory. */
};

namespace {

// The identifier of a list chunk. A list chunk has a form type, as a RIFF chunk does.
constexpr DWORD EAR_LIST_ID = 0x5453494c;

// The size of the identifier and the size that start each chunk.
constexpr LONG EAR_RIFF_HEADER_BYTES = 8;

LONG Tell(EarRiffFile *file) {
    if (file->memory) {
        return file->position;
    }
    file->stream.clear();
    return static_cast<LONG>(file->stream.tellg());
}

} // namespace

extern "C" {

EarRiffFile *EarRiffOpenPath(const char *path) {
    auto *file = new EarRiffFile;
    file->stream.open(std::filesystem::path(path), std::ios::binary);
    if (!file->stream) {
        delete file;
        return nullptr;
    }
    return file;
}

EarRiffFile *EarRiffOpenMemory(const void *data, LONG size) {
    auto *file = new EarRiffFile;
    file->memory = static_cast<const char *>(data);
    file->size = size;
    return file;
}

void EarRiffClose(EarRiffFile *file) {
    delete file;
}

LONG EarRiffRead(EarRiffFile *file, void *buffer, LONG bytes) {
    if (bytes < 0) {
        return -1;
    }
    if (file->memory) {
        LONG available = file->size - file->position;
        LONG count = bytes < available ? bytes : available;
        if (count <= 0) {
            return 0;
        }
        std::memcpy(buffer, file->memory + file->position, static_cast<std::size_t>(count));
        file->position += count;
        return count;
    }
    file->stream.clear();
    file->stream.read(static_cast<char *>(buffer), bytes);
    if (file->stream.bad()) {
        return -1;
    }
    return static_cast<LONG>(file->stream.gcount());
}

LONG EarRiffSeek(EarRiffFile *file, LONG offset, int origin) {
    if (file->memory) {
        LONG base = origin == SEEK_SET ? 0 : origin == SEEK_CUR ? file->position : file->size;
        LONG target = base + offset;
        if (target < 0) {
            return -1;
        }
        file->position = target;
        return target;
    }
    std::ios::seekdir direction = origin == SEEK_SET ? std::ios::beg :
                                  origin == SEEK_CUR ? std::ios::cur :
                                                       std::ios::end;
    file->stream.clear();
    if (!file->stream.seekg(offset, direction)) {
        return -1;
    }
    return Tell(file);
}

UINT EarRiffDescend(EarRiffFile *file,
                    EarRiffChunk *chunk,
                    const EarRiffChunk *parent,
                    UINT flags) {
    DWORD wanted = chunk->ckid;
    LONG end = parent ? static_cast<LONG>(parent->dwDataOffset + parent->cksize) : -1;
    for (;;) {
        LONG start = Tell(file);
        if (end >= 0 && start + EAR_RIFF_HEADER_BYTES > end) {
            return 1;
        }
        DWORD header[2];
        if (EarRiffRead(file, header, sizeof(header)) != static_cast<LONG>(sizeof(header))) {
            return 1;
        }
        chunk->ckid = header[0];
        chunk->cksize = header[1];
        chunk->dwDataOffset = static_cast<DWORD>(start + EAR_RIFF_HEADER_BYTES);
        chunk->dwFlags = 0;
        if (chunk->ckid == EAR_RIFF_ID || chunk->ckid == EAR_LIST_ID) {
            if (EarRiffRead(file, &chunk->fccType, sizeof(chunk->fccType)) !=
                static_cast<LONG>(sizeof(chunk->fccType))) {
                return 1;
            }
        }
        if (!(flags & EAR_RIFF_FIND_CHUNK) || chunk->ckid == wanted) {
            return 0;
        }
        LONG next = static_cast<LONG>(chunk->dwDataOffset + chunk->cksize + (chunk->cksize & 1));
        if (EarRiffSeek(file, next, SEEK_SET) == -1) {
            return 1;
        }
    }
}

UINT EarRiffAscend(EarRiffFile *file, const EarRiffChunk *chunk) {
    LONG next = static_cast<LONG>(chunk->dwDataOffset + chunk->cksize + (chunk->cksize & 1));
    return EarRiffSeek(file, next, SEEK_SET) == -1;
}

} // extern "C"
