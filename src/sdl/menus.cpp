#include "menus.h"

namespace coastline {

Menus::Menus(std::unique_ptr<MenuScreen> screen) : m_screen(std::move(screen)) {
}

Menus::~Menus() = default;

void Menus::Restart() {
    m_current = Screen::MainMenu;
}

MenusEnd Menus::Frame(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    m_end = MenusEnd::Running;
    if (m_current == Screen::NotBuilt) {
        m_current = Screen::MainMenu;
    }
    switch (m_current) {
    case Screen::MainMenu:
        ProcessMainMenu(renderer, seconds, first);
        break;
    case Screen::NotBuilt:
        break;
    }
    return m_end;
}

} // namespace coastline
