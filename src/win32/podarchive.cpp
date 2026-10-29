#include "common/podarchive.h"

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include "win32/app.h"

PodArchive g_BossRallyPodArchive;

namespace {

/**
 * The entries of the pod file being written.
 *
 * @ghidraAddress BRD3D: 0x1022b358
 * @ghidraAddress BRGlide: 0x10226e88
 */
PodEntry g_aPodArchiveEntries[POD_WRITER_ENTRY_COUNT];

/**
 * The number of entries of g_aPodArchiveEntries in use.
 *
 * @ghidraAddress BRD3D: 0x10277358
 * @ghidraAddress BRGlide: 0x10272e88
 */
int g_nPodArchiveEntryCount;

/**
 * The pod file being written.
 *
 * @ghidraAddress BRD3D: 0x1027735c
 * @ghidraAddress BRGlide: 0x10272e8c
 */
FILE *g_pPodArchiveFile;

} // namespace

// The size of the buffer of a message of FatalErrorWithMessage().
#define FATAL_MESSAGE_SIZE 0x400
// The characters of the signature of a pod file that are checked.
#define POD_SIGNATURE_LENGTH 3

void FatalErrorWithMessage(const char *pszFormat, ...) {
    // The message is formatted into memory that is not freed, and is not shown.
    char *pszMessage = new char[FATAL_MESSAGE_SIZE];
    va_list args;

    va_start(args, pszFormat);
    vsprintf(pszMessage, pszFormat, args);
    va_end(args);
    exit(1);
}

void PodFileHelper::ExtractFileName(const char *pszPath, char *pszName) {
    const char *pszStart = pszPath + strlen(pszPath) - 1;

    while (pszStart != pszPath && pszStart[-1] != '\\') {
        pszStart--;
    }
    strcpy(pszName, pszStart);
}

FILE *PodFileHelper::FileOpenBinaryWriteOrFatal(const char *pszPath) {
    FILE *pFile = fopen(pszPath, "wb");

    if (pFile == nullptr) {
        FatalErrorWithMessage("Error opening %s: %s", pszPath, strerror(errno));
    }
    return pFile;
}

FILE *PodFileHelper::FileOpenBinaryReadOrFatal(const char *pszPath) {
    FILE *pFile = fopen(pszPath, "rb");

    if (pFile == nullptr) {
        FatalErrorWithMessage("Error opening %s: %s", pszPath, strerror(errno));
    }
    return pFile;
}

void PodFileHelper::FileReadExactOrFatal(FILE *pFile, void *pBuffer, unsigned dwSize) {
    if (fread(pBuffer, 1, dwSize, pFile) != dwSize) {
        FatalErrorWithMessage("File read failure");
    }
}

void PodFileHelper::FileWriteChecked(FILE *pFile, const void *pBuffer, unsigned dwSize) {
    if (fwrite(pBuffer, 1, dwSize, pFile) != dwSize) {
        FatalErrorWithMessage("File read failure");
    }
}

// Clear the header, the directory, the file, and the path of an archive.
inline void PodArchive::ClearPodArchive() {
    pEntries = nullptr;
    pFile = nullptr;
    dwDirectorySize = 0;
    memset(&header, 0, sizeof(header));
    memset(szPath, 0, sizeof(szPath));
}

PodFileHelper::PodFileHelper() {
}

PodArchive::PodArchive() {
    ClearPodArchive();
}

PodArchive::~PodArchive() {
}

void PodArchive::CleanupName(const char *pszPath, char *pszName) {
    unsigned i;

    helper.ExtractFileName(pszPath, pszName);
    if (strlen(pszName) > POD_ENTRY_NAME_SIZE) {
        FatalErrorWithMessage("CleanupName: Name is greater than 64 bytes. Memory Corrupted!");
    }
    for (i = 0; i < POD_ENTRY_NAME_SIZE && pszName[i] != '\0'; i++) {
        pszName[i] = static_cast<char>(toupper(pszName[i]));
    }
    if (i < POD_ENTRY_NAME_SIZE) {
        memset(pszName + i, 0, POD_ENTRY_NAME_SIZE - i);
    }
}

