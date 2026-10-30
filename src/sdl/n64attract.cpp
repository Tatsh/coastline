// The threads, the message queues, and the devices of libultra are replaced by routines of the
// host at the addresses of their entry points, and the code of the game around them runs as it
// is. A thread runs until it blocks on a queue or a thread of a higher priority becomes runnable,
// as in libultra. The events of the devices (the retrace, the end of a task of the RSP, the end of
// a read of the controllers) arrive when every thread waits, as an interrupt would send them.
#include "n64attract.h"

#include <algorithm>
#include <cstring>
#include <optional>

#include <fmt/format.h>

#include "mipscpu.h"

namespace coastline {

namespace {

// The entry points of libultra and of the game that the recording replaces or watches.
struct DemoSymbols {
    std::uint32_t osInitialize;
    std::uint32_t osCreateThread;
    std::uint32_t osStartThread;
    std::uint32_t osSetThreadPri;
    std::uint32_t osGetThreadPri;
    std::uint32_t osYieldThread;
    std::uint32_t osDestroyThread;
    std::uint32_t osRecvMesg;
    std::uint32_t osSendMesg;
    std::uint32_t osJamMesg;
    std::uint32_t osSetEventMesg;
    std::uint32_t osCreateViManager;
    std::uint32_t osViSetMode;
    std::uint32_t osViBlack;
    std::uint32_t osViSetSpecialFeatures;
    std::uint32_t osViSwapBuffer;
    std::uint32_t osViSetEvent;
    std::uint32_t osViGetCurrentFramebuffer;
    std::uint32_t osCreatePiManager;
    std::uint32_t osPiStartDma;
    std::uint32_t osPiRawStartDma;
    std::uint32_t osEPiRawStartDma;
    std::uint32_t osPiRawReadIo;
    std::uint32_t osEPiRawReadIo;
    std::uint32_t osEPiRawWriteIo;
    std::uint32_t osCartRomInit;
    std::uint32_t osSpTaskLoad;
    std::uint32_t osSpTaskStartGo;
    std::uint32_t osContInit;
    std::uint32_t osContStartReadData;
    std::uint32_t osContGetReadData;
    std::uint32_t osPfsInit;
    std::uint32_t osPfsInitPak;
    std::uint32_t osPfsIsPlug;
    std::uint32_t osPfsFindFile;
    std::uint32_t osPfsReadWriteFile;
    std::uint32_t osPfsAllocateFile;
    std::uint32_t osPfsDeleteFile;
    std::uint32_t osPfsFreeBlocks;
    std::uint32_t osPfsNumFiles;
    std::uint32_t osPfsFileState;
    std::uint32_t osPfsRepairId;
    std::uint32_t osPfsChecker;
    std::uint32_t osMotorInit;
    std::uint32_t osMotorStart;
    std::uint32_t osMotorStop;
    std::uint32_t osEepromProbe;
    std::uint32_t osEepromRead;
    std::uint32_t osEepromWrite;
    std::uint32_t osEepromLongRead;
    std::uint32_t osEepromLongWrite;
    std::uint32_t osGetCount;
    std::uint32_t osGetTime;
    std::uint32_t osSetTimer;
    std::uint32_t osInvalDCache;
    std::uint32_t osInvalICache;
    std::uint32_t osWritebackDCache;
    std::uint32_t osWritebackDCacheAll;
    std::uint32_t osVirtualToPhysical;
    std::uint32_t osSetIntMask;
    std::uint32_t osAiSetFrequency;
    std::uint32_t osAiSetNextBuffer;
    std::uint32_t osAiGetStatus;
    std::uint32_t osAiGetLength;
    std::uint32_t osSyncPrintf;
    std::uint32_t osMapTLBRdb;
    std::uint32_t processRacingScreen;
    std::uint32_t startAttractDemo;
    std::uint32_t processSplashScreens;
    std::uint32_t vehicles;
    std::uint32_t raceInitialized;
    std::uint32_t selectedTrack;
};

// The Japanese and the North American releases have no EEPROM routines.
constexpr DemoSymbols USA_SYMBOLS{
    0x80265D90, 0x80265670, 0x802657C0, 0x802668F0, 0x8026C040, 0x8026E820, 0x8026E500, 0x802642E0,
    0x802654B0, 0x8026B5D0, 0x80265440, 0x80266400, 0x80264A60, 0x80260AB0, 0x80264490, 0x80264AD0,
    0x80265600, 0x80264B20, 0x80266760, 0x80264650, 0x8026C680, 0x8026C760, 0x802663A0, 0x8026E780,
    0x8026E730, 0x8026C3D0, 0x80264C7C, 0x80264E0C, 0x80265910, 0x80264760, 0x80264824, 0x80265CD0,
    0x80261CB0, 0x80261940, 0x802628C0, 0x802635DC, 0x80263B30, 0x80267C20, 0x802639E0, 0x802677E0,
    0x80267930, 0x80262660, 0x80262A80, 0x80262370, 0x80262088, 0x80261F20, 0,          0,
    0,          0,          0,          0x802649F0, 0x8026B960, 0x80268390, 0x802662E0, 0x8026BAC0,
    0x8026B7D0, 0x802649C0, 0x8026B750, 0x8026E460, 0x80268230, 0x80268470, 0x80268520, 0x80268530,
    0x802607DC, 0x8026BB40, 0x8020082C, 0x802005FC, 0x8020686C, 0x8031B760, 0x802707C8, 0x8028B940,
};

constexpr DemoSymbols EUR_SYMBOLS{
    0x80267520, 0x80266E00, 0x80266F50, 0x802680E0, 0x8026E010, 0x802707F0, 0x802704D0, 0x80265A60,
    0x80266C40, 0x8026D5A0, 0x80266BD0, 0x80267B90, 0x80265DD0, 0x80265E40, 0x80265C10, 0x80266260,
    0x80266D90, 0x802662B0, 0x80267F50, 0x80265EB0, 0x8026E650, 0x8026E730, 0x80267B30, 0x80270750,
    0x80270700, 0x8026E3A0, 0x8026640C, 0x8026659C, 0x802670A0, 0x80265FC0, 0x80266084, 0x80267460,
    0x80263430, 0x802630C0, 0x80264040, 0x80264D5C, 0x802652B0, 0x80269410, 0x80265160, 0x80268FD0,
    0x80269120, 0x80263DE0, 0x80264200, 0x80263AF0, 0x80263808, 0x802636A0, 0x80262DD0, 0x8026B020,
    0x8026AB40, 0x80262E40, 0x80262F80, 0x80266250, 0x8026D930, 0x80269B80, 0x80267A70, 0x8026DA90,
    0x8026D7A0, 0x80266220, 0x8026D720, 0x80270430, 0x80269A20, 0x80269C60, 0x80269D10, 0x80269D20,
    0x80261CDC, 0x8026DB10, 0x8020085C, 0x8020062C, 0x80206A2C, 0x8031DC60, 0x8027279C, 0x8028DBF0,
};

constexpr DemoSymbols JPN_SYMBOLS{
    0x80267020, 0x80266900, 0x80266A50, 0x80267B80, 0x8026DAC0, 0x802702A0, 0x8026FF80, 0x80265500,
    0x80266740, 0x8026D050, 0x802666D0, 0x80267690, 0x80265CF0, 0x80265870, 0x802656B0, 0x80265D60,
    0x80266890, 0x80265DB0, 0x802679F0, 0x802658E0, 0x8026E100, 0x8026E1E0, 0x80267630, 0x80270200,
    0x802701B0, 0x8026DE50, 0x80265F0C, 0x8026609C, 0x80266BA0, 0x802659F0, 0x80265AB4, 0x80266F60,
    0x80262ED0, 0x80262B60, 0x80263AE0, 0x802647FC, 0x80264D50, 0x80268EC0, 0x80264C00, 0x80268A80,
    0x80268BD0, 0x80263880, 0x80263CA0, 0x80263590, 0x802632A8, 0x80263140, 0x80262870, 0x8026AAD0,
    0x8026A5F0, 0x802628E0, 0x80262A20, 0x80265C80, 0x8026D3E0, 0x80269630, 0x80267570, 0x8026D540,
    0x8026D250, 0x80265C50, 0x8026D1D0, 0x8026FEE0, 0x802694D0, 0x80269710, 0x802697C0, 0x802697D0,
    0x80261A3C, 0x8026D5C0, 0x8020081C, 0x802005EC, 0x80206AB4, 0x8031C890, 0x8027224C, 0x8028D620,
};

// The cartridge and the console at power on, as the boot code sets them up.
constexpr std::size_t RAM_SIZE = 0x800000;
constexpr std::uint32_t MEMORY_SIZE = 0x400000;
constexpr std::uint32_t ENTRY = 0x80200000u;
constexpr std::size_t BOOT_COPY_ROM = 0x1000;
constexpr std::size_t BOOT_COPY_SIZE = 0x100000;
constexpr std::uint32_t OS_TV_TYPE = 0x80000300u;
constexpr std::uint32_t OS_ROM_BASE = 0x80000308u;
constexpr std::uint32_t OS_MEM_SIZE = 0x80000318u;
constexpr std::uint32_t ROM_BASE = 0xB0000000u;
constexpr std::uint32_t TV_PAL = 0;
constexpr std::uint32_t TV_NTSC = 1;
// The control and status register libultra sets. It flushes results too small to be normal and
// traps an invalid operation.
constexpr std::uint32_t FPCSR_LIBULTRA = 0x01000800u;
// Addresses among the exception vectors the game does not run: the return of a thread, and the
// handle of the cartridge osCartRomInit() gives.
constexpr std::uint32_t THREAD_EXIT = 0x80000100u;
constexpr std::uint32_t CART_HANDLE = 0x80000110u;
// The cycles of Count a second, half the clock of the processor.
constexpr std::uint64_t COUNT_RATE = 46875000;
// The events of osSetEventMesg().
constexpr std::uint32_t EVENT_SP = 4;
constexpr std::uint32_t EVENT_SI = 5;
constexpr std::uint32_t EVENT_DP = 9;
constexpr std::uint32_t EVENT_COUNT = 15;
// The task type of the graphics, whose end is also the end of the RDP.
constexpr std::uint32_t TASK_GRAPHICS = 1;
// A poll waits on the halt bit of the status register of the RSP.
constexpr std::uint32_t SP_STATUS = 0x04040010u;
constexpr std::uint32_t SP_STATUS_HALT = 1;
// The reports of a missing controller and of a missing controller pak.
constexpr std::uint8_t CONT_NO_RESPONSE = 8;
constexpr std::uint16_t CONT_TYPE_NORMAL = 5;
constexpr std::uint32_t PFS_NO_PAK = 1;
constexpr int CONTROLLERS = 4;
// The offsets of the fields of the console the recording reads, the same in each release.
constexpr std::uint32_t VEHICLE_BODY = 0x148;
constexpr std::uint32_t VEHICLE_WHEELS = 0x350;
constexpr std::uint32_t VEHICLE_ENGINE_RPM = 0xDF4;
constexpr std::uint32_t VEHICLE_THROTTLE = 0xE38;
constexpr std::uint32_t VEHICLE_LAP = 0xF78;
constexpr std::uint32_t VEHICLE_TOTAL_TIME = 0xFA0;
constexpr std::uint32_t VEHICLE_DISTANCE = 0xFA8;
constexpr std::uint32_t VEHICLE_CAR = 0x205C;
constexpr std::uint32_t VEHICLE_COLOR = 0x2060;
constexpr std::uint32_t VEHICLE_INPUT = 0x2074;
constexpr std::uint32_t BODY_SIZE = 0x208;
constexpr std::uint32_t BODY_MATRIX = 0xBC;
constexpr std::uint32_t BODY_SURFACE = 0x1A0;
constexpr std::uint32_t BODY_CONTACT = 0x1B4;
constexpr std::uint32_t BODY_STEER = 0x1C0;
constexpr std::uint32_t BODY_ROLL = 0x1D4;
constexpr std::uint32_t BODY_VEHICLE_IMPACT = 0x1FE;
constexpr std::uint32_t BODY_IMPACT = 0x1FF;
constexpr std::uint32_t BODY_EFFECT = 0x202;
constexpr std::uint32_t BODY_LANDED = 0x203;
constexpr std::uint32_t BODY_SKIDDING = 0x204;
constexpr std::uint32_t INPUT_PLAYBACK = 0x44;
// The most instructions a demo may take, about ten minutes of the console, before it is abandoned.
constexpr std::uint64_t INSTRUCTION_BUDGET = 60'000'000'000ull;
constexpr std::uint64_t RUN_SLICE = 10'000'000;
// The most events sent while every thread waits, a minute of retraces, before the wait is taken
// for a deadlock.
constexpr std::uint32_t MAX_IDLE_EVENTS = 3600;
// The most frames of a demo, to stop a race that does not end.
constexpr std::size_t MAX_FRAMES = 30 * 300;

struct Context {
    std::uint64_t gpr[32];
    std::uint64_t hi;
    std::uint64_t lo;
    std::uint32_t fpr[32];
    std::uint32_t fcr31;
    std::uint32_t pc;
    std::uint32_t npc;
};

enum class ThreadState { Stopped, Runnable, Waiting };

struct Thread {
    std::uint32_t address;
    std::int32_t priority;
    ThreadState state;
    std::uint64_t order; // When the thread last became runnable, for the order of equal priority.
    Context context;
};

struct EventMessage {
    std::uint32_t queue = 0;
    std::uint32_t message = 0;
};

struct Timer {
    std::uint64_t due;
    std::uint64_t interval;
    std::uint32_t queue;
    std::uint32_t message;
};

class Console {
public:
    Console(std::span<const std::uint8_t> rom, N64DemoRelease release);
    std::expected<N64Demo, std::string> Record();

private:
    void Hook(std::uint32_t address, std::function<void()> routine);
    void Replace(std::uint32_t address, std::uint32_t value);
    void InstallLibultra();
    void InstallObservers();
    Context Save() const;
    void Restore(const Context &context);
    Thread *Find(std::uint32_t address);
    Thread *Current();
    void Schedule();
    void Block();
    void PreemptIfNeeded();
    bool Send(std::uint32_t queue, std::uint32_t message, bool jam);
    void WakeOn(std::uint32_t queue);
    void Idle();
    bool DeliverPending();
    void CopyFromRom(std::uint32_t romAddress, std::uint32_t dram, std::uint32_t size);
    void Sample();

