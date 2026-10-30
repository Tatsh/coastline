// Host check of the interpreter of the VR4300 the import records the attract demos of the
// cartridges with. Each program is hand-assembled here and run from RDRAM.

#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>

#include "mipscpu.h"

namespace {

using coastline::MipsCpu;

int g_failures = 0;

void Check(bool condition, const char *description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++g_failures;
    }
}

constexpr std::uint32_t PROGRAM = 0x80001000u;
constexpr std::uint32_t DATA = 0x80002000u;
// The return address of a program, where a hook stops the processor.
constexpr std::uint32_t DONE = 0x80000800u;

enum Reg : std::uint32_t {
    ZERO = 0,
    AT = 1,
    V0 = 2,
    V1 = 3,
    A0 = 4,
    A1 = 5,
    T0 = 8,
    T1 = 9,
    T2 = 10,
    T3 = 11,
    S0 = 16,
    RA = 31,
};

std::uint32_t I(std::uint32_t op, std::uint32_t rs, std::uint32_t rt, std::int32_t immediate) {
    return op << 26 | rs << 21 | rt << 16 | (static_cast<std::uint32_t>(immediate) & 0xFFFF);
}

std::uint32_t
R(std::uint32_t rs, std::uint32_t rt, std::uint32_t rd, std::uint32_t sa, std::uint32_t function) {
    return rs << 21 | rt << 16 | rd << 11 | sa << 6 | function;
}

std::uint32_t F(std::uint32_t format,
                std::uint32_t ft,
                std::uint32_t fs,
                std::uint32_t fd,
                std::uint32_t function) {
    return 17u << 26 | format << 21 | ft << 16 | fs << 11 | fd << 6 | function;
}

std::uint32_t Jal(std::uint32_t target) {
    return 3u << 26 | ((target >> 2) & 0x03FFFFFF);
}

constexpr std::uint32_t NOP = 0;
constexpr std::uint32_t JR_RA = 31u << 21 | 8;
constexpr std::uint32_t SINGLE = 16;
constexpr std::uint32_t DOUBLE = 17;
constexpr std::uint32_t WORD = 20;

std::uint32_t Lui(std::uint32_t rt, std::uint32_t value) {
    return I(15, 0, rt, static_cast<std::int32_t>(value));
}

std::uint32_t Addiu(std::uint32_t rt, std::uint32_t rs, std::int32_t value) {
    return I(9, rs, rt, value);
}

std::uint32_t Ori(std::uint32_t rt, std::uint32_t rs, std::uint32_t value) {
    return I(13, rs, rt, static_cast<std::int32_t>(value));
}

std::uint32_t Mtc1(std::uint32_t rt, std::uint32_t fs) {
    return 17u << 26 | 4u << 21 | rt << 16 | fs << 11;
}

std::uint32_t Mfc1(std::uint32_t rt, std::uint32_t fs) {
    return 17u << 26 | rt << 16 | fs << 11;
}

// Run a program that ends with jr ra, from a processor of zeroed registers.
void Run(MipsCpu &cpu, const std::vector<std::uint32_t> &program) {
    for (std::size_t i = 0; i < program.size(); ++i) {
        cpu.Write32(PROGRAM + static_cast<std::uint32_t>(i) * 4, program[i]);
    }
    cpu.SetHook(DONE, [](MipsCpu &stopped) {
        stopped.Stop();
        return MipsCpu::HookResult::Jumped;
    });
    cpu.gpr[RA] = static_cast<std::uint64_t>(static_cast<std::int32_t>(DONE));
    cpu.Jump(PROGRAM);
    Check(cpu.Run(10000), "the program returns");
    if (!cpu.Fault().empty()) {
        std::cerr << cpu.Fault() << '\n';
    }
}

