#include "discordpresence.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

#ifndef COASTLINE_DISCORD
#define COASTLINE_DISCORD 0
#endif

#if COASTLINE_DISCORD
#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <poll.h>
#endif

namespace coastline {
namespace {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
using Json = nlohmann::json;
constexpr std::uint32_t MAX_PAYLOAD = 65536;

std::string Text(std::string_view value) {
    auto count = std::min<std::size_t>(value.size(), 128);
    if (count != value.size()) {
        while (count > 0 && (static_cast<unsigned char>(value[count]) & 0xc0) == 0x80) {
            --count;
        }
    }
    return std::string(value.substr(0, count));
}

class Connection {
public:
    explicit Connection(const std::atomic<bool> &stop) : m_stop(stop) {
    }

    ~Connection() {
#ifdef _WIN32
        if (m_pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pipe);
        }
#else
        if (m_socket >= 0) {
            close(m_socket);
        }
#endif
    }

    bool Open() {
        for (unsigned index = 0; index < 10 && !m_stop; ++index) {
#ifdef _WIN32
            const auto path = L"\\\\?\\pipe\\discord-ipc-" + std::to_wstring(index);
            m_pipe = CreateFileW(path.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 nullptr);
            if (m_pipe != INVALID_HANDLE_VALUE) {
                DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
                if (SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr)) {
                    return true;
                }
                CloseHandle(m_pipe);
                m_pipe = INVALID_HANDLE_VALUE;
            }
#else
            std::string root = "/tmp";
            for (const auto *key : {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"}) {
                const auto *value = std::getenv(key);
                if (value && *value) {
                    root = value;
                    break;
                }
            }
            for (const auto *suffix : {"", "/app/com.discordapp.Discord"}) {
                const auto path = root + suffix + "/discord-ipc-" + std::to_string(index);
                sockaddr_un address{};
                if (path.size() >= sizeof(address.sun_path)) {
                    return false;
                }
                address.sun_family = AF_UNIX;
                std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
                m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
                if (m_socket < 0) {
                    return false;
                }
                if (fcntl(m_socket, F_SETFD, FD_CLOEXEC) == 0 &&
                    fcntl(m_socket, F_SETFL, O_NONBLOCK) == 0) {
#ifdef SO_NOSIGPIPE
                    const int enabled = 1;
                    if (setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) !=
                        0) {
                        close(m_socket);
                        m_socket = -1;
                        continue;
                    }
#endif
                    if (connect(m_socket,
                                reinterpret_cast<const sockaddr *>(&address),
                                sizeof(address)) == 0) {
                        return true;
                    }
                }
                close(m_socket);
                m_socket = -1;
            }
#endif
        }
        return false;
    }

    int Read(std::span<std::uint8_t> bytes, bool waiting = false) {
        return Transfer(bytes, false, waiting);
    }

    bool Send(std::uint32_t opcode, std::string_view payload) {
        if (payload.size() > MAX_PAYLOAD) {
            return false;
        }
        std::vector<std::uint8_t> frame(8 + payload.size());
        for (unsigned byte = 0; byte < 4; ++byte) {
            frame[byte] = static_cast<std::uint8_t>(opcode >> (byte * 8));
            frame[byte + 4] = static_cast<std::uint8_t>(payload.size() >> (byte * 8));
        }
        std::copy(payload.begin(), payload.end(), frame.begin() + 8);
        return Transfer(frame, true, true) == 1;
    }

private:
    int Transfer(std::span<std::uint8_t> bytes, bool writing, bool waiting) {
        const auto deadline = Clock::now() + 500ms;
        bool partial = false;
        while (!bytes.empty() && !m_stop) {
#ifdef _WIN32
            OVERLAPPED operation{};
            operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!operation.hEvent) {
                return -1;
            }
            DWORD count = 0;
            bool good = (writing ? WriteFile(m_pipe,
                                             bytes.data(),
                                             static_cast<DWORD>(bytes.size()),
                                             &count,
                                             &operation) :
                                   ReadFile(m_pipe,
                                            bytes.data(),
                                            static_cast<DWORD>(bytes.size()),
                                            &count,
                                            &operation)) != FALSE;
            const bool pending = !good && GetLastError() == ERROR_IO_PENDING;
            if (pending) {
                bool idle = false;
                while (!m_stop && Clock::now() < deadline) {
                    if (WaitForSingleObject(operation.hEvent, 20) == WAIT_OBJECT_0) {
                        good = GetOverlappedResult(m_pipe, &operation, &count, FALSE) != FALSE;
                        break;
                    }
                    if (!writing && !partial && !waiting) {
                        idle = true;
                        break;
                    }
                }
                if (!good) {
                    CancelIoEx(m_pipe, &operation);
                    good = GetOverlappedResult(m_pipe, &operation, &count, TRUE) != FALSE;
                    if (!good && idle && GetLastError() == ERROR_OPERATION_ABORTED) {
                        CloseHandle(operation.hEvent);
                        return 0;
                    }
                }
            }
            CloseHandle(operation.hEvent);
            if (!good) {
                return -1;
            }
#else
            pollfd descriptor{m_socket, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
            const auto ready = poll(&descriptor, 1, 20);
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready < 0 || (descriptor.revents & (POLLERR | POLLNVAL))) {
                return -1;
            }
            if (ready == 0) {
                if (!writing && !partial && !waiting) {
                    return 0;
                }
                if (Clock::now() >= deadline) {
                    return -1;
                }
                continue;
            }
#ifdef MSG_NOSIGNAL
            constexpr int flags = MSG_NOSIGNAL;
#else
            constexpr int flags = 0;
#endif
            const auto count = writing ? send(m_socket, bytes.data(), bytes.size(), flags) :
                                         recv(m_socket, bytes.data(), bytes.size(), 0);
            if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (Clock::now() >= deadline) {
                    return -1;
                }
                continue;
            }
#endif
            if (count <= 0 || static_cast<std::size_t>(count) > bytes.size()) {
                return -1;
            }
            bytes = bytes.subspan(static_cast<std::size_t>(count));
            partial = true;
            if (!bytes.empty() && Clock::now() >= deadline) {
                return -1;
            }
        }
        return bytes.empty() ? 1 : -1;
    }

    const std::atomic<bool> &m_stop;