int PodArchive::FindPodIndexByName(const char *pszPath) {
    char szName[POD_ENTRY_NAME_SIZE];

    CleanupName(pszPath, szName);
    for (unsigned i = 0; i < static_cast<unsigned>(header.nEntryCount); i++) {
        if (memcmp(pEntries[i].szName, szName, sizeof(szName)) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int PodArchive::GetPodIndexByName(const char *pszPath) {
    int nIndex = FindPodIndexByName(pszPath);

    if (nIndex == -1) {
        FatalErrorWithMessage("GetNumForName: %s not found!", pszPath);
        return -1;
    }
    return nIndex;
}

unsigned PodArchive::GetPodLength(unsigned nIndex) {
    if (nIndex >= static_cast<unsigned>(header.nEntryCount)) {
        FatalErrorWithMessage("GetPodLength: %i >= m_cNumPods", nIndex);
    }
    return pEntries[nIndex].dwSize;
}

void PodArchive::ReadPodBytesFromFile(unsigned nIndex, void *pBuffer) {
    if (nIndex >= static_cast<unsigned>(header.nEntryCount)) {
        FatalErrorWithMessage("ReadPod: %i >= cNumPods", nIndex);
    }
    fseek(pFile, pEntries[nIndex].dwOffset, SEEK_SET);
    helper.FileReadExactOrFatal(pFile, pBuffer, pEntries[nIndex].dwSize);
}

void *PodArchive::LoadPodByName(const char *pszPath, void *pBuffer) {
    return LoadPodEntryByIndex(GetPodIndexByName(pszPath), pBuffer);
}

void *PodArchive::LoadPodEntryByIndex(unsigned nIndex, void *pBuffer) {
    if (nIndex >= static_cast<unsigned>(header.nEntryCount)) {
        FatalErrorWithMessage("LoadPod: %i >= m_cNumPods", nIndex);
    }
    ReadPodBytesFromFile(nIndex, pBuffer);
    return pBuffer;
}

void *PodArchive::AllocAndLoadPodByName(const char *pszPath) {
    return AllocAndLoadPodByIndex(GetPodIndexByName(pszPath));
}

void *PodArchive::AllocAndLoadPodByIndex(unsigned nIndex) {
    void *pData;

    if (nIndex >= static_cast<unsigned>(header.nEntryCount)) {
        FatalErrorWithMessage("LoadPod: %i >= m_cNumPods", nIndex);
    }
    pData = ::operator new(GetPodLength(nIndex));
    ReadPodBytesFromFile(nIndex, pData);
    return pData;
}

void PodArchive::OpenPodFile() {
    pFile = helper.FileOpenBinaryReadOrFatal(szPath);
    helper.FileReadExactOrFatal(pFile, &header, sizeof(header));
    if (strncmp(header.szSignature, "POD", POD_SIGNATURE_LENGTH) != 0) {
        FatalErrorWithMessage("%s is not a valid POD file", szPath);
    }
    dwDirectorySize = header.nEntryCount * sizeof(PodEntry);
    pEntries = static_cast<PodEntry *>(::operator new(dwDirectorySize));
    fseek(pFile, header.dwDirectoryOffset, SEEK_SET);
    helper.FileReadExactOrFatal(pFile, pEntries, dwDirectorySize);
}

void PodArchive::CreatePodFile(const char *pszPath) {
    g_pPodArchiveFile = helper.FileOpenBinaryWriteOrFatal(pszPath);
    fseek(g_pPodArchiveFile, sizeof(PodHeader), SEEK_SET);
    memset(g_aPodArchiveEntries, 0, sizeof(g_aPodArchiveEntries));
    g_nPodArchiveEntryCount = 0;
}

void PodArchive::AddPodEntry(
    const char *pszPath, const void *pData, unsigned dwSize, char bAttrA, char bAttrB) {
    PodEntry *pEntry = &g_aPodArchiveEntries[g_nPodArchiveEntryCount++];

    pEntry->dwOffset = 0;
    helper.ExtractFileName(pszPath, pEntry->szName);
    if (strlen(pEntry->szName) > POD_ENTRY_NAME_SIZE) {
        FatalErrorWithMessage("Add: Name is too long to be a pod name.");
    }
    _strupr(pEntry->szName);
    pEntry->dwOffset = ftell(g_pPodArchiveFile);
    pEntry->bAttrA = static_cast<unsigned char>(bAttrA);
    pEntry->dwSize = dwSize;
    pEntry->bAttrB = static_cast<unsigned char>(bAttrB);
    helper.FileWriteChecked(g_pPodArchiveFile, pData, dwSize);
}

void PodArchive::FinalizePodFile() {
    PodHeader newHeader;
    long dwDirectoryOffset = ftell(g_pPodArchiveFile);

    helper.FileWriteChecked(
        g_pPodArchiveFile, g_aPodArchiveEntries, g_nPodArchiveEntryCount * sizeof(PodEntry));
    // The fourth byte of the signature is not set, as in the original.
    newHeader.szSignature[0] = 'P';
    newHeader.szSignature[1] = 'O';
    newHeader.szSignature[2] = 'D';
    newHeader.nVersion = POD_FILE_VERSION;
    newHeader.nEntryCount = g_nPodArchiveEntryCount;
    newHeader.dwDirectoryOffset = dwDirectoryOffset;
    fseek(g_pPodArchiveFile, 0, SEEK_SET);
    helper.FileWriteChecked(g_pPodArchiveFile, &newHeader, sizeof(newHeader));
    fclose(g_pPodArchiveFile);
}

void PodArchive::ClosePodFile() {
    if (pFile != nullptr) {
        fclose(pFile);
    }
    if (pEntries != nullptr) {
        ::operator delete(pEntries);
    }
    ClearPodArchive();
}

void PodArchive::SetFilePath(const char *pszPath) {
    if (pszPath != nullptr) {
        strcpy(szPath, pszPath);
    }
}
