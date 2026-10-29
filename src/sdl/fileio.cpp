// The file routines of the PC release on the directories of the SDL game. The behaviour of each
// routine, its messages, and its exits follow src/win32/fileio.c.

#include "win32/fileio.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

#include "sdl/gamefiles.h"
#include "sdl/log.h"
#include "win32/diagnostics.h"

namespace {

// The size of the buffer of a message to the debugger.
constexpr int MESSAGE_SIZE = 1024;

std::filesystem::path g_dataDirectory;
std::filesystem::path g_stateDirectory;

// End the game, as the PC release does on an error of a file. The destructors of the program do
// not run while the threads of the audio and of SDL still run.
[[noreturn]] void ExitOnFileError() {
    coastline::StopGameLog();
    std::_Exit(1);
}

void WriteDebugMessage(const char *message) {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
}

bool SameNameIgnoringCase(const std::string &left, const std::string &right) {
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), [](char a, char b) {
               return std::tolower(static_cast<unsigned char>(a)) ==
                      std::tolower(static_cast<unsigned char>(b));
           });
}

// The PC release names its files as Windows does, with backslashes and in any case. Each part of
// the path matches the entry of the directory of the same name without regard to case.
std::optional<std::filesystem::path> FindExisting(const std::filesystem::path &base,
                                                  const std::filesystem::path &relative) {
    std::filesystem::path current = base;
    for (const auto &part : relative) {
        std::error_code error;
        auto exact = current / part;
        if (std::filesystem::exists(exact, error)) {
            current = exact;
            continue;
        }
        bool found = false;
        for (const auto &entry : std::filesystem::directory_iterator(current, error)) {
            if (SameNameIgnoringCase(entry.path().filename().string(), part.string())) {
                current = entry.path();
                found = true;
                break;
            }
        }
        if (!found) {
            return std::nullopt;
        }
    }
    return current;
}

std::filesystem::path RelativeGamePath(const char *path) {
    std::string text(path);
    // The install directory of the PC release is the state directory, and a data file named under
    // it is looked up again relative to the data directory.
    std::string state = g_stateDirectory.string();
    if (!state.empty() && text.starts_with(state)) {
        text.erase(0, state.size());
    }
    std::replace(text.begin(), text.end(), '\\', '/');
    // A drive or a leading separator from an absolute Windows path is dropped.
    if (text.size() > 1 && text[1] == ':') {
        text.erase(0, 2);
    }
    while (!text.empty() && text.front() == '/') {
        text.erase(0, 1);
    }
    return std::filesystem::path(text);
}

} // namespace

namespace coastline {

void SetGameDirectories(const std::filesystem::path &data, const std::filesystem::path &state) {
    g_dataDirectory = data;
    g_stateDirectory = state;
}

std::optional<std::filesystem::path> FindGameFile(const char *path) {
    // A path under the state directory, which the install directory of the PC release names, is
    // already complete.
    std::filesystem::path complete(path);
    std::error_code error;
    if (complete.is_absolute() && std::filesystem::exists(complete, error)) {
        return complete;
    }
    auto relative = RelativeGamePath(path);
    if (auto saved = FindExisting(g_stateDirectory, relative)) {
        return saved;
    }
    return FindExisting(g_dataDirectory, relative);
}

std::filesystem::path WritableGameFile(const char *path) {
    std::filesystem::path complete(path);
    if (complete.is_absolute()) {
        return complete;
    }
    auto relative = RelativeGamePath(path);
    if (auto existing = FindExisting(g_stateDirectory, relative)) {
        return *existing;
    }
    auto target = g_stateDirectory / relative;
    std::error_code error;
    std::filesystem::create_directories(target.parent_path(), error);
    return target;
}

} // namespace coastline

extern "C" {

int g_bVerboseFileTrace;

#ifdef ENABLE_PATCHES
void WriteApplicationDiagnostic(const char *message) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
}
#endif

long GetFileLength(GameFile *pFile) {
    long nPosition = std::ftell(pFile->pFile);
    std::fseek(pFile->pFile, 0, SEEK_END);
    long nLength = std::ftell(pFile->pFile);
    std::fseek(pFile->pFile, nPosition, SEEK_SET);
    return nLength;
}

GameFile *CHK_FReadOpen(const char *pszPath) {
    char szMessage[MESSAGE_SIZE];
    auto *pFile =
        static_cast<GameFile *>(CHK_AllocateMemory(sizeof(GameFile), "CHK_FReadOpen():pfil"));
    pFile->pszPath =
        static_cast<char *>(CHK_AllocateMemory(std::strlen(pszPath) + 1, "CHK_FReadOpen():szName"));
    std::strcpy(pFile->pszPath, pszPath);
    if (g_bVerboseFileTrace) {
        std::snprintf(szMessage, sizeof(szMessage), "CHK_FReadOpen(%s)\n", pFile->pszPath);
        WriteDebugMessage(szMessage);
    }
    auto found = coastline::FindGameFile(pszPath);
    pFile->pFile = found ? std::fopen(found->string().c_str(), "rb") : nullptr;
    if (pFile->pFile == nullptr) {
        if (found) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "%s cannot be opened: %s",
                         found->string().c_str(),
                         std::strerror(errno));
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "%s is in neither %s nor %s.",
                         pszPath,
                         g_stateDirectory.string().c_str(),
                         g_dataDirectory.string().c_str());
        }
        std::snprintf(szMessage,
                      sizeof(szMessage),
                      "CHK_FReadOpen(): error opening file %s.\n",
                      pFile->pszPath);
        // The PC release writes the message to c:\RallyError.txt, which is in the state directory
        // here.
        if (FILE *pErrorFile =
                std::fopen(coastline::WritableGameFile("RallyError.txt").string().c_str(), "w")) {
            std::fputs(szMessage, pErrorFile);
            std::fclose(pErrorFile);
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    return pFile;
}