#ifdef _WIN32
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
#else
    int m_socket = -1;
#endif
};

std::uint32_t Number(const std::array<std::uint8_t, 8> &header, unsigned offset) {
    std::uint32_t result = 0;
    for (unsigned byte = 0; byte < 4; ++byte) {
        result |= static_cast<std::uint32_t>(header[offset + byte]) << (byte * 8);
    }
    return result;
}

std::uint64_t ProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}
} // namespace

struct DiscordPresence::Worker {
    std::atomic<bool> stop{false};
    std::mutex mutex;
    std::condition_variable changed;
    std::string details;
    std::string state;
    std::uint64_t revision = 0;
    std::string id;
    std::thread thread;

    explicit Worker(std::string_view applicationId)
        : id(applicationId), thread([this] {
              try {
                  Run();
              } catch (const std::exception &) {
              }
          }) {
    }

    ~Worker() {
        stop = true;
        changed.notify_all();
        thread.join();
    }

    void Run() {
        std::uint64_t nonce = 0;
        while (!stop) {
            {
                Connection connection(stop);
                if (connection.Open() &&
                    connection.Send(0, Json{{"v", 1}, {"client_id", id}}.dump())) {
                    try {
                        Session(connection, nonce);
                    } catch (const Json::exception &) {
                    }
                }
            }
            std::unique_lock lock(mutex);
            changed.wait_for(lock, 5s, [&] { return stop.load(); });
        }
    }

    void Session(Connection &connection, std::uint64_t &nonce) {
        bool ready = false;
        std::uint64_t sent = 0;
        auto nextSend = Clock::now();
        const auto handshakeDeadline = nextSend + 5s;
        const auto started = std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
        while (!stop) {
            if (!ready && Clock::now() > handshakeDeadline) {
                return;
            }
            std::array<std::uint8_t, 8> header{};
            const auto result = connection.Read(header);
            if (result < 0) {
                return;
            }
            if (result > 0) {
                const auto opcode = Number(header, 0);
                const auto length = Number(header, 4);
                if (length > MAX_PAYLOAD || opcode > 4) {
                    return;
                }
                std::string payload(length, '\0');
                if (length &&
                    connection.Read(
                        std::span(reinterpret_cast<std::uint8_t *>(payload.data()), payload.size()),
                        true) != 1) {
                    return;
                }
                if (opcode == 2) {
                    return;
                }
                if (opcode == 3 && !connection.Send(4, payload)) {
                    return;
                }
                if (opcode == 1) {
                    bool bounded = true;
                    const auto message = Json::parse(
                        payload,
                        [&](int depth, Json::parse_event_t, Json &) {
                            bounded = bounded && depth <= 16;
                            return depth <= 16;
                        },
                        false);
                    if (!bounded || !message.is_object()) {
                        return;
                    }
                    if (message.value("evt", Json{}) == "ERROR") {
                        return;
                    }
                    if (message.value("evt", Json{}) == "READY") {
                        ready = true;
                    }
                }
            }
            if (ready && Clock::now() >= nextSend) {
                std::string currentDetails, currentState;
                std::uint64_t currentRevision;
                {
                    std::lock_guard lock(mutex);
                    currentRevision = revision;
                    currentDetails = details;
                    currentState = state;
                }
                if (currentRevision != sent) {
                    Json activity{{"type", 0},
                                  {"details", currentDetails},
                                  {"timestamps", {{"start", started}}}};
                    if (!currentState.empty()) {
                        activity["state"] = currentState;
                    }
                    const Json message{{"cmd", "SET_ACTIVITY"},
                                       {"nonce", std::to_string(++nonce)},
                                       {"args", {{"pid", ProcessId()}, {"activity", activity}}}};
                    if (!connection.Send(
                            1, message.dump(-1, ' ', false, Json::error_handler_t::replace))) {
                        return;
                    }
                    sent = currentRevision;
                    nextSend = Clock::now() + 15s;
                }
            }
        }
    }
};

DiscordPresence::DiscordPresence(std::string_view id) {
    std::uint64_t number = 0;
    if (!id.empty() && id.size() <= 20 && id.front() != '0') {
        const auto parsed = std::from_chars(id.data(), id.data() + id.size(), number);
        if (number != 0 && parsed.ec == std::errc{} && parsed.ptr == id.data() + id.size()) {
            try {
                m_worker = std::make_unique<Worker>(id);
            } catch (const std::exception &) {
            }
        }
    }
}

DiscordPresence::~DiscordPresence() = default;

void DiscordPresence::Update(std::string_view details, std::string_view state) {
    if (!m_worker) {
        return;
    }
    const auto boundedDetails = Text(details), boundedState = Text(state);
    std::lock_guard lock(m_worker->mutex);
    if (m_worker->details != boundedDetails || m_worker->state != boundedState) {
        m_worker->details = boundedDetails;
        m_worker->state = boundedState;
        ++m_worker->revision;
    }
}

} // namespace coastline
#else
namespace coastline {
struct DiscordPresence::Worker {};
DiscordPresence::DiscordPresence(std::string_view) {
}
DiscordPresence::~DiscordPresence() = default;
void DiscordPresence::Update(std::string_view, std::string_view) {
}
} // namespace coastline
#endif