void TestIntegers() {
    MipsCpu cpu(0x10000);
    Run(cpu,
        {Lui(T0, 0x8000),
         Ori(T1, ZERO, 0xFFFF),
         Addiu(T2, ZERO, -3),
         Addiu(T3, ZERO, 5),
         R(T2, T3, 0, 0, 24), // mult
         R(0, 0, V0, 0, 18),  // mflo
         R(0, 0, V1, 0, 16),  // mfhi
         Addiu(A0, ZERO, -7),
         Addiu(A1, ZERO, 2),
         R(A0, A1, 0, 0, 26), // div
         R(0, 0, S0, 0, 18),  // mflo
         R(0, 0, 17, 0, 16),  // mfhi into s1
         R(0, T0, 18, 4, 2),  // srl s2, t0, 4
         R(0, T0, 19, 4, 3),  // sra s3, t0, 4
         JR_RA,
         NOP});
    Check(cpu.gpr[T0] == 0xFFFFFFFF80000000ull, "lui sign-extends");
    Check(cpu.gpr[T1] == 0xFFFF, "ori zero-extends");
    Check(static_cast<std::int64_t>(cpu.gpr[V0]) == -15, "mult low word");
    Check(static_cast<std::int64_t>(cpu.gpr[V1]) == -1, "mult high word");
    Check(static_cast<std::int64_t>(cpu.gpr[S0]) == -3, "div quotient truncates");
    Check(static_cast<std::int64_t>(cpu.gpr[17]) == -1,
          "div remainder takes the sign of the dividend");
    Check(cpu.gpr[18] == 0x08000000, "srl of a word");
    Check(cpu.gpr[19] == 0xFFFFFFFFF8000000ull, "sra of a word");
}

void TestBranches() {
    MipsCpu cpu(0x10000);
    constexpr std::uint32_t SUBROUTINE = PROGRAM + 0x100;
    // The subroutine sets v1 to 7 in the delay slot of its jr.
    cpu.Write32(SUBROUTINE, JR_RA);
    cpu.Write32(SUBROUTINE + 4, Addiu(V1, ZERO, 7));
    Run(cpu,
        {Addiu(S0, RA, 0),
         I(4, ZERO, ZERO, 2),  // beq zero, zero, +2: taken
         Addiu(T0, ZERO, 1),   // the delay slot runs
         Addiu(T0, ZERO, 99),  // skipped
         I(21, ZERO, ZERO, 2), // bnel zero, zero: not taken
         Addiu(T1, ZERO, 1),   // the delay slot of a branch likely not taken does not run
         Jal(SUBROUTINE),
         Addiu(T2, ZERO, 3), // the delay slot of jal runs before the call
         Addiu(RA, S0, 0),
         JR_RA,
         NOP});
    Check(cpu.gpr[T0] == 1, "a taken branch runs its delay slot and skips its fall-through");
    Check(cpu.gpr[T1] == 0, "a branch likely not taken skips its delay slot");
    Check(cpu.gpr[T2] == 3 && cpu.gpr[V1] == 7, "jal and jr run their delay slots");
}

void TestUnaligned() {
    MipsCpu cpu(0x10000);
    cpu.Write32(DATA, 0x11223344);
    cpu.Write32(DATA + 4, 0x55667788);
    Run(cpu,
        {Lui(A0, DATA >> 16),
         Ori(A0, A0, DATA & 0xFFFF),
         I(34, A0, T0, 1), // lwl t0, 1(a0)
         I(38, A0, T0, 4), // lwr t0, 4(a0)
         Lui(T1, 0xAABB),
         Ori(T1, T1, 0xCCDD),
         I(42, A0, T1, 2), // swl t1, 2(a0)
         I(46, A0, T1, 5), // swr t1, 5(a0)
         JR_RA,
         NOP});
    Check(static_cast<std::uint32_t>(cpu.gpr[T0]) == 0x22334455, "lwl and lwr load across words");
    Check(cpu.Read32(DATA) == 0x1122AABB && cpu.Read32(DATA + 4) == 0xCCDD7788,
          "swl and swr store across words");
}

