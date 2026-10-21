/* Host check of src/audio.c on a small XM module, with libultra and the mixers stubbed. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

#include "audio.c"

u16 g_wAudioBufferWritePos;
MixerChannel g_aMixerChannels[MIXER_CHANNELS];
u64 g_qwSoundEffectSampleMask;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
s32 g_nAudioChannelCount, g_dwMusicPatternRow, g_dwAudioTickToggle, g_dwAiDmaPrevLength,
    g_bMusicPlaybackEnabled, g_dwAudioMixBytes, g_dwAiDmaCurrentLength, g_dwAudioChannelLoopIdx;
u8 g_bBgmVolume = 0xFF, g_bMusicVolume = 0x80, g_bSfxMasterVolume = 0xFF, g_bSfxVolume = 0x80;
f32 g_afNoteFrequencies[MUSIC_NOTES] = {
    261.7f, 277.2f, 293.7f, 311.1f, 329.6f, 349.2f, 370.0f, 392.0f, 415.3f, 440.0f, 466.2f, 493.9f};
s8 g_SilenceSampleData[4];
u32 g_dwSoundEffectMixOffset;
OSMesgQueue g_AudioMsgQueue;
OSMesg g_aAudioMsgBuf[2];
OSTimer g_AudioTimer;
s16 g_AudioDmaBuffer[AUDIO_RING_BYTES / sizeof(s16)];
XmSample *g_apInstrumentData[128];
u8 *g_apPatternData[256];
MusicChannel g_MusicChannelStates[MIXER_CHANNELS];
SoundEffectSample g_aSoundEffectSamples[SOUND_EFFECT_CHANNELS];
MusicPlayer g_MusicPlayer;
OSThread g_AudioThread;
u64 g_aAudioThreadStack[AUDIO_THREAD_STACK_WORDS];
u64 g_aFreqScaleTable[MUSIC_OCTAVES][MUSIC_NOTES];
u64 osClockRate = 62500000;

static jmp_buf stop;
static s32 messages, mixed, pairs[4], pairCount, effectBytes, effectOffset, rumble = -1;
static OSTime timerCountdown, timerInterval;
static void *threadStack;
static s32 threadPriority, threadId;

void osSyncPrintf(const char *fmt, ...) {
}
s32 osAiSetFrequency(u32 f) {
    return 22050;
}
void osCreateMesgQueue(OSMesgQueue *q, OSMesg *m, s32 n) {
    assert(m == g_aAudioMsgBuf && n == 2);
}
int osSetTimer(OSTimer *t, OSTime c, OSTime i, OSMesgQueue *q, OSMesg m) {
    timerCountdown = c;
    timerInterval = i;
    return 0;
}
u32 osGetCount(void) {
    return 1000;
}
s32 osAiSetNextBuffer(void *p, u32 size) {
    assert(p == g_AudioDmaBuffer && size == 0x4000);
    return 7;
}
u32 osAiGetStatus(void) {
    return 0;
}
u32 osAiGetLength(void) {
    return 0x2000;
}
void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri) {
    threadId = id;
    threadStack = sp;
    threadPriority = pri;
}
void osStartThread(OSThread *t) {
}
s32 osRecvMesg(OSMesgQueue *q, OSMesg *m, s32 flag) {
    if (messages++) {
        longjmp(stop, 1);
    }
    return 0;
}
void MixAudioSamples(s16 *ring, u32 bytes) {
    mixed = bytes;
    g_wAudioBufferWritePos += 4;
}
void MixAudioStereoSamples(s16 *ring, u32 bytes, u32 offset) {
    /* Each pair mixes over the same bytes as the first six channels, from where they started. */
    assert(g_wAudioBufferWritePos == 0);
    pairs[pairCount++] = offset;
    g_wAudioBufferWritePos += 4;
}
void MixSoundEffectChannels(s16 *ring, u32 bytes, u32 offset) {
    effectBytes = bytes;
    effectOffset = offset;
}
void UpdateRumblePaks(s32 unused) {
    rumble = unused;
}

static u8 module[0x400];
static u8 work[0x2000] __attribute__((aligned(8)));

static void Le32(u32 at, u32 v) {
    module[at] = v;
    module[at + 1] = v >> 8;
    module[at + 2] = v >> 16;
    module[at + 3] = v >> 24;
}

