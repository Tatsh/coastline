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
    enum class HookResult {
        Continue, /*!< Execute the hooked instruction normally. */
        Jumped    /*!< Skip the instruction after the hook changed the program counter. */
    };

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

    /**
     * Report why Run() ended with a fault. The first fault is recorded and is never cleared or
     * replaced by a later fault.
     *
     * @return The fault with the address of its instruction, or empty text without a fault.
     */
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

    /**
     * Count the ticks of the Count register since power on.
     *
     * @return One tick for every two instructions run, plus the idle ticks.
     */
    std::uint64_t Ticks() const {
        return instructions / 2 + idleTicks;
    }

    /**
     * Read the Count register of the system control processor.
     *
     * @return The low word of Ticks().
     */
    std::uint32_t Count() const {
        return static_cast<std::uint32_t>(Ticks());
    }

private:
    // Fetch and run the instruction at the program counter. A branch to itself calls the idle
    // routine first, and the instruction is skipped when the routine moves the program counter or
    // stops the processor.
    void Step();
    // Decode an instruction by its major opcode and run it. The address is the virtual address of
    // the instruction, for the targets of branches.
    void Execute(std::uint32_t instruction, std::uint32_t address);
    // Run an instruction of the SPECIAL opcode, decoded by its function field.
    void Special(std::uint32_t instruction);
    // Run a REGIMM branch on the sign of a register, with link and likely forms. Other forms fault.
    void RegImm(std::uint32_t instruction, std::uint32_t address);
    // Run a move to or from the system control processor. Count reads from Ticks(), the TLB
    // instructions do nothing, and ERET faults.
    void Cop0(std::uint32_t instruction);
    // Run a move or a branch of the floating-point unit, or pass an arithmetic instruction to
    // FloatOperation().
    void Cop1(std::uint32_t instruction, std::uint32_t address);
    // Run an arithmetic, conversion, or comparison instruction of the floating-point unit in the
    // single, double, word, or long format, with the rounding mode of fcr31.
    void FloatOperation(std::uint32_t instruction);
    // Resolve a branch. A taken branch sets the target after the delay slot, and a branch likely
    // not taken skips its delay slot.
    void Branch(bool taken, std::uint32_t target, bool likely);
    // Record a fault with the address of the current instruction, unless a fault is already
    // recorded. Run() stops after the instruction.
    void SetFault(const std::string &fault);
    // Find bytes of RDRAM through KSEG0 or KSEG1. Returns nullptr when the address is outside
    // KSEG0 and KSEG1 or the access passes the end of RDRAM.
    std::uint8_t *RamAt(std::uint32_t address, int size);
    // Read an address outside RDRAM. The cartridge image supplies 0x10000000 to 0x1FBFFFFF, with
    // zero past its end, and the read routine supplies the other physical addresses. A mapped
    // address faults. Returns the big-endian value, or zero after a fault or without a read
    // routine.
    std::uint64_t ReadDevice(std::uint32_t address, int size);
    // Write an address outside RDRAM through the write routine. A mapped address faults. Without
    // the routine the write is discarded.
    void WriteDevice(std::uint32_t address, int size, std::uint64_t value);
    // Read the bits of a floating-point register as a float.
    float GetSingle(int reg) const;
    // Read a pair of floating-point registers as a double. The low bit of the register is ignored.
    double GetDouble(int reg) const;
    void SetSingle(int reg, float value);
    // Write a double to a pair of floating-point registers. The low bit of the register is ignored.
    void SetDouble(int reg, double value);
    // Read a pair of floating-point registers as 64 bits, with the odd register in the high word
    // and the even register in the low word. The low bit of the register is ignored.
    std::uint64_t GetLong(int reg) const;
    // Write 64 bits to a pair of floating-point registers, with the high word to the odd register
    // and the low word to the even register. The low bit of the register is ignored.
    void SetLong(int reg, std::uint64_t value);

    std::span<const std::uint8_t> m_rom;             // The cartridge image from SetRom().
    std::unordered_map<std::uint32_t, Hook> m_hooks; // The hooks by physical address.
    // A flag for each word of RDRAM, nonzero where a hook is set, checked before each lookup.
    std::vector<std::uint8_t> m_hooked;
    DeviceRead m_deviceRead;               // The read routine from SetDevices(), or empty.
    DeviceWrite m_deviceWrite;             // The write routine from SetDevices(), or empty.
    std::function<void(MipsCpu &)> m_idle; // The routine from SetIdle(), or empty.
    std::string m_fault;                   // The first fault, or empty text.
    bool m_stopped = false;                // Set by Stop() and cleared by Run().
    std::uint32_t m_current = 0;           // The address of the instruction Step() runs.
};

} // namespace coastline
