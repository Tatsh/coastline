#include "mipscpu.h"

#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>

namespace coastline {

namespace {

constexpr std::uint32_t KSEG0 = 0x80000000u;

// A word as 0x and eight hexadecimal digits, for the faults.
std::string Hex32(std::uint32_t value) {
    char text[11];
    std::snprintf(text, sizeof(text), "0x%08x", static_cast<unsigned>(value));
    return text;
}

// The high and the low word of the unsigned 128-bit product of two words, from their 32-bit halves,
// as DMULTU makes it on every compiler.
void MultiplyWide(std::uint64_t a, std::uint64_t b, std::uint64_t &high, std::uint64_t &low) {
    std::uint64_t aLow = a & 0xFFFFFFFFu;
    std::uint64_t aHigh = a >> 32;
    std::uint64_t bLow = b & 0xFFFFFFFFu;
    std::uint64_t bHigh = b >> 32;
    std::uint64_t lowProduct = aLow * bLow;
    std::uint64_t across = aHigh * bLow;
    std::uint64_t down = aLow * bHigh;
    std::uint64_t middle = (lowProduct >> 32) + (across & 0xFFFFFFFFu) + (down & 0xFFFFFFFFu);
    low = (middle << 32) | (lowProduct & 0xFFFFFFFFu);
    high = aHigh * bHigh + (across >> 32) + (down >> 32) + (middle >> 32);
}
constexpr std::uint32_t KSEG2 = 0xC0000000u;
constexpr std::uint32_t PHYSICAL_MASK = 0x1FFFFFFFu;
constexpr std::uint32_t CARTRIDGE_START = 0x10000000u;
constexpr std::uint32_t CARTRIDGE_END = 0x1FC00000u;
// The branch to itself an idle loop spins on (beq zero, zero, -1).
constexpr std::uint32_t BRANCH_TO_SELF = 0x1000FFFFu;
// The bits of the control and status register of the floating-point unit.
constexpr std::uint32_t FCR31_CONDITION = 1u << 23;
constexpr std::uint32_t FCR31_FLUSH = 1u << 24;
constexpr std::uint32_t FCR31_ROUNDING = 3u;
// The value the implementation register of the floating-point unit reads.
constexpr std::uint32_t FCR0_VR4300 = 0x0B00u;
constexpr int COP0_COUNT = 9;

std::uint64_t SignExtend32(std::uint32_t value) {
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(value)));
}

std::uint64_t SignExtend16(std::uint32_t value) {
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(value)));
}

int Rs(std::uint32_t instruction) {
    return static_cast<int>((instruction >> 21) & 31);
}

int Rt(std::uint32_t instruction) {
    return static_cast<int>((instruction >> 16) & 31);
}

int Rd(std::uint32_t instruction) {
    return static_cast<int>((instruction >> 11) & 31);
}

int Shift(std::uint32_t instruction) {
    return static_cast<int>((instruction >> 6) & 31);
}

// The value of a result too small to be normal, zero when the flush bit is set as libultra sets it.
template <typename T>
T Flush(T value, std::uint32_t fcr31) {
    if ((fcr31 & FCR31_FLUSH) != 0 && std::fpclassify(value) == FP_SUBNORMAL) {
        return std::copysign(T(0), value);
    }
    return value;
}

// A value rounded to an integer by the mode of a conversion (0 to the nearest, 1 towards zero, 2
// up, and 3 down).
double RoundBy(double value, std::uint32_t mode) {
    switch (mode) {
    case 1:
        return std::trunc(value);
    case 2:
        return std::ceil(value);
    case 3:
        return std::floor(value);
    default:
        return std::nearbyint(value);
    }
}

// A value converted to a 32-bit integer, the largest negative one when it is out of range.
std::uint32_t ToWord(double value, std::uint32_t mode) {
    double rounded = RoundBy(value, mode);
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        return 0x80000000u;
    }
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(rounded));
}

// A value converted to a 64-bit integer, the largest negative one when it is out of range.
std::uint64_t ToLong(double value, std::uint32_t mode) {
    double rounded = RoundBy(value, mode);
    if (!(rounded >= -9223372036854775808.0 && rounded < 9223372036854775808.0)) {
        return 0x8000000000000000ull;
    }
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(rounded));
}

