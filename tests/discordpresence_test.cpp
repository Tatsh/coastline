#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "discordpresence.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdlib>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <poll.h>
#endif

namespace {
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;

class Server {
public:
    Server() {
#ifdef _WIN32
        m_pipe = CreateNamedPipeW(L"\\\\?\\pipe\\discord-ipc-9",
                                  PIPE_ACCESS_DUPLEX,
                                  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
                                  1,
                                  65536,
                                  65536,
                                  0,
                                  nullptr);
        assert(m_pipe != INVALID_HANDLE_VALUE);
#else
        m_root = std::filesystem::temp_directory_path() /
                 ("coastline-discord-test-" + std::to_string(getpid()));
        std::filesystem::create_directories(m_root);
        setenv("XDG_RUNTIME_DIR", m_root.c_str(), 1);
        const auto path = (m_root / "discord-ipc-9").string();
        sockaddr_un address{};
        assert(path.size() < sizeof(address.sun_path));
        address.sun_family = AF_UNIX;
        std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
        m_listener = socket(AF_UNIX, SOCK_STREAM, 0);
        assert(m_listener >= 0);
        assert(fcntl(m_listener, F_SETFL, O_NONBLOCK) == 0);
        assert(bind(m_listener, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) ==
               0);
        assert(listen(m_listener, 2) == 0);
#endif
    }

    ~Server() {
#ifdef _WIN32
        CloseHandle(m_pipe);
#else
        if (m_client >= 0) {
            close(m_client);
        }
        close(m_listener);
        std::filesystem::remove_all(m_root);
#endif
    }

    bool Accept(std::chrono::milliseconds wait = 5s) {
        const auto until = Clock::now() + wait;
        do {
#ifdef _WIN32
            if (ConnectNamedPipe(m_pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
                return true;
            }
#else
            m_client = accept(m_listener, nullptr, nullptr);
            if (m_client >= 0) {
                assert(fcntl(m_client, F_SETFL, O_NONBLOCK) == 0);
                return true;
            }
#endif
            std::this_thread::sleep_for(2ms);
        } while (Clock::now() < until);
        return false;
    }

    bool
    Transfer(std::span<std::uint8_t> bytes, bool writing, std::chrono::milliseconds wait = 20s) {
        const auto until = Clock::now() + wait;
        do {
#ifdef _WIN32
            DWORD count = 0;
            const auto good =
                writing ?
                    WriteFile(
                        m_pipe, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr) :
                    ReadFile(
                        m_pipe, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr);
            if (good && count > 0) {
                bytes = bytes.subspan(count);
            }
#else
#ifdef MSG_NOSIGNAL
            constexpr int flags = MSG_NOSIGNAL;
#else
            constexpr int flags = 0;
#endif
            const auto count = writing ? send(m_client, bytes.data(), bytes.size(), flags) :
                                         recv(m_client, bytes.data(), bytes.size(), 0);
            if (count > 0) {
                bytes = bytes.subspan(static_cast<std::size_t>(count));
            }
#endif
            if (bytes.empty()) {
                return true;
            }
            std::this_thread::sleep_for(2ms);
        } while (Clock::now() < until);
        return false;
    }

    void Send(std::uint32_t opcode, std::string payload, bool fragment = false) {
        std::vector<std::uint8_t> bytes(8 + payload.size());
        for (unsigned index = 0; index < 4; ++index) {
            bytes[index] = static_cast<std::uint8_t>(opcode >> (index * 8));
            bytes[index + 4] = static_cast<std::uint8_t>(payload.size() >> (index * 8));
        }
        std::copy(payload.begin(), payload.end(), bytes.begin() + 8);
        if (payload.size() > 65536) {
            bytes.resize(8);
        }
        if (fragment) {
            for (auto &byte : bytes) {
                assert(Transfer(std::span(&byte, 1), true));
                std::this_thread::sleep_for(1ms);
            }
        } else {
            assert(Transfer(bytes, true));
        }
    }

    Json Receive(std::uint32_t opcode, std::chrono::milliseconds wait = 20s) {
        std::array<std::uint8_t, 8> header{};
        if (!Transfer(header, false, wait)) {
            return nullptr;
        }
        auto number = [&](unsigned offset) {
            std::uint32_t value = 0;
            for (unsigned index = 0; index < 4; ++index) {
                value |= static_cast<std::uint32_t>(header[offset + index]) << (index * 8);
            }
            return value;
        };
        assert(number(0) == opcode && number(4) <= 65536);
        std::string payload(number(4), '\0');
        assert(Transfer(std::span(reinterpret_cast<std::uint8_t *>(payload.data()), payload.size()),
                        false));
        return Json::parse(payload);
    }

    bool Closed() {
        const auto until = Clock::now() + 2s;
        do {
#ifdef _WIN32
            DWORD available = 0;
            if (!PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &available, nullptr)) {
                return true;
            }
#else
            std::uint8_t byte;
            if (recv(m_client, &byte, 1, MSG_PEEK) == 0) {
                return true;
            }
#endif
            std::this_thread::sleep_for(2ms);
        } while (Clock::now() < until);
        return false;
    }