void TestFloats() {
    MipsCpu cpu(0x10000);
    cpu.fcr31 = 0;
    Run(cpu,
        {// A double built from its halves, the high one in the odd register: 1.5.
         Lui(AT, 0x3FF8),
         Mtc1(AT, 1),
         Mtc1(ZERO, 0),
         F(DOUBLE, 0, 0, 2, 32), // cvt.s.d f2, f0
         F(SINGLE, 2, 2, 3, 2),  // mul.s f3, f2, f2: 2.25
         Lui(AT, 0xC02C),        // -2.6875
         Mtc1(AT, 4),
         F(SINGLE, 0, 4, 5, 13), // trunc.w.s f5, f4: -2
         Mfc1(T0, 5),
         Lui(AT, 0x4020), // 2.5
         Mtc1(AT, 6),
         F(SINGLE, 0, 6, 7, 36), // cvt.w.s f7, f6: 2 when rounding to the nearest even
         Mfc1(T1, 7),
         F(SINGLE, 3, 2, 0, 60),              // c.lt.s f2, f3
         17u << 26 | 8u << 21 | 1u << 16 | 2, // bc1t +2
         Addiu(T2, ZERO, 5),                  // the delay slot runs
         Addiu(T2, ZERO, 9),                  // skipped
         JR_RA,
         NOP});
    Check(std::bit_cast<float>(cpu.fpr[2]) == 1.5f, "a double from two halves converts to single");
    Check(std::bit_cast<float>(cpu.fpr[3]) == 2.25f, "mul.s");
    Check(static_cast<std::int32_t>(cpu.gpr[T0]) == -2, "trunc.w.s rounds towards zero");
    Check(static_cast<std::int32_t>(cpu.gpr[T1]) == 2, "cvt.w.s rounds to the nearest even");
    Check(cpu.gpr[T2] == 5, "bc1t follows c.lt.s");
}

void TestFlush() {
    for (bool flush : {false, true}) {
        MipsCpu cpu(0x10000);
        cpu.fcr31 = flush ? 0x01000000u : 0;
        cpu.fpr[0] = std::bit_cast<std::uint32_t>(1e-30f);
        cpu.fpr[1] = std::bit_cast<std::uint32_t>(1e-10f);
        Run(cpu, {F(SINGLE, 1, 0, 2, 2), JR_RA, NOP});
        float product = std::bit_cast<float>(cpu.fpr[2]);
        Check(flush ? product == 0.0f : product != 0.0f,
              flush ? "a result too small to be normal flushes to zero" :
                      "a result too small to be normal is retained without the flush bit");
    }
}

void TestDoubleMemory() {
    MipsCpu cpu(0x10000);
    cpu.Write64(DATA, std::bit_cast<std::uint64_t>(1.25));
    Run(cpu,
        {Lui(A0, DATA >> 16),
         Ori(A0, A0, DATA & 0xFFFF),
         I(53, A0, 4, 0),       // ldc1 f4, 0(a0)
         F(DOUBLE, 4, 4, 6, 0), // add.d f6, f4, f4
         I(61, A0, 6, 8),       // sdc1 f6, 8(a0)
         Mfc1(T0, 5),
         JR_RA,
         NOP});
    Check(std::bit_cast<double>(cpu.Read64(DATA + 8)) == 2.5, "ldc1, add.d, and sdc1");
    Check(static_cast<std::uint32_t>(cpu.gpr[T0]) == 0x3FF40000,
          "the odd register of a double has its high word");
}

void TestHooks() {
    MipsCpu cpu(0x10000);
    constexpr std::uint32_t ROUTINE = PROGRAM + 0x200;
    cpu.SetHook(ROUTINE, [](MipsCpu &hooked) {
        hooked.Return(hooked.Argument(0) + 40);
        return MipsCpu::HookResult::Jumped;
    });
    int idles = 0;
    cpu.SetIdle([&idles](MipsCpu &idle) {
        // The spin ends by moving to the next instruction, as a move to another thread would.
        if (++idles == 3) {
            idle.Jump(idle.pc + 8);
        }
    });
    Run(cpu,
        {Addiu(S0, RA, 0),
         Jal(ROUTINE),
         Addiu(A0, ZERO, 2),
         I(4, ZERO, ZERO, -1), // beq zero, zero, -1: the idle spin
         NOP,
         Addiu(RA, S0, 0),
         JR_RA,
         NOP});
    Check(cpu.gpr[V0] == 42, "a hook takes the place of a routine");
    Check(idles == 3, "a branch to itself calls the idle routine");
}

} // namespace

int main() {
    TestIntegers();
    TestBranches();
    TestUnaligned();
    TestFloats();
    TestFlush();
    TestDoubleMemory();
    TestHooks();
    if (g_failures != 0) {
        std::cerr << g_failures << " checks failed\n";
        return 1;
    }
    return 0;
}