// Whether a comparison of the floating-point unit is true, from its four low bits of condition.
template <typename T>
bool Compare(T a, T b, std::uint32_t condition) {
    bool unordered = std::isnan(a) || std::isnan(b);
    bool less = !unordered && a < b;
    bool equal = !unordered && a == b;
    return ((condition & 1) != 0 && unordered) || ((condition & 2) != 0 && equal) ||
           ((condition & 4) != 0 && less);
}

} // namespace

MipsCpu::MipsCpu(std::size_t ramSize) : ram(ramSize, 0), m_hooked(ramSize / 4, 0) {
}

void MipsCpu::SetRom(std::span<const std::uint8_t> rom) {
    m_rom = rom;
}

void MipsCpu::SetHook(std::uint32_t address, Hook hook) {
    std::uint32_t physical = address & PHYSICAL_MASK;
    if (physical / 4 < m_hooked.size()) {
        m_hooked[physical / 4] = 1;
    }
    m_hooks[physical] = std::move(hook);
}

void MipsCpu::SetDevices(DeviceRead read, DeviceWrite write) {
    m_deviceRead = std::move(read);
    m_deviceWrite = std::move(write);
}

void MipsCpu::SetIdle(std::function<void(MipsCpu &)> idle) {
    m_idle = std::move(idle);
}

void MipsCpu::Stop() {
    m_stopped = true;
}

void MipsCpu::Return(std::uint32_t value) {
    gpr[MIPS_V0] = SignExtend32(value);
    Jump(static_cast<std::uint32_t>(gpr[MIPS_RA]));
}

void MipsCpu::Jump(std::uint32_t address) {
    pc = address;
    npc = address + 4;
}

std::uint32_t MipsCpu::Argument(int index) {
    if (index < 4) {
        return static_cast<std::uint32_t>(gpr[MIPS_A0 + index]);
    }
    return Read32(static_cast<std::uint32_t>(gpr[MIPS_SP]) + static_cast<std::uint32_t>(index) * 4);
}

void MipsCpu::SetFault(const std::string &fault) {
    if (m_fault.empty()) {
        m_fault = fault + " (instruction at " + Hex32(m_current) + ")";
    }
}

std::uint8_t *MipsCpu::RamAt(std::uint32_t address, int size) {
    if (address < KSEG0 || address >= KSEG2) {
        return nullptr;
    }
    std::uint32_t physical = address & PHYSICAL_MASK;
    if (physical + static_cast<std::uint32_t>(size) > ram.size()) {
        return nullptr;
    }
    return ram.data() + physical;
}

std::uint64_t MipsCpu::ReadDevice(std::uint32_t address, int size) {
    if (address < KSEG0 || address >= KSEG2) {
        SetFault("A read of the mapped address " + Hex32(address));
        return 0;
    }
    std::uint32_t physical = address & PHYSICAL_MASK;
    if (physical >= CARTRIDGE_START && physical < CARTRIDGE_END) {
        std::uint64_t value = 0;
        std::size_t offset = physical - CARTRIDGE_START;
        for (int i = 0; i < size; ++i) {
            std::size_t at = offset + static_cast<std::size_t>(i);
            value = value << 8 | (at < m_rom.size() ? m_rom[at] : 0);
        }
        return value;
    }
    return m_deviceRead ? m_deviceRead(physical, size) : 0;
}

void MipsCpu::WriteDevice(std::uint32_t address, int size, std::uint64_t value) {
    if (address < KSEG0 || address >= KSEG2) {
        SetFault("A write to the mapped address " + Hex32(address));
        return;
    }
    if (m_deviceWrite) {
        m_deviceWrite(address & PHYSICAL_MASK, size, value);
    }
}

std::uint8_t MipsCpu::Read8(std::uint32_t address) {
    if (std::uint8_t *p = RamAt(address, 1)) {
        return *p;
    }
    return static_cast<std::uint8_t>(ReadDevice(address, 1));
}

std::uint16_t MipsCpu::Read16(std::uint32_t address) {
    if ((address & 1) != 0) {
        SetFault("An unaligned read of " + Hex32(address));
        return 0;
    }
    if (std::uint8_t *p = RamAt(address, 2)) {
        return static_cast<std::uint16_t>(p[0] << 8 | p[1]);
    }
    return static_cast<std::uint16_t>(ReadDevice(address, 2));
}