    void Reset() {
#ifdef _WIN32
        DisconnectNamedPipe(m_pipe);
#else
        close(m_client);
        m_client = -1;
#endif
    }

private:
#ifdef _WIN32
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
#else
    std::filesystem::path m_root;
    int m_listener = -1;
    int m_client = -1;
#endif
};
} // namespace

int main() {
    Server server;
    for (const auto *id : {"", "0", "01", "invalid", "18446744073709551616", "123bad"}) {
        coastline::DiscordPresence disabled(id);
        disabled.Update("Racing");
        assert(!server.Accept(50ms));
    }
#if !COASTLINE_DISCORD
    coastline::DiscordPresence disabled("123456789012345678");
    disabled.Update("Racing");
    assert(!server.Accept(100ms));
#else
    {
        coastline::DiscordPresence presence("123456789012345678");
        presence.Update("Racing", "Jungle");
        assert(server.Accept());
        const auto handshake = server.Receive(0);
        assert(handshake["v"] == 1 && handshake["client_id"] == "123456789012345678");
        assert(server.Receive(1, 100ms).is_null());
        server.Send(3, "{\"ping\":true}", true);
        assert(server.Receive(4)["ping"] == true);
        server.Send(1, "{\"cmd\":\"DISPATCH\",\"evt\":\"READY\"}", true);
        const auto racing = server.Receive(1);
        assert(racing["cmd"] == "SET_ACTIVITY" && racing["args"]["pid"] > 0);
        assert(racing["args"]["activity"]["details"] == "Racing");
        assert(racing["args"]["activity"]["state"] == "Jungle");
        assert(racing["args"]["activity"]["timestamps"]["start"] > 0);
        presence.Update("Racing", "Jungle");
        assert(server.Receive(1, 100ms).is_null());
        presence.Update("Discarded pending state");
        presence.Update("In the menus");
        const auto menus = server.Receive(1);
        assert(menus["nonce"] != racing["nonce"]);
        assert(menus["args"]["activity"]["details"] == "In the menus");
        assert(!menus["args"]["activity"].contains("state"));
        server.Send(1, "{\"evt\":\"ERROR\"}");
        assert(server.Closed());
        server.Reset();
        assert(server.Accept(7s));
        assert(server.Receive(0)["client_id"] == "123456789012345678");
        presence.Update(std::string(127, 'x') + "\xe6\x97\xa5");
        server.Send(1, "{\"evt\":\"READY\"}");
        assert(server.Receive(1)["args"]["activity"]["details"] == std::string(127, 'x'));
    }
    assert(server.Closed());
    server.Reset();
    for (const auto &payload : {std::string("not JSON"),
                                std::string(65537, 'x'),
                                std::string(100, '[') + "0" + std::string(100, ']')}) {
        coastline::DiscordPresence presence("123456789012345678");
        assert(server.Accept());
        assert(!server.Receive(0).is_null());
        server.Send(1, payload);
        assert(server.Closed());
        server.Reset();
    }
#endif
}