GameFile *CHK_FWriteOpen(const char *path, const char *mode) {
    char message[MESSAGE_SIZE];
    auto *file =
        static_cast<GameFile *>(CHK_AllocateMemory(sizeof(GameFile), "CHK_FWriteOpen():pfil"));
    // The PC release names the read routine for the path of a write.
    file->pszPath =
        static_cast<char *>(CHK_AllocateMemory(std::strlen(path) + 1, "CHK_FReadOpen():szName"));
    std::strcpy(file->pszPath, path);
    if (g_bVerboseFileTrace) {
        std::snprintf(message, sizeof(message), "CHK_FWriteOpen(%s)\n", file->pszPath);
        WriteDebugMessage(message);
    }
    file->pFile = std::fopen(coastline::WritableGameFile(path).string().c_str(), mode);
    if (file->pFile == nullptr) {
        std::snprintf(
            message, sizeof(message), "CHK_FWriteOpen(): error creating file %s.\n", file->pszPath);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
        ExitOnFileError();
    }
    return file;
}

void CHK_FPutS(const char *text, GameFile *file) {
    char message[MESSAGE_SIZE];
    if (std::fputs(text, file->pFile) == EOF) {
        std::snprintf(
            message, sizeof(message), "CHK_FPutS(): error writing file %s.\n", file->pszPath);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
        ExitOnFileError();
    }
}

bool FCHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];
    if (nSize * nCount == 0) {
        return true;
    }
    size_t nRead = std::fread(pBuffer, nSize, nCount, pFile->pFile);
    if (nRead == 0) {
        return false;
    }
    if (nRead != nCount) {
        std::snprintf(szMessage,
                      sizeof(szMessage),
                      "FCHK_FRead(): trying to read %d bytes, but got only %d bytes.\n",
                      static_cast<int>(nSize * nCount),
                      static_cast<int>(nRead * nSize));
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    return true;
}

void *CHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];
    if (!FCHK_FRead(pBuffer, nSize, nCount, pFile)) {
        std::snprintf(szMessage,
                      sizeof(szMessage),
                      "CHK_FRead(): trying to read %u bytes, but got EOF.\n",
                      static_cast<unsigned>(nCount * nSize));
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    return pBuffer;
}

char *FileReadLine(char *pszLine, int nSize, GameFile *pFile) {
    for (int nCount = 0; nCount < nSize; nCount++) {
        int c = std::fgetc(pFile->pFile);
        if (c == EOF) {
            if (nCount == 0) {
                return nullptr;
            }
            *pszLine = '\0';
            return pszLine + 1;
        }
        if (c == '\r') {
            *pszLine++ = '\n';
            *pszLine++ = '\0';
            // A line feed after the carriage return belongs to the same line.
            c = std::fgetc(pFile->pFile);
            if (c != EOF && c != '\n') {
                std::ungetc(c, pFile->pFile);
            }
            return pszLine;
        }
        if (c == '\n') {
            *pszLine++ = '\n';
            *pszLine = '\0';
            return pszLine + 1;
        }
        *pszLine++ = static_cast<char>(c);
    }
    return pszLine;
}

void CHK_FClose(GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];
    if (g_bVerboseFileTrace) {
        std::snprintf(szMessage, sizeof(szMessage), "CHK_FClose(%s)\n", pFile->pszPath);
        WriteDebugMessage(szMessage);
    }
    if (std::fclose(pFile->pFile) == EOF) {
        std::snprintf(
            szMessage, sizeof(szMessage), "CHK_FClose(): error closing file %s.\n", pFile->pszPath);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    std::free(pFile->pszPath);
    std::free(pFile);
}

int CHK_FileExists(const char *pszPath) {
    char szMessage[MESSAGE_SIZE];
    if (g_bVerboseFileTrace) {
        std::snprintf(szMessage, sizeof(szMessage), "CHK_FileExists(%s)\n", pszPath);
        WriteDebugMessage(szMessage);
    }
    return coastline::FindGameFile(pszPath).has_value();
}

void *CHK_AllocateMemory(size_t nSize, const char *szCaller) {
    char szMessage[MESSAGE_SIZE];
    if (nSize == 0) {
        return nullptr;
    }
    void *pMemory = std::malloc(nSize);
    if (pMemory == nullptr) {
        std::snprintf(szMessage,
                      sizeof(szMessage),
                      "CHK_AllocateMemory(): Out of memory: couldn't allocate %s\n",
                      szCaller);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    return pMemory;
}

void *CHK_ReAllocateMemory(void *pMemory, size_t nSize, const char *szCaller) {
    char szMessage[MESSAGE_SIZE];
    void *pResizedMemory = std::realloc(pMemory, nSize);
    // Zero-size requests still call realloc().
    if (nSize == 0) {
        return nullptr;
    }
    if (pResizedMemory == nullptr) {
        std::snprintf(szMessage,
                      sizeof(szMessage),
                      "CHK_ReAllocateMemory(): Out of memory: couldn't reallocate %s\n",
                      szCaller);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", szMessage);
        ExitOnFileError();
    }
    return pResizedMemory;
}

} // extern "C"