std::uint32_t MipsCpu::Read32(std::uint32_t address) {
    if ((address & 3) != 0) {
        SetFault("An unaligned read of " + Hex32(address));
        return 0;
    }
    if (std::uint8_t *p = RamAt(address, 4)) {
        return static_cast<std::uint32_t>(p[0]) << 24 | static_cast<std::uint32_t>(p[1]) << 16 |
               static_cast<std::uint32_t>(p[2]) << 8 | p[3];
    }
    return static_cast<std::uint32_t>(ReadDevice(address, 4));
}

std::uint64_t MipsCpu::Read64(std::uint32_t address) {
    if ((address & 7) != 0) {
        SetFault("An unaligned read of " + Hex32(address));
        return 0;
    }
    return static_cast<std::uint64_t>(Read32(address)) << 32 | Read32(address + 4);
}

void MipsCpu::Write8(std::uint32_t address, std::uint8_t value) {
    if (std::uint8_t *p = RamAt(address, 1)) {
        *p = value;
        return;
    }
    WriteDevice(address, 1, value);
}

void MipsCpu::Write16(std::uint32_t address, std::uint16_t value) {
    if ((address & 1) != 0) {
        SetFault("An unaligned write to " + Hex32(address));
        return;
    }
    if (std::uint8_t *p = RamAt(address, 2)) {
        p[0] = static_cast<std::uint8_t>(value >> 8);
        p[1] = static_cast<std::uint8_t>(value);
        return;
    }
    WriteDevice(address, 2, value);
}

void MipsCpu::Write32(std::uint32_t address, std::uint32_t value) {
    if ((address & 3) != 0) {
        SetFault("An unaligned write to " + Hex32(address));
        return;
    }
    if (std::uint8_t *p = RamAt(address, 4)) {
        p[0] = static_cast<std::uint8_t>(value >> 24);
        p[1] = static_cast<std::uint8_t>(value >> 16);
        p[2] = static_cast<std::uint8_t>(value >> 8);
        p[3] = static_cast<std::uint8_t>(value);
        return;
    }
    WriteDevice(address, 4, value);
}

void MipsCpu::Write64(std::uint32_t address, std::uint64_t value) {
    if ((address & 7) != 0) {
        SetFault("An unaligned write to " + Hex32(address));
        return;
    }
    Write32(address, static_cast<std::uint32_t>(value >> 32));
    Write32(address + 4, static_cast<std::uint32_t>(value));
}

float MipsCpu::ReadFloat(std::uint32_t address) {
    return std::bit_cast<float>(Read32(address));
}

float MipsCpu::GetSingle(int reg) const {
    return std::bit_cast<float>(fpr[reg]);
}

void MipsCpu::SetSingle(int reg, float value) {
    fpr[reg] = std::bit_cast<std::uint32_t>(value);
}

std::uint64_t MipsCpu::GetLong(int reg) const {
    int even = reg & ~1;
    return static_cast<std::uint64_t>(fpr[even + 1]) << 32 | fpr[even];
}

void MipsCpu::SetLong(int reg, std::uint64_t value) {
    int even = reg & ~1;
    fpr[even] = static_cast<std::uint32_t>(value);
    fpr[even + 1] = static_cast<std::uint32_t>(value >> 32);
}

double MipsCpu::GetDouble(int reg) const {
    return std::bit_cast<double>(GetLong(reg));
}

void MipsCpu::SetDouble(int reg, double value) {
    SetLong(reg, std::bit_cast<std::uint64_t>(value));
}

bool MipsCpu::Run(std::uint64_t budget) {
    m_stopped = false;
    for (std::uint64_t i = 0; i < budget; ++i) {
        std::uint32_t physical = pc & PHYSICAL_MASK;
        if (physical / 4 < m_hooked.size() && m_hooked[physical / 4] != 0) {
            auto hook = m_hooks.find(physical);
            if (hook != m_hooks.end() && hook->second(*this) == HookResult::Jumped) {
                if (m_stopped || !m_fault.empty()) {
                    break;
                }
                continue;
            }
        }
        if (m_stopped || !m_fault.empty()) {
            break;
        }
        Step();
        gpr[MIPS_ZERO] = 0;
        if (!m_fault.empty()) {
            return false;
        }
    }
    return m_stopped && m_fault.empty();
}

void MipsCpu::Step() {
    std::uint32_t address = pc;
    m_current = address;
    std::uint32_t instruction = Read32(address);
    if (!m_fault.empty()) {
        return;
    }
    if (instruction == BRANCH_TO_SELF && m_idle) {
        // The routine may move to another thread, whose next instruction runs instead.
        m_idle(*this);
        if (pc != address || m_stopped) {
            return;
        }
    }
    pc = npc;
    npc += 4;
    ++instructions;
    Execute(instruction, address);
}

