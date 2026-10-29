#include <cstdint>

namespace {
class DisplayModeList {
public:
    /** @ghidraAddress BRD3D: 0x1001ba00 */
    DisplayModeList();
    /** @ghidraAddress BRD3D: 0x10008b80 */
    ~DisplayModeList();

private:
    [[maybe_unused]] std::uint32_t reservedWords[3];
};

DisplayModeList::DisplayModeList() : reservedWords{} {
}

DisplayModeList::~DisplayModeList() {
}

// The native object is only constructed and destroyed.
// @ghidraAddress BRD3D: 0x118abe10
DisplayModeList g_DisplayModeList;
} // namespace