/* An instrument of 0x20 bytes, then a sample header and its deltas. */
static u32
Instrument(u32 at, u32 length, u32 loop, u8 volume, u8 type, s8 relative, const u8 *deltas) {
    u32 i;

    Le32(at, 0x20);
    module[at + 0x1B] = deltas != NULL;
    if (deltas == NULL) {
        return at + 0x20;
    }
    at += 0x20;
    Le32(at, length);
    Le32(at + 8, loop);
    module[at + 0xC] = volume;
    module[at + 0xE] = type;
    module[at + 0x10] = relative;
    for (i = 0; i < length; i++) {
        module[at + 0x28 + i] = deltas[i];
    }
    return at + 0x28 + length;
}

int main(void) {
    static const u8 rows0[] = {0x83, 13, 1, 0x80, 0x98, 0x0A, 0x02, 25, 3, 0x30, 0x0F, 0x04};
    static const u8 rows1[] = {0x80, 0x80};
    static const u8 deltas0[] = {1, 1, 1, 1, 1};
    static const u8 deltas2[] = {10, 0xEC, 5};
    XmSample *sample;
    u64 step;
    s64 period;
    u32 i, at;

    /* The module: three orders, two patterns and three instruments, the second empty. */
    Le32(0x3C, 0x114);
    module[0x40] = 3;
    module[0x42] = 1;
    module[0x44] = 2;
    module[0x46] = 2;
    module[0x48] = 3;
    module[0x4C] = 6;
    module[0x51] = 1;
    at = 0x150;
    Le32(at, 9);
    module[at + 5] = 2;
    module[at + 7] = sizeof(rows0);
    for (i = 0; i < sizeof(rows0); i++) {
        module[at + 9 + i] = rows0[i];
    }
    at += 9 + sizeof(rows0);
    Le32(at, 9);
    module[at + 5] = 1;
    module[at + 7] = sizeof(rows1);
    for (i = 0; i < sizeof(rows1); i++) {
        module[at + 9 + i] = rows1[i];
    }
    at += 9 + sizeof(rows1);
    at = Instrument(at, 5, 2, 48, 1, 2, deltas0);
    at = Instrument(at, 0, 0, 0, 0, 0, NULL);
    at = Instrument(at, 3, 0, 64, 0, -1, deltas2);

    LoadMusicModule(module, work);
    assert(g_MusicPlayer.orders == work && g_MusicPlayer.songLength == 3 &&
           g_MusicPlayer.speed == 6);
    assert(g_MusicPlayer.restartOrder == 1 && g_nAudioChannelCount == 2);
    assert(work[0] == 0 && work[1] == 1 && work[2] == 0);
    assert(g_apPatternData[0] == work + 4 && g_apPatternData[1] == work + 25);
    assert(work[4 + 9] == 0x83 && work[25 + 9] == 0x80);
    sample = g_apInstrumentData[0];
    assert((u8 *)sample == work + 36 && sample->length == 5 && sample->loopLength == 2);
    assert(sample->volume == 48 && sample->type == 1 && sample->relativeNote == 2);
    assert(((u8 *)(sample + 1))[0] == 1 && ((u8 *)(sample + 1))[4] == 5);
    assert(((u8 *)(sample + 1))[5] == 4 && ((u8 *)(sample + 1))[6] == 5 &&
           ((u8 *)(sample + 1))[1204] == 5);
    assert(g_apInstrumentData[1] == NULL);
    sample = g_apInstrumentData[2];
    assert((u8 *)sample == work + 1284 && sample->length == 3 && sample->relativeNote == -1);
    assert(((s8 *)(sample + 1))[1] == -10 && ((s8 *)(sample + 1))[2] == -5);
    assert(((u8 *)(sample + 1))[3] == 0 && ((u8 *)(sample + 1))[1202] == 0);

    /* The steps of the notes. */
    assert(*(u64 *)&(double){NOTE_STEP_SCALE} == 0x3FD854B86E0B2019ULL);
    assert(*(u64 *)&(double){NOTE_BASE_FREQUENCY} == 0x40705B3333333333ULL);
    BuildFrequencyScaleTable();
    assert(g_aFreqScaleTable[0][0] ==
           (u64)((f64)261.7f * 0.0625 / 261.7 * 0.380170924629511797 * 4294967296.0));
    assert(g_aFreqScaleTable[3][9] ==
           (u64)((f64)(440.0f * 8.0f) * 0.0625 / 261.7 * 0.380170924629511797 * 4294967296.0));

    /* The first row starts instrument 1 on channel 0; six ticks later the second row slides it. */
    ResetMusicChannels();
    UpdateMusicPlayback();
    assert(g_MusicPlayer.order == 0 && g_MusicPlayer.rowsLeft == 1 && g_MusicPlayer.tick == 6);
    sample = g_apInstrumentData[0];
    assert(g_MusicChannelStates[0].note == 12 && g_MusicChannelStates[0].instrument == 1);
    assert(g_aMixerChannels[0].sample == (s8 *)(sample + 1) &&
           g_aMixerChannels[0].step == g_aFreqScaleTable[1][2]);
    assert(g_aMixerChannels[0].volume == 48 && g_MusicChannelStates[1].instrument == 0);
    for (i = 0; i < 5; i++) {
        UpdateMusicPlayback();
    }
    assert(g_MusicPlayer.tick == 1 && g_MusicChannelStates[0].volume == 64);
    UpdateMusicPlayback();
    assert(g_MusicPlayer.speed == 4 && g_MusicPlayer.tick == 4 && g_MusicPlayer.rowsLeft == 0);
    assert(g_MusicChannelStates[0].volumeSlide == -2 && g_MusicChannelStates[0].volume == 62);
    assert(g_aMixerChannels[0].volume == 46);
    assert(g_MusicChannelStates[1].note == 24 && g_MusicChannelStates[1].instrument == 3);
    assert(g_aMixerChannels[1].step == g_aFreqScaleTable[1][11] &&
           g_aMixerChannels[1].volume == 48);
    UpdateMusicPlayback();
    assert(g_MusicChannelStates[0].volume == 60 && g_aMixerChannels[0].volume == 45);
    for (i = 0; i < 3; i++) {
        UpdateMusicPlayback();
    }
    assert(g_MusicPlayer.order == 1 && g_MusicPlayer.rowsLeft == 0 &&
           g_MusicChannelStates[0].volumeSlide == 0);
    for (i = 0; i < 8; i++) {
        UpdateMusicPlayback();
    }
    assert(g_MusicPlayer.order == 2 && g_MusicPlayer.rowsLeft == 0);
    for (i = 0; i < 4; i++) {
        UpdateMusicPlayback();
    }
    assert(g_MusicPlayer.order == 1);

    /* An arpeggio steps through its notes and the step itself. */
    {
        static u8 row[] = {0x98, 0x00, 0x47, 0x80};
        ParseMusicPatternRow(row);
    }
    assert(g_MusicChannelStates[0].arpeggioOn && g_MusicChannelStates[0].arpeggio[0] == 12);
    assert(g_MusicChannelStates[0].arpeggio[1] == 16 && g_MusicChannelStates[0].arpeggio[2] == 19);
    g_MusicPlayer.tick = 2;
    g_MusicChannelStates[0].arpeggio[3] = 0;
    UpdateMusicPlayback();
    assert(g_aMixerChannels[0].step == NOTE_STEP(2 + 12) &&
           g_MusicChannelStates[0].arpeggio[3] == 1);
    g_MusicPlayer.tick = 2;
    UpdateMusicPlayback();
    g_MusicPlayer.tick = 2;
    UpdateMusicPlayback();
    assert(g_aMixerChannels[0].step == NOTE_STEP(2 + 19) &&
           g_MusicChannelStates[0].arpeggio[3] == 3);
    g_MusicPlayer.tick = 2;
    UpdateMusicPlayback();
    assert(g_aMixerChannels[0].step == NOTE_STEP(2 + 3) &&
           g_MusicChannelStates[0].arpeggio[3] == 0);

    /* A tone portamento up stops at its target, and one down at its own. */
    {
        static u8 row[] = {0x9B, 25, 1, 0x03, 0x40, 0x80};
        g_aMixerChannels[0].step = NOTE_STEP(2 + 12);
        ParseMusicPatternRow(row);
    }
    assert(g_MusicChannelStates[0].portamentoTarget == NOTE_STEP(2 + 24));
    assert(g_MusicChannelStates[0].portamentoSpeed == 0x40 &&
           g_aMixerChannels[0].step == NOTE_STEP(2 + 12));
    g_MusicPlayer.tick = 2;
    step = g_aMixerChannels[0].step;
    UpdateMusicPlayback();
    period = (s64)(0xEE9FCFF0B5ULL / step) - 0x40;
    assert(g_aMixerChannels[0].step == (u64)(0xEE9FCFF0B5LL / period));
    for (i = 0; i < 200; i++) {
        g_MusicPlayer.tick = 2;
        UpdateMusicPlayback();
    }
    assert(g_aMixerChannels[0].step == NOTE_STEP(2 + 24));
    {
        static u8 row[] = {0x9B, 13, 1, 0x03, 0x40, 0x80};
        ParseMusicPatternRow(row);
    }
    assert(g_MusicChannelStates[0].portamentoSpeed == -0x40);
    for (i = 0; i < 400; i++) {
        g_MusicPlayer.tick = 2;
        UpdateMusicPlayback();
    }
    assert(g_aMixerChannels[0].step == NOTE_STEP(2 + 12));

    /* A sample that passes its end loops back, and one without a loop stops. */
    g_aMixerChannels[0].sample = (s8 *)(g_apInstrumentData[0] + 1) + 6;
    CheckMusicLoopPoints();
    assert(g_aMixerChannels[0].sample == (s8 *)(g_apInstrumentData[0] + 1) + 4);
    g_MusicChannelStates[1].instrument = 3;
    g_aMixerChannels[1].sample = (s8 *)(g_apInstrumentData[2] + 1) + 4;
    CheckMusicLoopPoints();
    assert(g_aMixerChannels[1].step == 0);

    /* The sound effects. */
    assert(FindFreeSoundChannel() == 0);
    InitSoundChannel(0, (s8 *)work, 100, 0);
    assert(g_aSoundEffectChannels[0].step == 1ULL << 32 &&
           g_aSoundEffectChannels[0].volume == 0x00200020);
    StartSoundChannelMuted(1, (s8 *)work + 8, 50, 10);
    assert(g_aSoundEffectChannels[1].volume == 0 && g_aSoundEffectSamples[1].loopLength == 10);
    assert(FindFreeSoundChannel() == 2);
    for (i = 2; i < 6; i++) {
        InitSoundChannel(i, (s8 *)work, 1, 0);
    }
    assert(FindFreeSoundChannel() == -1);
    SetSoundChannelVolume(1, 0x00400010);
    assert(g_aSoundEffectChannels[1].volume == 0x00000010);
    SetSoundChannelVolume(1, 0x00100030);
    assert(g_aSoundEffectChannels[1].volume == 0x00100020);
    g_aSoundEffectChannels[0].sample = (s8 *)work + 100;
    g_aSoundEffectChannels[1].sample = (s8 *)work + 59;
    CheckSoundEffectLoopPoints();
    assert(g_aSoundEffectChannels[0].step == 0 &&
           g_aSoundEffectChannels[1].sample == (s8 *)work + 49);

    /* The start of the audio, then a pass of the mixing thread with eight music channels. */
    InitAudioSystem(module, work);
    assert(g_qwSoundEffectSampleMask == 0xFFFEFFFEFFFEFFFEULL && timerCountdown == 625000 &&
           timerInterval == 625000);
    assert(threadId == 7 && threadPriority == 0x7F && threadStack == g_aAudioThreadStack + 0x80);
    assert(g_aMixerChannels[1].sample == (s8 *)g_AudioDmaBuffer &&
           g_aMixerChannels[1].volume == 0x20);
    assert(g_aSoundEffectChannels[5].sample == g_SilenceSampleData &&
           g_aSoundEffectChannels[5].volume == 0x20);
    assert(mixed == 0x4000 && g_bMusicPlaybackEnabled && g_MusicPlayer.order == 0xFF);
    /* The patterns have two channels, so the music stays off while the thread mixes eight. */
    g_nAudioChannelCount = 8;
    g_bMusicPlaybackEnabled = FALSE;
    g_aSoundEffectChannels[2].volume = 0x00200010;
    g_dwAiDmaPrevLength = 0x3000;
    g_dwSoundEffectMixOffset = 0x100;
    g_wAudioBufferWritePos = 0;
    if (!setjmp(stop)) {
        AudioMixingThreadMain(NULL);
    }
    /* The toggle flips again before the second message, which ends the pass. */
    assert(g_dwAudioTickToggle == 0 && g_dwAudioMixBytes == 0x2000 && mixed == 0x2000);
    assert(g_dwAiDmaPrevLength == 0x1000 && g_dwAiDmaCurrentLength == 0x1000);
    assert(g_aMixerChannels[0].mixVolume == 15 &&
           g_aSoundEffectChannels[2].mixVolume == 0x000F0007);
    assert(pairCount == 1 && pairs[0] == 0 && g_dwAudioChannelLoopIdx == 8);
    assert(effectBytes == 0x2F00 && effectOffset == 0x100 && g_dwSoundEffectMixOffset == 0x3000);
    assert(rumble == 0 && g_MusicPlayer.order == 0xFF);
    puts("audio ok");
    return 0;
}