void MipsCpu::Branch(bool taken, std::uint32_t target, bool likely) {
    if (taken) {
        npc = target;
    } else if (likely) {
        // The delay slot of a branch likely not taken does not run.
        pc = npc;
        npc += 4;
    }
}

void MipsCpu::Execute(std::uint32_t instruction, std::uint32_t address) {
    const int rs = Rs(instruction);
    const int rt = Rt(instruction);
    const std::uint64_t immediate = SignExtend16(instruction & 0xFFFF);
    const std::uint32_t unsignedImmediate = instruction & 0xFFFF;
    const std::uint32_t base = static_cast<std::uint32_t>(gpr[rs]);
    const std::uint32_t effective = base + static_cast<std::uint32_t>(immediate);
    const std::uint32_t target = address + 4 + (static_cast<std::uint32_t>(immediate) << 2);
    const std::int64_t s = static_cast<std::int64_t>(gpr[rs]);
    switch (instruction >> 26) {
    case 0:
        Special(instruction);
        break;
    case 1:
        RegImm(instruction, address);
        break;
    case 2:
        npc = (address & 0xF0000000u) | ((instruction & 0x03FFFFFFu) << 2);
        break;
    case 3:
        gpr[MIPS_RA] = SignExtend32(address + 8);
        npc = (address & 0xF0000000u) | ((instruction & 0x03FFFFFFu) << 2);
        break;
    case 4:
        Branch(gpr[rs] == gpr[rt], target, false);
        break;
    case 5:
        Branch(gpr[rs] != gpr[rt], target, false);
        break;
    case 6:
        Branch(s <= 0, target, false);
        break;
    case 7:
        Branch(s > 0, target, false);
        break;
    case 8:
    case 9:
        gpr[rt] = SignExtend32(base + static_cast<std::uint32_t>(immediate));
        break;
    case 10:
        gpr[rt] = s < static_cast<std::int64_t>(immediate) ? 1 : 0;
        break;
    case 11:
        gpr[rt] = gpr[rs] < immediate ? 1 : 0;
        break;
    case 12:
        gpr[rt] = gpr[rs] & unsignedImmediate;
        break;
    case 13:
        gpr[rt] = gpr[rs] | unsignedImmediate;
        break;
    case 14:
        gpr[rt] = gpr[rs] ^ unsignedImmediate;
        break;
    case 15:
        gpr[rt] = SignExtend32(unsignedImmediate << 16);
        break;
    case 16:
        Cop0(instruction);
        break;
    case 17:
        Cop1(instruction, address);
        break;
    case 20:
        Branch(gpr[rs] == gpr[rt], target, true);
        break;
    case 21:
        Branch(gpr[rs] != gpr[rt], target, true);
        break;
    case 22:
        Branch(s <= 0, target, true);
        break;
    case 23:
        Branch(s > 0, target, true);
        break;
    case 24:
    case 25:
        gpr[rt] = gpr[rs] + immediate;
        break;
    case 26: {
        // LDL loads the bytes from the address to the end of its double word into the high bytes.
        std::uint32_t aligned = effective & ~7u;
        int shift = static_cast<int>(effective & 7) * 8;
        std::uint64_t word = Read64(aligned);
        std::uint64_t keep = shift == 0 ? 0 : (std::uint64_t{1} << shift) - 1;
        gpr[rt] = (gpr[rt] & keep) | (word << shift);
        break;
    }
    case 27: {
        // LDR loads the bytes from the start of its double word to the address into the low bytes.
        std::uint32_t aligned = effective & ~7u;
        int shift = (7 - static_cast<int>(effective & 7)) * 8;
        std::uint64_t word = Read64(aligned);
        std::uint64_t keep = shift == 0 ? 0 : ~(~std::uint64_t{0} >> shift);
        gpr[rt] = (gpr[rt] & keep) | (word >> shift);
        break;
    }
    case 32:
        gpr[rt] = static_cast<std::uint64_t>(static_cast<std::int8_t>(Read8(effective)));
        break;
    case 33:
        gpr[rt] = SignExtend16(Read16(effective));
        break;
    case 34: {
        // LWL loads the bytes from the address to the end of its word into the high bytes.
        std::uint32_t aligned = effective & ~3u;
        int shift = static_cast<int>(effective & 3) * 8;
        std::uint32_t word = Read32(aligned);
        std::uint32_t keep = shift == 0 ? 0 : (1u << shift) - 1;
        gpr[rt] = SignExtend32((static_cast<std::uint32_t>(gpr[rt]) & keep) | (word << shift));
        break;
    }
    case 35:
        gpr[rt] = SignExtend32(Read32(effective));
        break;
    case 36:
        gpr[rt] = Read8(effective);
        break;
    case 37:
        gpr[rt] = Read16(effective);
        break;
    case 38: {
        // LWR loads the bytes from the start of its word to the address into the low bytes.
        std::uint32_t aligned = effective & ~3u;
        int shift = (3 - static_cast<int>(effective & 3)) * 8;
        std::uint32_t word = Read32(aligned);
        std::uint32_t keep = shift == 0 ? 0 : ~(0xFFFFFFFFu >> shift);
        gpr[rt] = SignExtend32((static_cast<std::uint32_t>(gpr[rt]) & keep) | (word >> shift));
        break;
    }
    case 39:
        gpr[rt] = Read32(effective);
        break;
    case 40:
        Write8(effective, static_cast<std::uint8_t>(gpr[rt]));
        break;
    case 41:
        Write16(effective, static_cast<std::uint16_t>(gpr[rt]));
        break;
    case 42: {
        // SWL stores the high bytes of the register from the address to the end of its word.
        std::uint32_t aligned = effective & ~3u;
        int shift = static_cast<int>(effective & 3) * 8;
        std::uint32_t word = Read32(aligned);
        std::uint32_t keep = shift == 0 ? 0 : ~(0xFFFFFFFFu >> shift);
        Write32(aligned, (word & keep) | (static_cast<std::uint32_t>(gpr[rt]) >> shift));
        break;
    }
    case 43:
        Write32(effective, static_cast<std::uint32_t>(gpr[rt]));
        break;
    case 44: {
        std::uint32_t aligned = effective & ~7u;
        int shift = static_cast<int>(effective & 7) * 8;
        std::uint64_t word = Read64(aligned);
        std::uint64_t keep = shift == 0 ? 0 : ~(~std::uint64_t{0} >> shift);
        Write64(aligned, (word & keep) | (gpr[rt] >> shift));
        break;
    }
    case 45: {
        std::uint32_t aligned = effective & ~7u;
        int shift = (7 - static_cast<int>(effective & 7)) * 8;
        std::uint64_t word = Read64(aligned);
        std::uint64_t keep = shift == 0 ? 0 : (std::uint64_t{1} << shift) - 1;
        Write64(aligned, (word & keep) | (gpr[rt] << shift));
        break;
    }
    case 46: {
        // SWR stores the low bytes of the register from the start of its word to the address.
        std::uint32_t aligned = effective & ~3u;
        int shift = (3 - static_cast<int>(effective & 3)) * 8;
        std::uint32_t word = Read32(aligned);
        std::uint32_t keep = shift == 0 ? 0 : (1u << shift) - 1;
        Write32(aligned, (word & keep) | (static_cast<std::uint32_t>(gpr[rt]) << shift));
        break;
    }
    case 47:
        // CACHE: the caches are not modelled.
        break;
    case 48:
        gpr[rt] = SignExtend32(Read32(effective));
        break;
    case 49:
        fpr[rt] = Read32(effective);
        break;
    case 53:
        SetLong(rt, Read64(effective));
        break;
    case 55:
        gpr[rt] = Read64(effective);
        break;
    case 56:
        Write32(effective, static_cast<std::uint32_t>(gpr[rt]));
        gpr[rt] = 1;
        break;
    case 57:
        Write32(effective, fpr[rt]);
        break;
    case 61:
        Write64(effective, GetLong(rt));
        break;
    case 63:
        Write64(effective, gpr[rt]);
        break;
    default:
        SetFault("An unknown instruction " + Hex32(instruction));
        break;
    }
}

