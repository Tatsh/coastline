// The file of MFC that EAR writes the music of a MIDI device with, as MFC 4.2 implements it over
// the files of Windows.

#include <windows.h>

#include "ear/mfc.h"

namespace {

// The handle of a file that is not open.
constexpr UINT NO_FILE = static_cast<UINT>(-1);

} // namespace

CFile::CFile() : file(NO_FILE), closeOnDelete(FALSE), fileName(nullptr) {
}

CFile::~CFile() {
    if (file != NO_FILE && closeOnDelete) {
        Close();
    }
}

BOOL CFile::Open(LPCSTR name, UINT openFlags, CFileException *error) {
    closeOnDelete = FALSE;
    file = NO_FILE;
    DWORD access = (openFlags & modeWrite) ? GENERIC_WRITE : GENERIC_READ;
    DWORD creation = (openFlags & modeCreate) ? CREATE_ALWAYS : OPEN_EXISTING;
    HANDLE handle = CreateFileA(name,
                                access,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr,
                                creation,
                                FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    file = static_cast<UINT>(reinterpret_cast<UINT_PTR>(handle));
    closeOnDelete = TRUE;
    return TRUE;
}

void CFile::Write(const void *buffer, UINT count) {
    DWORD written;
    WriteFile(
        reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(file)), buffer, count, &written, nullptr);
}

void CFile::Close() {
    if (file != NO_FILE) {
        CloseHandle(reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(file)));
    }
    file = NO_FILE;
    closeOnDelete = FALSE;
}
