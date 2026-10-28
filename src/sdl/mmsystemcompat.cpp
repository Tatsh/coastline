#include "mmsystemcompat.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

struct MemoryFile {
    std::vector<char> bytes;
    std::size_t position = 0;
};

MemoryFile *FromHandle(HMMIO file) {
    return static_cast<MemoryFile *>(file);
}

bool ReadWord(MemoryFile &file, DWORD *value) {
    if (file.position > file.bytes.size() || file.bytes.size() - file.position < sizeof(*value)) {
        return false;
    }
    std::memcpy(value, file.bytes.data() + file.position, sizeof(*value));
    file.position += sizeof(*value);
    return true;
}

// Read the header of the chunk at the position, and the form type of a RIFF or list chunk.
bool ReadChunkHeader(MemoryFile &file, MMCKINFO *chunk) {
    if (!ReadWord(file, &chunk->ckid) || !ReadWord(file, &chunk->cksize)) {
        return false;
    }
    chunk->dwDataOffset = static_cast<DWORD>(file.position);
    chunk->fccType = 0;
    if (chunk->ckid == FOURCC_RIFF || chunk->ckid == FOURCC_LIST) {
        return ReadWord(file, &chunk->fccType);
    }
    return true;
}

} // namespace

extern "C" {

HMMIO mmioOpenA(LPSTR path, MMIOINFO *info, DWORD flags) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return nullptr;
    }
    auto *file = new MemoryFile;
    file->bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return file;
}

UINT mmioDescend(HMMIO handle, MMCKINFO *chunk, const MMCKINFO *parent, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    std::size_t end = parent != nullptr ? parent->dwDataOffset + parent->cksize : file.bytes.size();
    if (!(flags & MMIO_FINDCHUNK)) {
        return ReadChunkHeader(file, chunk) ? 0 : MMIOERR_CHUNKNOTFOUND;
    }
    FOURCC wanted = chunk->ckid;
    while (file.position < end) {
        if (!ReadChunkHeader(file, chunk)) {
            return MMIOERR_CHUNKNOTFOUND;
        }
        if (chunk->ckid == wanted) {
            return 0;
        }
        file.position = chunk->dwDataOffset + ((chunk->cksize + 1) & ~DWORD{1});
    }
    return MMIOERR_CHUNKNOTFOUND;
}

UINT mmioAscend(HMMIO handle, MMCKINFO *chunk, UINT flags) {
    FromHandle(handle)->position = chunk->dwDataOffset + ((chunk->cksize + 1) & ~DWORD{1});
    return 0;
}

LONG mmioRead(HMMIO handle, HPSTR buffer, LONG size) {
    MemoryFile &file = *FromHandle(handle);
    if (size < 0 || file.position > file.bytes.size()) {
        return -1;
    }
    std::size_t count = std::min(static_cast<std::size_t>(size), file.bytes.size() - file.position);
    std::memcpy(buffer, file.bytes.data() + file.position, count);
    file.position += count;
    return static_cast<LONG>(count);
}

LONG mmioSeek(HMMIO handle, LONG offset, int origin) {
    MemoryFile &file = *FromHandle(handle);
    long long base = origin == SEEK_CUR ? static_cast<long long>(file.position) :
                     origin == SEEK_END ? static_cast<long long>(file.bytes.size()) :
                                          0;
    long long position = base + offset;
    if (position < 0 || position > static_cast<long long>(file.bytes.size())) {
        return -1;
    }
    file.position = static_cast<std::size_t>(position);
    return static_cast<LONG>(position);
}

UINT mmioGetInfo(HMMIO handle, MMIOINFO *info, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    *info = {};
    info->pchBuffer = file.bytes.data();
    info->pchNext = file.bytes.data() + std::min(file.position, file.bytes.size());
    info->pchEndRead = file.bytes.data() + file.bytes.size();
    info->lDiskOffset = static_cast<LONG>(file.bytes.size());
    return 0;
}

UINT mmioAdvance(HMMIO handle, MMIOINFO *info, UINT flags) {
    return 0;
}

UINT mmioSetInfo(HMMIO handle, const MMIOINFO *info, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    file.position = static_cast<std::size_t>(info->pchNext - file.bytes.data());
    return 0;
}

UINT mmioClose(HMMIO handle, UINT flags) {
    delete FromHandle(handle);
    return 0;
}

HGLOBAL GlobalAlloc(UINT flags, size_t size) {
    // A block of no bytes is still a block, as GlobalAlloc() returns one.
    return std::calloc(size != 0 ? size : 1, 1);
}

HGLOBAL GlobalFree(HGLOBAL memory) {
    std::free(memory);
    return nullptr;
}

} // extern "C"