    std::span<const std::uint8_t> m_rom;
    N64DemoRelease m_release;
    const DemoSymbols &m_symbols;
    MipsCpu m_cpu;
    std::vector<Thread> m_threads;
    std::optional<std::size_t> m_current;
    std::uint64_t m_order = 0;
    EventMessage m_events[EVENT_COUNT];
    EventMessage m_retrace;
    std::uint32_t m_retraceCount = 1;
    std::uint32_t m_retraces = 0;
    std::uint64_t m_nextRetrace = 0;
    std::uint64_t m_retraceTicks;
    std::uint32_t m_framebuffer = 0;
    std::vector<std::uint32_t> m_pendingEvents;
    std::vector<Timer> m_timers;
    std::string m_log;
    bool m_demoStarted = false;
    bool m_finished = false;
    N64Demo m_demo{};
};

Console::Console(std::span<const std::uint8_t> rom, N64DemoRelease release)
    : m_rom(rom), m_release(release), m_symbols(release == N64DemoRelease::Usa ? USA_SYMBOLS :
                                                release == N64DemoRelease::Eur ? EUR_SYMBOLS :
                                                                                 JPN_SYMBOLS),
      m_cpu(RAM_SIZE), m_retraceTicks(COUNT_RATE / (release == N64DemoRelease::Eur ? 50 : 60)) {
    m_cpu.SetRom(rom);
    std::size_t copy = std::min(BOOT_COPY_SIZE, rom.size() - BOOT_COPY_ROM);
    std::memcpy(m_cpu.ram.data() + (ENTRY & 0x1FFFFFFF), rom.data() + BOOT_COPY_ROM, copy);
    m_cpu.Write32(OS_TV_TYPE, release == N64DemoRelease::Eur ? TV_PAL : TV_NTSC);
    m_cpu.Write32(OS_ROM_BASE, ROM_BASE);
    m_cpu.Write32(OS_MEM_SIZE, MEMORY_SIZE);
    m_cpu.Jump(ENTRY);
    m_nextRetrace = m_retraceTicks;
    m_demo.stepSeconds = release == N64DemoRelease::Eur ? 1.0f / 25.0f : 1.0f / 30.0f;
    m_cpu.SetDevices([](std::uint32_t address,
                        int) -> std::uint64_t { return address == SP_STATUS ? SP_STATUS_HALT : 0; },
                     nullptr);
    m_cpu.SetIdle([this](MipsCpu &) { Idle(); });
    InstallLibultra();
    InstallObservers();
}

void Console::Hook(std::uint32_t address, std::function<void()> routine) {
    if (address == 0) {
        return;
    }
    m_cpu.SetHook(address, [routine = std::move(routine)](MipsCpu &) {
        routine();
        return MipsCpu::HookResult::Jumped;
    });
}

void Console::Replace(std::uint32_t address, std::uint32_t value) {
    Hook(address, [this, value] { m_cpu.Return(value); });
}

Context Console::Save() const {
    Context context{};
    std::memcpy(context.gpr, m_cpu.gpr, sizeof(context.gpr));
    context.hi = m_cpu.hi;
    context.lo = m_cpu.lo;
    std::memcpy(context.fpr, m_cpu.fpr, sizeof(context.fpr));
    context.fcr31 = m_cpu.fcr31;
    context.pc = m_cpu.pc;
    context.npc = m_cpu.npc;
    return context;
}

void Console::Restore(const Context &context) {
    std::memcpy(m_cpu.gpr, context.gpr, sizeof(context.gpr));
    m_cpu.hi = context.hi;
    m_cpu.lo = context.lo;
    std::memcpy(m_cpu.fpr, context.fpr, sizeof(context.fpr));
    m_cpu.fcr31 = context.fcr31;
    m_cpu.pc = context.pc;
    m_cpu.npc = context.npc;
}

Thread *Console::Find(std::uint32_t address) {
    auto found = std::ranges::find(m_threads, address, &Thread::address);
    return found == m_threads.end() ? nullptr : &*found;
}

Thread *Console::Current() {
    return m_current ? &m_threads[*m_current] : nullptr;
}

// Run the runnable thread of the highest priority, the one runnable longest among equals, after
// saving the registers of the current one.
void Console::Schedule() {
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < m_threads.size(); ++i) {
        const Thread &thread = m_threads[i];
        if (thread.state != ThreadState::Runnable) {
            continue;
        }
        if (!best || thread.priority > m_threads[*best].priority ||
            (thread.priority == m_threads[*best].priority &&
             thread.order < m_threads[*best].order)) {
            best = i;
        }
    }
    if (best == m_current) {
        return;
    }
    if (Thread *current = Current()) {
        current->context = Save();
    }
    m_current = best;
    if (best) {
        Restore(m_threads[*best].context);
        return;
    }
    // Every thread waits until an event of a device makes one runnable.
    for (std::uint32_t idle = 0; !m_current && m_cpu.Fault().empty(); ++idle) {
        if (idle > MAX_IDLE_EVENTS || !DeliverPending()) {
            m_log += "Every thread waits on a queue no event sends to.\n";
            m_cpu.Stop();
            return;
        }
        Schedule();
    }
}

// Make the current thread wait on the queue its arguments give, and run the routine of the hook
// again when it becomes runnable.
void Console::Block() {
    Thread *current = Current();
    if (current == nullptr) {
        m_log += "The boot code waits on a queue.\n";
        m_cpu.Stop();
        return;
    }
    current->state = ThreadState::Waiting;
    Schedule();
}

void Console::PreemptIfNeeded() {
    Thread *current = Current();
    if (current == nullptr) {
        Schedule();
        return;
    }
    for (const Thread &thread : m_threads) {
        if (thread.state == ThreadState::Runnable && thread.priority > current->priority) {
            current->order = ++m_order;
            Schedule();
            return;
        }
    }
}

// Make the threads waiting on a queue runnable, to read it again.
void Console::WakeOn(std::uint32_t queue) {
    for (Thread &thread : m_threads) {
        if (thread.state == ThreadState::Waiting &&
            static_cast<std::uint32_t>(thread.context.gpr[MIPS_A0]) == queue) {
            thread.state = ThreadState::Runnable;
            thread.order = ++m_order;
        }
    }
}

// Put a message in a queue, as osSendMesg() and osJamMesg() do. An OSMesgQueue has the waiting
// threads, then validCount, first, msgCount, and the address of the messages.
bool Console::Send(std::uint32_t queue, std::uint32_t message, bool jam) {
    if (queue == 0) {
        return true;
    }
    std::uint32_t valid = m_cpu.Read32(queue + 8);
    std::uint32_t first = m_cpu.Read32(queue + 12);
    std::uint32_t count = m_cpu.Read32(queue + 16);
    std::uint32_t messages = m_cpu.Read32(queue + 20);
    if (count == 0 || valid >= count) {
        return false;
    }
    std::uint32_t slot = 0;
    if (jam) {
        slot = (first + count - 1) % count;
        m_cpu.Write32(queue + 12, slot);
    } else {
        slot = (first + valid) % count;
    }
    m_cpu.Write32(messages + slot * 4, message);
    m_cpu.Write32(queue + 8, valid + 1);
    WakeOn(queue);
    return true;
}

void Console::CopyFromRom(std::uint32_t romAddress, std::uint32_t dram, std::uint32_t size) {
    std::size_t offset = romAddress & 0x0FFFFFFFu;
    for (std::uint32_t i = 0; i < size; ++i) {
        std::size_t at = offset + i;
        m_cpu.Write8(dram + i, at < m_rom.size() ? m_rom[at] : 0);
    }
}

// Send the messages of the devices whose work has finished, or wait for the next retrace or
// timer. Report whether any message was sent.
bool Console::DeliverPending() {
    if (!m_pendingEvents.empty()) {
        std::vector<std::uint32_t> events;
        events.swap(m_pendingEvents);
        for (std::uint32_t event : events) {
            Send(m_events[event].queue, m_events[event].message, false);
        }
        return true;
    }
    std::uint64_t now = m_cpu.Ticks();
    std::uint64_t next = m_nextRetrace;
    for (const Timer &timer : m_timers) {
        next = std::min(next, timer.due);
    }
    if (next > now) {
        m_cpu.idleTicks += next - now;
        now = next;
    }
    bool sent = false;
    for (auto timer = m_timers.begin(); timer != m_timers.end();) {
        if (timer->due <= now) {
            sent = Send(timer->queue, timer->message, false) || sent;
            if (timer->interval != 0) {
                timer->due += timer->interval;
            } else {
                timer = m_timers.erase(timer);
                continue;
            }
        }
        ++timer;
    }
    if (m_nextRetrace <= now) {
        m_nextRetrace += m_retraceTicks;
        if (m_retrace.queue != 0 && ++m_retraces >= m_retraceCount) {
            m_retraces = 0;
            Send(m_retrace.queue, m_retrace.message, false);
        }
        sent = true;
    }
    return sent || m_retrace.queue != 0 || !m_timers.empty();
}

// The idle thread spins, and the time passes to the next event.
void Console::Idle() {
    if (DeliverPending()) {
        PreemptIfNeeded();
    }
}

void Console::InstallLibultra() {
    const DemoSymbols &s = m_symbols;
    MipsCpu &cpu = m_cpu;
    Hook(s.osInitialize, [&cpu] {
        cpu.fcr31 = FPCSR_LIBULTRA;
        cpu.Return(0);
    });
    Hook(s.osCreateThread, [this, &cpu] {
        Thread thread{};
        thread.address = cpu.Argument(0);
        thread.priority = static_cast<std::int32_t>(cpu.Argument(5));
        thread.state = ThreadState::Stopped;
        thread.context.pc = cpu.Argument(2);
        thread.context.npc = thread.context.pc + 4;
        thread.context.gpr[MIPS_A0] = static_cast<std::int32_t>(cpu.Argument(3));
        thread.context.gpr[MIPS_SP] = static_cast<std::int32_t>(cpu.Argument(4));
        thread.context.gpr[MIPS_RA] = static_cast<std::int32_t>(THREAD_EXIT);
        thread.context.fcr31 = cpu.fcr31;
        if (Thread *existing = Find(thread.address)) {
            *existing = thread;
        } else {
            m_threads.push_back(thread);
        }
        cpu.Return(0);
    });
    Hook(s.osStartThread, [this, &cpu] {
        Thread *thread = Find(cpu.Argument(0));
        cpu.Return(0);
        if (thread != nullptr && thread->state == ThreadState::Stopped) {
            thread->state = ThreadState::Runnable;
            thread->order = ++m_order;
        }
        PreemptIfNeeded();
    });
    Hook(s.osSetThreadPri, [this, &cpu] {
        std::uint32_t address = cpu.Argument(0);
        Thread *thread = address == 0 ? Current() : Find(address);
        auto priority = static_cast<std::int32_t>(cpu.Argument(1));
        cpu.Return(0);
        if (thread != nullptr) {
            thread->priority = priority;
        }
        PreemptIfNeeded();
    });
    Hook(s.osGetThreadPri, [this, &cpu] {
        std::uint32_t address = cpu.Argument(0);
        Thread *thread = address == 0 ? Current() : Find(address);
        cpu.Return(thread != nullptr ? static_cast<std::uint32_t>(thread->priority) : 0);
    });
    Hook(s.osYieldThread, [this, &cpu] {
        cpu.Return(0);
        if (Thread *current = Current()) {
            current->order = ++m_order;
        }
        Schedule();
    });
    Hook(s.osDestroyThread, [this, &cpu] {
        std::uint32_t address = cpu.Argument(0);
        Thread *thread = address == 0 ? Current() : Find(address);
        cpu.Return(0);
        if (thread != nullptr) {
            thread->state = ThreadState::Stopped;
        }
        Schedule();
    });
    Hook(THREAD_EXIT, [this] {
        if (Thread *current = Current()) {
            current->state = ThreadState::Stopped;
        }
        Schedule();
    });
    Hook(s.osRecvMesg, [this, &cpu] {
        std::uint32_t queue = cpu.Argument(0);
        std::uint32_t out = cpu.Argument(1);
        bool block = cpu.Argument(2) != 0;
        std::uint32_t valid = cpu.Read32(queue + 8);
        if (valid == 0) {
            if (!block) {
                cpu.Return(static_cast<std::uint32_t>(-1));
                return;
            }
            Block();
            return;
        }
        std::uint32_t first = cpu.Read32(queue + 12);
        std::uint32_t count = cpu.Read32(queue + 16);
        std::uint32_t messages = cpu.Read32(queue + 20);
        if (out != 0) {
            cpu.Write32(out, cpu.Read32(messages + first * 4));
        }
        cpu.Write32(queue + 12, (first + 1) % count);
        cpu.Write32(queue + 8, valid - 1);
        cpu.Return(0);
        WakeOn(queue);
        PreemptIfNeeded();
    });
    auto send = [this, &cpu](bool jam) {
        std::uint32_t queue = cpu.Argument(0);
        std::uint32_t message = cpu.Argument(1);
        bool block = cpu.Argument(2) != 0;
        if (!Send(queue, message, jam)) {
            if (!block) {
                cpu.Return(static_cast<std::uint32_t>(-1));
                return;
            }
            Block();
            return;
        }
        cpu.Return(0);
        PreemptIfNeeded();
    };
    Hook(s.osSendMesg, [send] { send(false); });
    Hook(s.osJamMesg, [send] { send(true); });
    Hook(s.osSetEventMesg, [this, &cpu] {
        std::uint32_t event = cpu.Argument(0);
        if (event < EVENT_COUNT) {
            m_events[event] = {cpu.Argument(1), cpu.Argument(2)};
        }
        cpu.Return(0);
    });
    Hook(s.osViSetEvent, [this, &cpu] {
        m_retrace = {cpu.Argument(0), cpu.Argument(1)};
        m_retraceCount = std::max<std::uint32_t>(cpu.Argument(2), 1);
        cpu.Return(0);
    });
    Hook(s.osViSwapBuffer, [this, &cpu] {
        m_framebuffer = cpu.Argument(0);
        cpu.Return(0);
    });
    Hook(s.osViGetCurrentFramebuffer, [this, &cpu] { cpu.Return(m_framebuffer); });
    for (std::uint32_t address : {s.osCreateViManager,
                                  s.osViSetMode,
                                  s.osViBlack,
                                  s.osViSetSpecialFeatures,
                                  s.osCreatePiManager,
                                  s.osEPiRawWriteIo,
                                  s.osSpTaskLoad,
                                  s.osInvalDCache,
                                  s.osInvalICache,
                                  s.osWritebackDCache,
                                  s.osWritebackDCacheAll,
                                  s.osSetIntMask,
                                  s.osAiSetNextBuffer,
                                  s.osAiGetStatus,
                                  s.osAiGetLength,
                                  s.osMapTLBRdb}) {
        Replace(address, 0);
    }
    Hook(s.osAiSetFrequency, [&cpu] { cpu.Return(cpu.Argument(0)); });
    Hook(s.osPiStartDma, [this, &cpu] {
        std::uint32_t message = cpu.Argument(0);
        std::uint32_t direction = cpu.Argument(2);
        std::uint32_t device = cpu.Argument(3);
        std::uint32_t dram = cpu.Argument(4);
        std::uint32_t size = cpu.Argument(5);
        std::uint32_t queue = cpu.Argument(6);
        if (direction == 0) {
            CopyFromRom(device, dram, size);
        }
        cpu.Write32(message + 4, queue);
        cpu.Write32(message + 8, dram);
        cpu.Write32(message + 12, device);
        cpu.Write32(message + 16, size);
        Send(queue, message, false);
        cpu.Return(0);
        PreemptIfNeeded();
    });
    Hook(s.osPiRawStartDma, [this, &cpu] {
        if (cpu.Argument(0) == 0) {
            CopyFromRom(cpu.Argument(1), cpu.Argument(2), cpu.Argument(3));
        }
        cpu.Return(0);
    });
    Hook(s.osEPiRawStartDma, [this, &cpu] {
        std::uint32_t base = cpu.Read32(cpu.Argument(0) + 12);
        if (cpu.Argument(1) == 0) {
            CopyFromRom(base | cpu.Argument(2), cpu.Argument(3), cpu.Argument(4));
        }
        cpu.Return(0);
    });
    auto readIo = [this, &cpu](std::uint32_t device, std::uint32_t out) {
        std::uint32_t word = 0;
        std::size_t offset = device & 0x0FFFFFFFu;
        for (std::size_t i = 0; i < 4; ++i) {
            word = word << 8 | (offset + i < m_rom.size() ? m_rom[offset + i] : 0);
        }
        cpu.Write32(out, word);
        cpu.Return(0);
    };
    Hook(s.osPiRawReadIo, [readIo, &cpu] { readIo(cpu.Argument(0), cpu.Argument(1)); });
    Hook(s.osEPiRawReadIo, [readIo, &cpu] {
        readIo(cpu.Read32(cpu.Argument(0) + 12) | cpu.Argument(1), cpu.Argument(2));
    });
    Hook(s.osCartRomInit, [&cpu] {
        for (std::uint32_t i = 0; i < 0x74; i += 4) {
            cpu.Write32(CART_HANDLE + i, 0);
        }
        cpu.Write32(CART_HANDLE + 12, ROM_BASE);
        cpu.Return(CART_HANDLE);
    });
    Hook(s.osSpTaskStartGo, [this, &cpu] {
        std::uint32_t type = cpu.Read32(cpu.Argument(0));
        m_pendingEvents.push_back(EVENT_SP);
        if (type == TASK_GRAPHICS) {
            m_pendingEvents.push_back(EVENT_DP);
        }
        cpu.Return(0);
    });
    // One controller without a pak, at rest.
    Hook(s.osContInit, [&cpu] {
        std::uint32_t pattern = cpu.Argument(1);
        std::uint32_t status = cpu.Argument(2);
        cpu.Write8(pattern, 1);
        for (int i = 0; i < CONTROLLERS; ++i) {
            std::uint32_t entry = status + static_cast<std::uint32_t>(i) * 4;
            cpu.Write16(entry, i == 0 ? CONT_TYPE_NORMAL : 0);
            cpu.Write8(entry + 2, 0);
            cpu.Write8(entry + 3, i == 0 ? 0 : CONT_NO_RESPONSE);
        }
        cpu.Return(0);
    });
    Hook(s.osContStartReadData, [this, &cpu] {
        std::uint32_t queue = cpu.Argument(0);
        std::uint32_t message = m_events[EVENT_SI].queue == queue ? m_events[EVENT_SI].message : 0;
        Send(queue, message, false);
        cpu.Return(0);
        PreemptIfNeeded();
    });
    Hook(s.osContGetReadData, [&cpu] {
        std::uint32_t pads = cpu.Argument(0);
        for (int i = 0; i < CONTROLLERS; ++i) {
            std::uint32_t pad = pads + static_cast<std::uint32_t>(i) * 6;
            cpu.Write16(pad, 0);
            cpu.Write8(pad + 2, 0);
            cpu.Write8(pad + 3, 0);
            cpu.Write8(pad + 4, i == 0 ? 0 : CONT_NO_RESPONSE);
        }
        cpu.Return(0);
    });
    Hook(s.osPfsIsPlug, [&cpu] {
        cpu.Write8(cpu.Argument(1), 0);
        cpu.Return(0);
    });
    for (std::uint32_t address : {s.osPfsInit,
                                  s.osPfsInitPak,
                                  s.osPfsFindFile,
                                  s.osPfsReadWriteFile,
                                  s.osPfsAllocateFile,
                                  s.osPfsDeleteFile,
                                  s.osPfsFreeBlocks,
                                  s.osPfsNumFiles,
                                  s.osPfsFileState,
                                  s.osPfsRepairId,
                                  s.osPfsChecker,
                                  s.osMotorInit,
                                  s.osMotorStart,
                                  s.osMotorStop}) {
        Replace(address, PFS_NO_PAK);
    }
    // The EEPROM is reported missing. On a cartridge without saved data the game falls back to the
    // controller pak, also missing.
    Replace(s.osEepromProbe, 0);
    for (std::uint32_t address :
         {s.osEepromRead, s.osEepromWrite, s.osEepromLongRead, s.osEepromLongWrite}) {
        Replace(address, static_cast<std::uint32_t>(-1));
    }
    Hook(s.osGetCount, [&cpu] { cpu.Return(cpu.Count()); });
    Hook(s.osGetTime, [&cpu] {
        std::uint64_t time = cpu.Ticks();
        cpu.Return(static_cast<std::uint32_t>(time >> 32));
        cpu.gpr[MIPS_V1] = static_cast<std::uint64_t>(
            static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(time))));
    });
    Hook(s.osSetTimer, [this, &cpu] {
        // The countdown and the interval are 64-bit, each in a pair of argument words.
        std::uint64_t countdown =
            static_cast<std::uint64_t>(cpu.Argument(2)) << 32 | cpu.Argument(3);
        std::uint64_t interval =
            static_cast<std::uint64_t>(cpu.Argument(4)) << 32 | cpu.Argument(5);
        m_timers.push_back({cpu.Ticks() + std::max<std::uint64_t>(countdown, 1),
                            interval,
                            cpu.Argument(6),
                            cpu.Argument(7)});
        cpu.Return(0);
    });
    Hook(s.osVirtualToPhysical, [&cpu] { cpu.Return(cpu.Argument(0) & 0x1FFFFFFFu); });
    Hook(s.osSyncPrintf, [this, &cpu] {
        std::uint32_t text = cpu.Argument(0);
        for (std::uint32_t i = 0; i < 256; ++i) {
            char c = static_cast<char>(cpu.Read8(text + i));
            if (c == '\0') {
                break;
            }
            m_log += c;
        }
        cpu.Return(0);
    });
}

