#include "configsaver.h"

#include <SDL3/SDL.h>

namespace coastline {

ConfigSaver::ConfigSaver() : m_thread([this] { Run(); }) {
}

ConfigSaver::~ConfigSaver() {
    {
        std::lock_guard lock(m_mutex);
        m_stop = true;
    }
    m_wake.notify_one();
    m_thread.join();
}

void ConfigSaver::Save(GameConfig config, std::filesystem::path path) {
    {
        std::lock_guard lock(m_mutex);
        m_pending.emplace(std::move(config), std::move(path));
    }
    m_wake.notify_one();
}

bool ConfigSaver::Flush() {
    std::unique_lock lock(m_mutex);
    m_idle.wait(lock, [this] { return !m_pending && !m_writing; });
    return !m_failed;
}

bool ConfigSaver::Failed() {
    std::lock_guard lock(m_mutex);
    return m_failed;
}

void ConfigSaver::Run() {
    std::unique_lock lock(m_mutex);
    for (;;) {
        m_wake.wait(lock, [this] { return m_pending || m_stop; });
        if (!m_pending) {
            return;
        }
        auto [config, path] = std::move(*m_pending);
        m_pending.reset();
        m_writing = true;
        lock.unlock();
        const bool saved = config.Save(path);
        if (!saved) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot write %s.", path.string().c_str());
        }
        lock.lock();
        m_writing = false;
        m_failed = !saved;
        m_idle.notify_all();
    }
}

} // namespace coastline
