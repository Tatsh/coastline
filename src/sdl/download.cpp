#include "download.h"

#include <array>
#include <fstream>
#include <memory>

#include <windows.h>
#include <winhttp.h>

namespace coastline {

std::optional<std::string> DownloadFile(const std::string &url,
                                        const std::filesystem::path &output,
                                        const ImportProgress &progress) {
    const auto failure = [](const char *step) {
        return std::string("Cannot download (") + step + ", error " +
               std::to_string(GetLastError()) + ").";
    };
    const std::wstring wideUrl(url.begin(), url.end());
    std::wstring host(256, L'\0');
    std::wstring path(2048, L'\0');
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.lpszHostName = host.data();
    parts.dwHostNameLength = static_cast<DWORD>(host.size());
    parts.lpszUrlPath = path.data();
    parts.dwUrlPathLength = static_cast<DWORD>(path.size());
    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) {
        return failure("URL");
    }
    using Internet = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;
    const Internet session(WinHttpOpen(L"Coastline",
                                       WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                       WINHTTP_NO_PROXY_NAME,
                                       WINHTTP_NO_PROXY_BYPASS,
                                       0),
                           WinHttpCloseHandle);
    if (!session) {
        return failure("session");
    }
    // Windows 7 enables only TLS 1.0 by default.
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    WinHttpSetTimeouts(session.get(), 30000, 30000, 30000, 60000);
    const Internet connection(WinHttpConnect(session.get(), parts.lpszHostName, parts.nPort, 0),
                              WinHttpCloseHandle);
    if (!connection) {
        return failure("connect");
    }
    const Internet request(WinHttpOpenRequest(connection.get(),
                                              L"GET",
                                              parts.lpszUrlPath,
                                              nullptr,
                                              WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES,
                                              WINHTTP_FLAG_SECURE),
                           WinHttpCloseHandle);
    if (!request) {
        return failure("request");
    }
    if (!WinHttpSendRequest(
            request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr)) {
        return failure("send");
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(),
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &status,
                             &statusSize,
                             WINHTTP_NO_HEADER_INDEX) ||
        status != 200) {
        return "Cannot download (HTTP " + std::to_string(status) + ").";
    }
    std::ofstream file(output, std::ios::binary);
    std::array<char, 65536> buffer{};
    for (;;) {
        DWORD read = 0;
        if (!WinHttpReadData(
                request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &read)) {
            return failure("read");
        }
        if (read == 0) {
            break;
        }
        if (!file.write(buffer.data(), read)) {
            return "Cannot write " + output.string() + ".";
        }
        if (progress && !progress("Downloading...", 0)) {
            return "Import cancelled.";
        }
    }
    file.close();
    if (!file) {
        return "Cannot write " + output.string() + ".";
    }
    return std::nullopt;
}

} // namespace coastline
