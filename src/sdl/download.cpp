#include "download.h"

#include <array>
#include <fstream>
#include <memory>

#include <fmt/format.h>
#include <windows.h>
#include <winhttp.h>

namespace coastline {

namespace {

using Event = std::unique_ptr<void, decltype(&CloseHandle)>;
using Internet = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;

struct Transfer {
    Event done{CreateEventW(nullptr, FALSE, FALSE, nullptr), CloseHandle};
    Event closed{CreateEventW(nullptr, TRUE, FALSE, nullptr), CloseHandle};
    DWORD error = 0;
    DWORD read = 0;
};

void CALLBACK
OnStatus(HINTERNET, DWORD_PTR context, DWORD status, LPVOID information, DWORD length) {
    auto *const transfer = reinterpret_cast<Transfer *>(context);
    switch (status) {
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        SetEvent(transfer->closed.get());
        return;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        transfer->read = length;
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        transfer->error = static_cast<WINHTTP_ASYNC_RESULT *>(information)->dwError;
        break;
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        break;
    default:
        return;
    }
    SetEvent(transfer->done.get());
}

// WinHTTP can call OnStatus after WinHttpCloseHandle returns, and a pending read still writes to
// the buffer. The request is released only once its handle reports closing.
struct Request {
    HINTERNET handle;
    Transfer &transfer;
    ~Request() {
        if (handle) {
            WinHttpCloseHandle(handle);
            WaitForSingleObject(transfer.closed.get(), INFINITE);
        }
    }
};

// Returns false when the user cancels.
bool Wait(const Transfer &transfer, const ImportProgress &progress) {
    for (;;) {
        if (progress && !progress("Downloading...", 0)) {
            return false;
        }
        if (WaitForSingleObject(transfer.done.get(), 100) == WAIT_OBJECT_0) {
            return true;
        }
    }
}

} // namespace

std::optional<std::string> DownloadFile(const std::string &url,
                                        const std::filesystem::path &output,
                                        const ImportProgress &progress) {
    const auto failure = [](const char *step, DWORD error = GetLastError()) {
        return fmt::format("Cannot download ({}, error {}).", step, error);
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
    Transfer transfer;
    if (!transfer.done || !transfer.closed) {
        return failure("event");
    }
    std::array<char, 65536> buffer{};
    const Internet session(WinHttpOpen(L"Coastline",
                                       WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                       WINHTTP_NO_PROXY_NAME,
                                       WINHTTP_NO_PROXY_BYPASS,
                                       WINHTTP_FLAG_ASYNC),
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
    HINTERNET handle = WinHttpOpenRequest(connection.get(),
                                          L"GET",
                                          parts.lpszUrlPath,
                                          nullptr,
                                          WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          WINHTTP_FLAG_SECURE);
    if (!handle) {
        return failure("request");
    }
    auto context = reinterpret_cast<DWORD_PTR>(&transfer);
    if (!WinHttpSetOption(handle, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context)) ||
        WinHttpSetStatusCallback(handle,
                                 OnStatus,
                                 WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS |
                                     WINHTTP_CALLBACK_FLAG_HANDLES,
                                 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
        const auto message = failure("callback");
        WinHttpCloseHandle(handle);
        return message;
    }
    const Request request{handle, transfer};
    if (!WinHttpSendRequest(
            handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, context)) {
        return failure("send");
    }
    if (!Wait(transfer, progress)) {
        return "Import cancelled.";
    }
    if (transfer.error || !WinHttpReceiveResponse(handle, nullptr)) {
        return failure("send", transfer.error ? transfer.error : GetLastError());
    }
    if (!Wait(transfer, progress)) {
        return "Import cancelled.";
    }
    if (transfer.error) {
        return failure("response", transfer.error);
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(handle,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &status,
                             &statusSize,
                             WINHTTP_NO_HEADER_INDEX) ||
        status != 200) {
        return fmt::format("Cannot download (HTTP {}).", status);
    }
    std::ofstream file(output, std::ios::binary);
    for (;;) {
        if (!WinHttpReadData(handle, buffer.data(), static_cast<DWORD>(buffer.size()), nullptr)) {
            return failure("read");
        }
        if (!Wait(transfer, progress)) {
            return "Import cancelled.";
        }
        if (transfer.error) {
            return failure("read", transfer.error);
        }
        if (transfer.read == 0) {
            break;
        }
        if (!file.write(buffer.data(), transfer.read)) {
            return fmt::format("Cannot write {}.", output.string());
        }
    }
    file.close();
    if (!file) {
        return fmt::format("Cannot write {}.", output.string());
    }
    return std::nullopt;
}

} // namespace coastline