void MipsCpu::Special(std::uint32_t instruction) {
    const int rs = Rs(instruction);
    const int rt = Rt(instruction);
    const int rd = Rd(instruction);
    const int sa = Shift(instruction);
    const std::uint32_t s32 = static_cast<std::uint32_t>(gpr[rs]);
    const std::uint32_t t32 = static_cast<std::uint32_t>(gpr[rt]);
    switch (instruction & 63) {
    case 0:
        gpr[rd] = SignExtend32(t32 << sa);
        break;
    case 2:
        gpr[rd] = SignExtend32(t32 >> sa);
        break;
    case 3:
        gpr[rd] = SignExtend32(static_cast<std::uint32_t>(static_cast<std::int32_t>(t32) >> sa));
        break;
    case 4:
        gpr[rd] = SignExtend32(t32 << (s32 & 31));
        break;
    case 6:
        gpr[rd] = SignExtend32(t32 >> (s32 & 31));
        break;
    case 7:
        gpr[rd] =
            SignExtend32(static_cast<std::uint32_t>(static_cast<std::int32_t>(t32) >> (s32 & 31)));
        break;
    case 8:
        npc = s32;
        break;
    case 9: {
        std::uint32_t destination = s32;
        gpr[rd] = SignExtend32(pc + 4);
        npc = destination;
        break;
    }
    case 12:
        SetFault("A system call");
        break;
    case 13:
        SetFault("A break of code " + std::to_string((instruction >> 6) & 0xFFFFF));
        break;
    case 15:
        break;
    case 16:
        gpr[rd] = hi;
        break;
    case 17:
        hi = gpr[rs];
        break;
    case 18:
        gpr[rd] = lo;
        break;
    case 19:
        lo = gpr[rs];
        break;
    case 20:
        gpr[rd] = gpr[rt] << (s32 & 63);
        break;
    case 22:
        gpr[rd] = gpr[rt] >> (s32 & 63);
        break;
    case 23:
        gpr[rd] = static_cast<std::uint64_t>(static_cast<std::int64_t>(gpr[rt]) >> (s32 & 63));
        break;
    case 24: {
        std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(s32)) *
                               static_cast<std::int32_t>(t32);
        lo = SignExtend32(static_cast<std::uint32_t>(product));
        hi = SignExtend32(static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32));
        break;
    }
    case 25: {
        std::uint64_t product = static_cast<std::uint64_t>(s32) * t32;
        lo = SignExtend32(static_cast<std::uint32_t>(product));
        hi = SignExtend32(static_cast<std::uint32_t>(product >> 32));
        break;
    }
    case 26: {
        auto n = static_cast<std::int32_t>(s32);
        auto d = static_cast<std::int32_t>(t32);
        if (d == 0) {
            lo = SignExtend32(n < 0 ? 1u : 0xFFFFFFFFu);
            hi = SignExtend32(s32);
        } else if (n == std::numeric_limits<std::int32_t>::min() && d == -1) {
            lo = SignExtend32(s32);
            hi = 0;
        } else {
            lo = SignExtend32(static_cast<std::uint32_t>(n / d));
            hi = SignExtend32(static_cast<std::uint32_t>(n % d));
        }
        break;
    }
    case 27:
        if (t32 == 0) {
            lo = SignExtend32(0xFFFFFFFFu);
            hi = SignExtend32(s32);
        } else {
            lo = SignExtend32(s32 / t32);
            hi = SignExtend32(s32 % t32);
        }
        break;
    case 28: {
        MultiplyWide(gpr[rs], gpr[rt], hi, lo);
        // The signed product differs from the unsigned one in its high word, by each negative
        // factor times the other.
        if (static_cast<std::int64_t>(gpr[rs]) < 0) {
            hi -= gpr[rt];
        }
        if (static_cast<std::int64_t>(gpr[rt]) < 0) {
            hi -= gpr[rs];
        }
        break;
    }
    case 29:
        MultiplyWide(gpr[rs], gpr[rt], hi, lo);
        break;
    case 30: {
        auto n = static_cast<std::int64_t>(gpr[rs]);
        auto d = static_cast<std::int64_t>(gpr[rt]);
        if (d == 0) {
            lo = n < 0 ? 1 : ~std::uint64_t{0};
            hi = gpr[rs];
        } else if (n == std::numeric_limits<std::int64_t>::min() && d == -1) {
            lo = gpr[rs];
            hi = 0;
        } else {
            lo = static_cast<std::uint64_t>(n / d);
            hi = static_cast<std::uint64_t>(n % d);
        }
        break;
    }
    case 31:
        if (gpr[rt] == 0) {
            lo = ~std::uint64_t{0};
            hi = gpr[rs];
        } else {
            lo = gpr[rs] / gpr[rt];
            hi = gpr[rs] % gpr[rt];
        }
        break;
    case 32:
    case 33:
        gpr[rd] = SignExtend32(s32 + t32);
        break;
    case 34:
    case 35:
        gpr[rd] = SignExtend32(s32 - t32);
        break;
    case 36:
        gpr[rd] = gpr[rs] & gpr[rt];
        break;
    case 37:
        gpr[rd] = gpr[rs] | gpr[rt];
        break;
    case 38:
        gpr[rd] = gpr[rs] ^ gpr[rt];
        break;
    case 39:
        gpr[rd] = ~(gpr[rs] | gpr[rt]);
        break;
    case 42:
        gpr[rd] = static_cast<std::int64_t>(gpr[rs]) < static_cast<std::int64_t>(gpr[rt]) ? 1 : 0;
        break;
    case 43:
        gpr[rd] = gpr[rs] < gpr[rt] ? 1 : 0;
        break;
    case 44:
    case 45:
        gpr[rd] = gpr[rs] + gpr[rt];
        break;
    case 46:
    case 47:
        gpr[rd] = gpr[rs] - gpr[rt];
        break;
    case 52:
        // The compiler places TEQ after a division to catch a divisor of zero.
        if (gpr[rs] == gpr[rt]) {
            SetFault("A trap of code " + std::to_string((instruction >> 6) & 0x3FF));
        }
        break;
    case 56:
        gpr[rd] = gpr[rt] << sa;
        break;
    case 58:
        gpr[rd] = gpr[rt] >> sa;
        break;
    case 59:
        gpr[rd] = static_cast<std::uint64_t>(static_cast<std::int64_t>(gpr[rt]) >> sa);
        break;
    case 60:
        gpr[rd] = gpr[rt] << (sa + 32);
        break;
    case 62:
        gpr[rd] = gpr[rt] >> (sa + 32);
        break;
    case 63:
        gpr[rd] = static_cast<std::uint64_t>(static_cast<std::int64_t>(gpr[rt]) >> (sa + 32));
        break;
    default:
        SetFault("An unknown instruction " + Hex32(instruction));
        break;
    }
}