void Console::InstallObservers() {
    const DemoSymbols &s = m_symbols;
    m_cpu.SetHook(s.startAttractDemo, [this](MipsCpu &) {
        m_demoStarted = true;
        return MipsCpu::HookResult::Continue;
    });
    m_cpu.SetHook(s.processRacingScreen, [this, &s](MipsCpu &cpu) {
        if (m_demoStarted && cpu.Read32(s.raceInitialized) != 0) {
            Sample();
        }
        return MipsCpu::HookResult::Continue;
    });
    m_cpu.SetHook(s.processSplashScreens, [this](MipsCpu &cpu) {
        if (m_demoStarted && !m_demo.frames.empty()) {
            m_finished = true;
            cpu.Stop();
        }
        return MipsCpu::HookResult::Continue;
    });
}

void Console::Sample() {
    MipsCpu &cpu = m_cpu;
    std::uint32_t vehicle = m_symbols.vehicles;
    std::uint32_t body = vehicle + VEHICLE_BODY;
    if (m_demo.frames.empty()) {
        m_demo.track = static_cast<std::int32_t>(cpu.Read32(m_symbols.selectedTrack));
        m_demo.car = static_cast<std::int32_t>(cpu.Read32(vehicle + VEHICLE_CAR));
        std::uint32_t recording = cpu.Read32(cpu.Read32(vehicle + VEHICLE_INPUT) + INPUT_PLAYBACK);
        for (std::uint32_t i = 0; i < m_demo.options.size(); ++i) {
            m_demo.options[i] = cpu.Read8(recording + 2 + i);
        }
        for (std::uint32_t i = 0; i < m_demo.color.size(); ++i) {
            m_demo.color[i] = cpu.Read8(vehicle + VEHICLE_COLOR + i);
        }
    }
    N64DemoFrame frame{};
    for (std::uint32_t row = 0; row < 4; ++row) {
        for (std::uint32_t column = 0; column < 4; ++column) {
            frame.matrix[row][column] = cpu.ReadFloat(body + BODY_MATRIX + (row * 4 + column) * 4);
        }
    }
    frame.bodyRoll = cpu.ReadFloat(body + BODY_ROLL);
    frame.steerAngle = cpu.ReadFloat(vehicle + VEHICLE_WHEELS + BODY_SIZE + BODY_STEER);
    for (std::uint32_t i = 0; i < 4; ++i) {
        std::uint32_t wheel = vehicle + VEHICLE_WHEELS + i * BODY_SIZE;
        frame.wheelRoll[i] = cpu.ReadFloat(wheel + BODY_ROLL);
        frame.wheelContact[i] = static_cast<std::int32_t>(cpu.Read32(wheel + BODY_CONTACT));
        frame.wheelSurface[i] = cpu.Read8(wheel + BODY_SURFACE);
    }
    frame.skidding = cpu.Read8(body + BODY_SKIDDING);
    frame.landed = cpu.Read8(body + BODY_LANDED);
    frame.impactVolume = cpu.Read8(body + BODY_IMPACT);
    frame.vehicleImpactVolume = cpu.Read8(body + BODY_VEHICLE_IMPACT);
    frame.effect = cpu.Read8(body + BODY_EFFECT);
    frame.buttons = cpu.Read32(cpu.Read32(vehicle + VEHICLE_INPUT));
    frame.throttle = cpu.ReadFloat(vehicle + VEHICLE_THROTTLE);
    frame.engineRpm = cpu.ReadFloat(vehicle + VEHICLE_ENGINE_RPM);
    frame.distance = cpu.ReadFloat(vehicle + VEHICLE_DISTANCE);
    frame.totalTime = cpu.ReadFloat(vehicle + VEHICLE_TOTAL_TIME);
    frame.lap = static_cast<std::int32_t>(cpu.Read32(vehicle + VEHICLE_LAP));
    m_demo.frames.push_back(frame);
    if (m_demo.frames.size() >= MAX_FRAMES) {
        m_log += "The demo did not end.\n";
        cpu.Stop();
    }
}

std::expected<N64Demo, std::string> Console::Record() {
    std::uint64_t run = 0;
    while (!m_finished && run < INSTRUCTION_BUDGET) {
        std::uint64_t before = m_cpu.instructions;
        bool stopped = m_cpu.Run(RUN_SLICE);
        run += std::max<std::uint64_t>(m_cpu.instructions - before, 1);
        if (!m_cpu.Fault().empty()) {
            return std::unexpected(fmt::format("{}\n{}", m_cpu.Fault(), m_log));
        }
        if (stopped && !m_finished) {
            return std::unexpected(m_log);
        }
    }
    if (!m_finished) {
        return std::unexpected(fmt::format("The demo did not end in time.\n{}", m_log));
    }
    m_demo.instructions = m_cpu.instructions;
    return m_demo;
}

} // namespace

std::expected<N64Demo, std::string> RecordN64AttractDemo(std::span<const std::uint8_t> rom,
                                                         N64DemoRelease release) {
    if (rom.size() < BOOT_COPY_ROM + BOOT_COPY_SIZE) {
        return std::unexpected("The image is too small.");
    }
    Console console(rom, release);
    return console.Record();
}

} // namespace coastline
