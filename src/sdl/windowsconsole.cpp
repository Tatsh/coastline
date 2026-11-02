#include "windowsconsole.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>

#include <io.h>
#include <windows.h>

namespace coastline {

void AttachParentConsole() {
    struct Stream {
        DWORD identifier;
        FILE *file;
        const char *device;
        const char *mode;
        HANDLE original;
        bool redirected;
    };
    std::array streams{Stream{STD_INPUT_HANDLE, stdin, "CONIN$", "r", nullptr, false},
                       Stream{STD_OUTPUT_HANDLE, stdout, "CONOUT$", "w", nullptr, false},
                       Stream{STD_ERROR_HANDLE, stderr, "CONOUT$", "w", nullptr, false}};
    for (auto &stream : streams) {
        stream.original = GetStdHandle(stream.identifier);
        stream.redirected = stream.original && stream.original != INVALID_HANDLE_VALUE &&
                            GetFileType(stream.original) != FILE_TYPE_UNKNOWN;
    }
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        return;
    }
    for (const auto &stream : streams) {
        if (stream.redirected) {
            SetStdHandle(stream.identifier, stream.original);
        }
        const int descriptor = _fileno(stream.file);
        if (descriptor < 0 || _get_osfhandle(descriptor) < 0) {
            HANDLE duplicate = INVALID_HANDLE_VALUE;
            if (stream.redirected) {
                DuplicateHandle(GetCurrentProcess(),
                                stream.original,
                                GetCurrentProcess(),
                                &duplicate,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS);
            }
            FILE *reopened = nullptr;
            (void)freopen_s(&reopened, stream.device, stream.mode, stream.file);
            if (duplicate != INVALID_HANDLE_VALUE) {
                const int flags =
                    _O_TEXT | (stream.identifier == STD_INPUT_HANDLE ? _O_RDONLY : _O_WRONLY);
                const int redirected =
                    _open_osfhandle(reinterpret_cast<std::intptr_t>(duplicate), flags);
                if (redirected >= 0) {
                    if (reopened) {
                        (void)_dup2(redirected, _fileno(reopened));
                    }
                    _close(redirected);
                } else {
                    CloseHandle(duplicate);
                }
                SetStdHandle(stream.identifier, stream.original);
            }
        }
    }
}

} // namespace coastline