void MipsCpu::RegImm(std::uint32_t instruction, std::uint32_t address) {
    const int rs = Rs(instruction);
    const auto value = static_cast<std::int64_t>(gpr[rs]);
    const std::uint32_t target =
        address + 4 + (static_cast<std::uint32_t>(SignExtend16(instruction & 0xFFFF)) << 2);
    const std::uint32_t kind = (instruction >> 16) & 31;
    const bool taken = (kind & 1) != 0 ? value >= 0 : value < 0;
    if ((kind & 16) != 0) {
        gpr[MIPS_RA] = SignExtend32(address + 8);
    }
    switch (kind) {
    case 0:
    case 1:
    case 16:
    case 17:
        Branch(taken, target, false);
        break;
    case 2:
    case 3:
    case 18:
    case 19:
        Branch(taken, target, true);
        break;
    default:
        SetFault("An unknown instruction " + Hex32(instruction));
        break;
    }
}

void MipsCpu::Cop0(std::uint32_t instruction) {
    const int rt = Rt(instruction);
    const int rd = Rd(instruction);
    switch (Rs(instruction)) {
    case 0:
    case 1:
        gpr[rt] = SignExtend32(rd == COP0_COUNT ? Count() : cop0[rd]);
        break;
    case 4:
    case 5:
        cop0[rd] = static_cast<std::uint32_t>(gpr[rt]);
        break;
    case 16:
        // The instructions of the TLB do nothing, as no address is mapped through it.
        if ((instruction & 63) == 24) {
            SetFault("A return from an exception");
        }
        break;
    default:
        SetFault("An unknown instruction " + Hex32(instruction));
        break;
    }
}

