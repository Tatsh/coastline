#pragma once

/**
 * @file
 * An interpreter of the VR4300 processor of the Nintendo 64, for the import only. It runs the code
 * of a cartridge from its RDRAM with the floating-point unit in the 32-bit register mode the
 * compiled code of the cartridges expects. It has no TLB, no caches, no interrupts, and no other
 * part of the console. The addresses of the registers of the console call a handler, and a hook on
 * the address of a routine takes the place of the routine.
 */

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace coastline {

/** The numbers of the general registers the calling convention gives a role. */
enum MipsRegister : int {
    MIPS_ZERO = 0, /*!< Always zero. */
    MIPS_V0 = 2,   /*!< The first word of a return value. */
    MIPS_V1 = 3,   /*!< The second word of a return value. */
    MIPS_A0 = 4,   /*!< The first argument. */
    MIPS_A1 = 5,   /*!< The second argument. */
    MIPS_A2 = 6,   /*!< The third argument. */
    MIPS_A3 = 7,   /*!< The fourth argument. */
    MIPS_SP = 29,  /*!< The stack pointer. */
    MIPS_RA = 31,  /*!< The return address. */
};

/** The VR4300 processor and the RDRAM it runs from. */
class MipsCpu {
public:
    /**
     * What a hook did.
     *
     * Run the instruction at the address of the hook, as if the hook were not there, or skip it
     * because the hook moved the program counter, as Return() does.
     */
    enum class HookResult { Continue, Jumped };

    /** A routine called before the instruction at the address it is set on. */
    using Hook = std::function<HookResult(MipsCpu &)>;

    /** A read of the registers of the console: the physical address and the size in bytes. */
    using DeviceRead = std::function<std::uint64_t(std::uint32_t address, int size)>;

    /** A write to the registers of the console: the physical address, the size, and the value. */
    using DeviceWrite = std::function<void(std::uint32_t address, int size, std::uint64_t value)>;

    /**
     * Make a processor with zeroed RDRAM.
     *
     * @param ramSize The bytes of RDRAM, a multiple of 4.
     */
    explicit MipsCpu(std::size_t ramSize);

    /**
     * Set the cartridge image the physical addresses from 0x10000000 read.
     *
     * @param rom The image, in the byte order of the console. It must remain valid while the
     * processor runs.
     */
    void SetRom(std::span<const std::uint8_t> rom);

    /**
     * Set the routine called before the instruction at an address of RDRAM.
     *
     * @param address The virtual address, in KSEG0.
     * @param hook The routine.
     */
    void SetHook(std::uint32_t address, Hook hook);

    /**
     * Set the routines the registers of the console are read and written through. Without them, a
     * read gives zero and a write is discarded.
     *
     * @param read The routine of the reads.
     * @param write The routine of the writes.
     */
    void SetDevices(DeviceRead read, DeviceWrite write);

    /**
     * Set the routine called when the processor spins on a branch to itself, as an idle thread
     * does.
     *
     * @param idle The routine.
     */
    void SetIdle(std::function<void(MipsCpu &)> idle);

    /**
     * Run instructions until Stop() or a fault.
     *
     * @param budget The most instructions to run.
     * @return Whether the processor stopped by Stop(), rather than by a fault or the budget.
     */
    bool Run(std::uint64_t budget);

    /** Stop Run() before the next instruction. */
    void Stop();

    /**
     * Return from the routine a hook is on, as its last instruction would.
     *
     * @param value The value in v0, sign-extended from 32 bits.
     */
    void Return(std::uint32_t value);

    /**
     * Set the program counter, with no branch pending.
     *
     * @param address The address of the next instruction.
     */
    void Jump(std::uint32_t address);

    /** The fault that ended Run(), or an empty string. */
    const std::string &Fault() const {
        return m_fault;
    }

    /**
     * Read a byte of memory, through the map of the console.
     *
     * @param address The virtual address.
     * @return The byte, or zero after a fault.
     */
    std::uint8_t Read8(std::uint32_t address);

    /**
     * Read a big-endian half-word of memory.
     *
     * @param address The virtual address, a multiple of 2.
     * @return The half-word.
     */
    std::uint16_t Read16(std::uint32_t address);

    /**
     * Read a big-endian word of memory.
     *
     * @param address The virtual address, a multiple of 4.
     * @return The word.
     */
    std::uint32_t Read32(std::uint32_t address);

    /**
     * Read a big-endian double word of memory.
     *
     * @param address The virtual address, a multiple of 8.
     * @return The double word.
     */
    std::uint64_t Read64(std::uint32_t address);

    /**
     * Write a byte of memory.
     *
     * @param address The virtual address.
     * @param value The byte.
     */
    void Write8(std::uint32_t address, std::uint8_t value);

    /**
     * Write a big-endian half-word of memory.
     *
     * @param address The virtual address, a multiple of 2.
     * @param value The half-word.
     */
    void Write16(std::uint32_t address, std::uint16_t value);

    /**
     * Write a big-endian word of memory.
     *
     * @param address The virtual address, a multiple of 4.
     * @param value The word.
     */
    void Write32(std::uint32_t address, std::uint32_t value);

    /**
     * Write a big-endian double word of memory.
     *
     * @param address The virtual address, a multiple of 8.
     * @param value The double word.
     */
    void Write64(std::uint32_t address, std::uint64_t value);

    /**
     * Read a single-precision value of memory.
     *
     * @param address The virtual address, a multiple of 4.
     * @return The value.
     */
    float ReadFloat(std::uint32_t address);

    /**
     * Read an argument of the routine a hook is on, from a0 to a3 and then from the stack.
     *
     * @param index The argument, from 0.
     * @return The low word of the argument.
     */
    std::uint32_t Argument(int index);

    /** The RDRAM. */
    std::vector<std::uint8_t> ram;
    /** The general registers. */
    std::uint64_t gpr[32] = {};
    /** The high word of a product or the remainder of a division. */
    std::uint64_t hi = 0;
    /** The low word of a product or the quotient of a division. */
    std::uint64_t lo = 0;
    /** The floating-point registers, as 32-bit halves. A double is the pair from an even one. */
    std::uint32_t fpr[32] = {};
    /** The control and status register of the floating-point unit. */
    std::uint32_t fcr31 = 0;
    /** The registers of the system control processor. Count is computed from the instructions. */
    std::uint32_t cop0[32] = {};
    /** The address of the next instruction. */
    std::uint32_t pc = 0;
    /** The address of the instruction after it, the target of a branch taken. */
    std::uint32_t npc = 4;
    /** The instructions run. */
    std::uint64_t instructions = 0;
    /** The ticks of Count added to those of the instructions, as time spent idle. */
    std::uint64_t idleTicks = 0;

    /** The ticks of Count since power on, one for two instructions plus the idle time. */
    std::uint64_t Ticks() const {
        return instructions / 2 + idleTicks;
    }

    /** The value of Count, the low word of Ticks(). */
    std::uint32_t Count() const {
        return static_cast<std::uint32_t>(Ticks());
    }

private:
    void Step();
    void Execute(std::uint32_t instruction, std::uint32_t address);
    void Special(std::uint32_t instruction);
    void RegImm(std::uint32_t instruction, std::uint32_t address);
    void Cop0(std::uint32_t instruction);
    void Cop1(std::uint32_t instruction, std::uint32_t address);
    void FloatOperation(std::uint32_t instruction);
    void Branch(bool taken, std::uint32_t target, bool likely);
    void SetFault(const std::string &fault);
    std::uint8_t *RamAt(std::uint32_t address, int size);
    std::uint64_t ReadDevice(std::uint32_t address, int size);
    void WriteDevice(std::uint32_t address, int size, std::uint64_t value);
    float GetSingle(int reg) const;
    double GetDouble(int reg) const;
    void SetSingle(int reg, float value);
    void SetDouble(int reg, double value);
    std::uint64_t GetLong(int reg) const;
    void SetLong(int reg, std::uint64_t value);

    std::span<const std::uint8_t> m_rom;
    std::unordered_map<std::uint32_t, Hook> m_hooks;
    std::vector<std::uint8_t> m_hooked;
    DeviceRead m_deviceRead;
    DeviceWrite m_deviceWrite;
    std::function<void(MipsCpu &)> m_idle;
    std::string m_fault;
    bool m_stopped = false;
    std::uint32_t m_current = 0;
};

} // namespace coastline