void MipsCpu::Cop1(std::uint32_t instruction, std::uint32_t address) {
    const int rt = Rt(instruction);
    const int fs = Rd(instruction);
    switch (Rs(instruction)) {
    case 0:
        gpr[rt] = SignExtend32(fpr[fs]);
        break;
    case 1:
        gpr[rt] = GetLong(fs);
        break;
    case 2:
        gpr[rt] = SignExtend32(fs == 31 ? fcr31 : fs == 0 ? FCR0_VR4300 : 0);
        break;
    case 4:
        fpr[fs] = static_cast<std::uint32_t>(gpr[rt]);
        break;
    case 5:
        SetLong(fs, gpr[rt]);
        break;
    case 6:
        if (fs == 31) {
            fcr31 = static_cast<std::uint32_t>(gpr[rt]);
        }
        break;
    case 8: {
        const bool condition = (fcr31 & FCR31_CONDITION) != 0;
        const bool onTrue = (rt & 1) != 0;
        const std::uint32_t target =
            address + 4 + (static_cast<std::uint32_t>(SignExtend16(instruction & 0xFFFF)) << 2);
        Branch(condition == onTrue, target, (rt & 2) != 0);
        break;
    }
    case 16:
    case 17:
    case 20:
    case 21:
        FloatOperation(instruction);
        break;
    default:
        SetFault("An unknown instruction " + Hex32(instruction));
        break;
    }
}

void MipsCpu::FloatOperation(std::uint32_t instruction) {
    const int format = Rs(instruction);
    const int ft = Rt(instruction);
    const int fs = Rd(instruction);
    const int fd = Shift(instruction);
    const std::uint32_t function = instruction & 63;
    const std::uint32_t mode = fcr31 & FCR31_ROUNDING;
    // The value of the source in double precision, whatever its format.
    double source = 0.0;
    switch (format) {
    case 16:
        source = GetSingle(fs);
        break;
    case 17:
        source = GetDouble(fs);
        break;
    case 20:
        source = static_cast<std::int32_t>(fpr[fs]);
        break;
    default:
        source = static_cast<double>(static_cast<std::int64_t>(GetLong(fs)));
        break;
    }
    if (function >= 48) {
        bool holds = format == 16 ? Compare(GetSingle(fs), GetSingle(ft), function & 15) :
                                    Compare(GetDouble(fs), GetDouble(ft), function & 15);
        fcr31 = holds ? fcr31 | FCR31_CONDITION : fcr31 & ~FCR31_CONDITION;
        return;
    }
    if (function >= 8 && function <= 15) {
        // ROUND, TRUNC, CEIL, and FLOOR, to a long and then to a word.
        std::uint32_t rounding = function & 3;
        if (function < 12) {
            SetLong(fd, ToLong(source, rounding));
        } else {
            fpr[fd] = ToWord(source, rounding);
        }
        return;
    }
    switch (function) {
    case 32:
        // CVT.S rounds a double, a word, or a long to the single-precision value nearest.
        if (format == 17) {
            SetSingle(fd, Flush(static_cast<float>(GetDouble(fs)), fcr31));
        } else if (format == 20) {
            SetSingle(fd, static_cast<float>(static_cast<std::int32_t>(fpr[fs])));
        } else {
            SetSingle(fd, static_cast<float>(static_cast<std::int64_t>(GetLong(fs))));
        }
        return;
    case 33:
        if (format == 21) {
            SetDouble(fd, static_cast<double>(static_cast<std::int64_t>(GetLong(fs))));
        } else {
            SetDouble(fd, source);
        }
        return;
    case 36:
        fpr[fd] = ToWord(source, mode);
        return;
    case 37:
        SetLong(fd, ToLong(source, mode));
        return;
    default:
        break;
    }
    if (format == 16) {
        float a = GetSingle(fs);
        float b = GetSingle(ft);
        float result = 0.0f;
        switch (function) {
        case 0:
            result = a + b;
            break;
        case 1:
            result = a - b;
            break;
        case 2:
            result = a * b;
            break;
        case 3:
            result = a / b;
            break;
        case 4:
            result = std::sqrt(a);
            break;
        case 5:
            result = std::fabs(a);
            break;
        case 6:
            fpr[fd] = fpr[fs];
            return;
        case 7:
            result = -a;
            break;
        default:
            SetFault("An unknown instruction " + Hex32(instruction));
            return;
        }
        SetSingle(fd, Flush(result, fcr31));
        return;
    }
    if (format == 17) {
        double a = GetDouble(fs);
        double b = GetDouble(ft);
        double result = 0.0;
        switch (function) {
        case 0:
            result = a + b;
            break;
        case 1:
            result = a - b;
            break;
        case 2:
            result = a * b;
            break;
        case 3:
            result = a / b;
            break;
        case 4:
            result = std::sqrt(a);
            break;
        case 5:
            result = std::fabs(a);
            break;
        case 6:
            SetLong(fd, GetLong(fs));
            return;
        case 7:
            result = -a;
            break;
        default:
            SetFault("An unknown instruction " + Hex32(instruction));
            return;
        }
        SetDouble(fd, Flush(result, fcr31));
        return;
    }
    SetFault("An unknown instruction " + Hex32(instruction));
}

} // namespace coastline
