#include "common/audio.h"

#include <stddef.h>

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/syncprintf.h"

/* The sound system of the PC release has channels of the same names, and the SDL game has both. */
#define InitSoundChannel N64InitSoundChannel
#define StartSoundChannelMuted N64StartSoundChannelMuted
#else
#include <PR/rcp.h>

#include "common/rumble.h"
#endif

/* The fields of the header of an XM module. */
#define XM_HEADER_SIZE 0x3C
#define XM_SONG_LENGTH 0x40
#define XM_RESTART 0x42
#define XM_CHANNELS 0x44
#define XM_PATTERNS 0x46
#define XM_INSTRUMENTS 0x48
#define XM_TEMPO 0x4C
#define XM_ORDERS 0x50

/* The fields of the header of an XM pattern, and where its packed rows start. */
#define XM_PATTERN_ROWS 5
#define XM_PATTERN_PACKED_SIZE 7
#define XM_PATTERN_DATA 9

/* The field of the header of an XM instrument that counts its samples. */
#define XM_INSTRUMENT_SAMPLES 0x1B

/* The fields of the header of an XM sample, which may not be aligned in the module. */
#define XM_SAMPLE_LOOP_LENGTH 8
#define XM_SAMPLE_TYPE 0xE

/* The flags of a packed note. A byte without the first holds a note, and four bytes follow it. */
#define XM_PACKED_NOTE 0x01
#define XM_PACKED_INSTRUMENT 0x02
#define XM_PACKED_VOLUME 0x04
#define XM_PACKED_EFFECT 0x08
#define XM_PACKED_PARAMETER 0x10

/* The effects the player runs. */
#define XM_EFFECT_ARPEGGIO 0x0
#define XM_EFFECT_PORTAMENTO_UP 0x1
#define XM_EFFECT_PORTAMENTO_DOWN 0x2
#define XM_EFFECT_TONE_PORTAMENTO 0x3
#define XM_EFFECT_VIBRATO_VOLUME_SLIDE 0x6
#define XM_EFFECT_VOLUME_SLIDE 0xA
#define XM_EFFECT_SET_VOLUME 0xC
#define XM_EFFECT_PATTERN_BREAK 0xD
#define XM_EFFECT_SET_SPEED 0xF

/* The loop types of an XM sample the loader handles. */
#define XM_SAMPLE_NO_LOOP 0
#define XM_SAMPLE_FORWARD_LOOP 1

/* The bytes that follow each sample, so the mixer can read past its end before it loops. */
#define SAMPLE_TAIL 1200

/* The volume of a note, and the volume the music channels start at. */
#define NOTE_VOLUME 0x40
#define MUSIC_CHANNEL_VOLUME 0x20

/* A period of the music times its 32.32 step. */
#define PERIOD_STEP_PRODUCT 0xEE9FCFF0B5LL

/* The scales from the frequencies of the notes to their steps. */
#define NOTE_FREQUENCY_SCALE 0.0625
#define NOTE_BASE_FREQUENCY 261.7
#define NOTE_STEP_SCALE 0.380170924629511797
#define FIXED_32_32 4294967296.0

/* The rate of the CPU counter, in counts a second. */
#define CPU_COUNT_RATE 46875500.0

/* The mask of the sound effect mixer, which clears the low bit of each 16-bit sample. */
#define SOUND_EFFECT_SAMPLE_MASK 0xFFFEFFFEFFFEFFFEULL

/* The levels of a sound effect channel, the right in the upper half and the left in the lower. */
#define SOUND_LEVEL_MAX 32
#define SOUND_LEVELS_MIDDLE ((SOUND_LEVEL_MAX << 16) | SOUND_LEVEL_MAX)
#define SOUND_LEVELS_START 0x20

/* A step of one sample of the sample for each sample of the output. */
#define STEP_ONE (1ULL << 32)

/* The period of the audio timer, in microseconds. */
#define AUDIO_TIMER_USEC 10000

/* The mixing thread. */
#define AUDIO_THREAD_ID 7
#define AUDIO_THREAD_PRIORITY 0x7F

/* How far behind the end of the buffer being played the music mixes up to. */
#define MUSIC_LEAD 0x1000

/* How far ahead of the play position the sound effects mix up to, before the ring wraps. */
#define SOUND_EFFECT_LEAD 0x5000

/* The extra music mixers start at this channel and mix a pair at a time. */
#define FIRST_PAIRED_CHANNEL 6

/* The 32.32 step of a note counted across the octaves. */
#define NOTE_STEP(note) ((&g_aFreqScaleTable[0][0])[(u16)(note)])

/* Round a pointer up to a multiple of 4 bytes. */
#define ALIGN4(p) ((u8 *)(((size_t)(p) + 3) & ~(size_t)3))

/* A little-endian word of the module, read a byte at a time. */
#define READ_LE32(p) ((p)[0] + (p)[1] * 0x100 + (p)[2] * 0x10000 + (u32)(p)[3] * 0x1000000)

/* The packed size of a pattern, after its header. */
#define PATTERN_SIZE(p)                                                                            \
    ((p)[XM_PATTERN_PACKED_SIZE] + (p)[XM_PATTERN_PACKED_SIZE + 1] * 0x100 + READ_LE32(p))

void LoadMusicModule(u8 *module, u8 *work) {
    u8 *start = work;
    u8 *in, *sampleHeader, *src;
    u32 i, j, size, length, patterns, instruments, sum;
#if TGR_PLATFORM == TGR_PLATFORM_N64 && (defined(__mips__) || defined(__sgi))
    u32 header;
#endif

    g_MusicPlayer.orders = work;
    g_MusicPlayer.songLength = module[XM_SONG_LENGTH];
    src = module + XM_ORDERS;
    g_MusicPlayer.speed = module[XM_TEMPO];
    g_nAudioChannelCount = module[XM_CHANNELS];
    for (i = 0; i < g_MusicPlayer.songLength; i++) {
        *work++ = *src++;
    }
    work = ALIGN4(work);
    osSyncPrintf("Length = %d\n", g_MusicPlayer.songLength);
    g_MusicPlayer.restartOrder = module[XM_RESTART];
    osSyncPrintf("Restartfrom = %d\n", g_MusicPlayer.restartOrder);
    patterns = module[XM_PATTERNS];
    osSyncPrintf("%d Patterns found\n", patterns);
    instruments = module[XM_INSTRUMENTS];
    osSyncPrintf("%d Instruments found\n", instruments);

    /* Each pattern is copied whole, its header with its packed rows. */
#if TGR_PLATFORM == TGR_PLATFORM_N64 && (defined(__mips__) || defined(__sgi))
    /* The original reads the aligned header size as one word before byte swapping it. */
    header = *(u32 *)(module + XM_HEADER_SIZE);
    in = module + XM_HEADER_SIZE +
         (((header >> 24) & 0xFF) + ((header >> 16) & 0xFF) * 0x100 +
          ((header >> 8) & 0xFF) * 0x10000 + ((header & 0xFF) << 24));
#else
    in = module + READ_LE32(&module[XM_HEADER_SIZE]) + XM_HEADER_SIZE;
#endif
    for (i = 0; i < patterns; i++) {
        g_apPatternData[i] = work;
        size = PATTERN_SIZE(in);
        src = in;
        for (j = 0; j < size; j++) {
            *work++ = *src++;
        }
        /* The original reads the size again rather than reuse it. */
        in += PATTERN_SIZE(in);
    }
    work = ALIGN4(work);

    /*
     * The first sample of each instrument is kept: its header, its data decoded from the deltas of
     * the module, and then silence, or the start of its loop again, for the mixer to run into.
     */
    osSyncPrintf("Now doSamples");
    for (i = 0; i < instruments; i++) {
        osSyncPrintf(".");
        sampleHeader = in + READ_LE32(in);
        if (in[XM_INSTRUMENT_SAMPLES] <= 0) {
            g_apInstrumentData[i] = NULL;
            in = sampleHeader;
            continue;
        }
        length = READ_LE32(sampleHeader);
        g_apInstrumentData[i] = (XmSample *)work;
        for (j = 0; j < sizeof(XmSample); j++) {
            *work++ = sampleHeader[j];
        }
        g_apInstrumentData[i]->length = READ_LE32(sampleHeader);
        g_apInstrumentData[i]->loopLength = READ_LE32(&sampleHeader[XM_SAMPLE_LOOP_LENGTH]);
        src = sampleHeader + sizeof(XmSample);
        sum = 0;
        for (j = 0; j < length; j++) {
            sum += *src++;
            *work++ = sum;
        }
        switch (sampleHeader[XM_SAMPLE_TYPE]) {
        case XM_SAMPLE_NO_LOOP:
            for (j = 0; j < SAMPLE_TAIL; j++) {
                *work++ = 0;
            }
            break;
        case XM_SAMPLE_FORWARD_LOOP:
            src = work - READ_LE32(&sampleHeader[XM_SAMPLE_LOOP_LENGTH]);
            for (j = 0; j < SAMPLE_TAIL; j++) {
                *work++ = *src++;
            }
            break;
        default:
            osSyncPrintf("WANKER fuck off no 16bit, no Ping fucking pong\n");
            break;
        }
        work = ALIGN4(work);
        in = sampleHeader + length + sizeof(XmSample);
    }
    osSyncPrintf("\n\nSample Space used = %d bytes\n", work - start);
}

void BuildFrequencyScaleTable(void) {
    s32 octave, note;
    f32 scale;
    f64 frequency;

    for (octave = 0; octave < MUSIC_OCTAVES; octave++) {
        scale = 1 << octave;
        for (note = 0; note < MUSIC_NOTES; note++) {
            frequency = (f64)(g_afNoteFrequencies[note] * scale) * NOTE_FREQUENCY_SCALE;
            g_aFreqScaleTable[octave][note] =
                (u64)(frequency / NOTE_BASE_FREQUENCY * NOTE_STEP_SCALE * FIXED_32_32);
        }
    }
}

void CheckMusicLoopPoints(void) {
    s32 i;
    size_t end;
    s32 instrument;

    for (i = 0; i < g_nAudioChannelCount; i++) {
        instrument = g_MusicChannelStates[i].instrument;
        if (instrument != 0) {
            end = (size_t)g_apInstrumentData[instrument - 1] +
                  g_apInstrumentData[instrument - 1]->length + sizeof(XmSample);
            if (end < (size_t)g_aMixerChannels[i].sample) {
                if (g_apInstrumentData[instrument - 1]->type == XM_SAMPLE_NO_LOOP) {
                    g_aMixerChannels[i].step = 0;
                } else {
                    g_aMixerChannels[i].sample -= g_apInstrumentData[instrument - 1]->loopLength;
                }
            }
        }
    }
}

u8 *ParseMusicPatternRow(u8 *row) {
    s32 i;
    s8 flags;
    s8 note;
    u8 instrument;
    s8 effect;
    s8 parameter;
    u32 sampleIndex;
    s32 unused;
    MixerChannel *voice;
    s32 n;
    s32 unusedSlots[3];
    u64 target;

    /* These locals preserve the original player's stack layout under IDO. */
    (void)sizeof(unused);
    (void)sizeof(unusedSlots);
    for (i = 0; i < g_nAudioChannelCount; i++) {
        note = 0;
        effect = 0;
        parameter = 0;
        instrument = 0;
        flags = *row++;
        if (flags < 0) {
            if (flags & XM_PACKED_NOTE) {
                note = g_MusicChannelStates[i].note = *row++;
            }
            if (flags & XM_PACKED_INSTRUMENT) {
                instrument = g_MusicChannelStates[i].rowInstrument = *row++;
            }
            if (flags & XM_PACKED_VOLUME) {
                g_MusicChannelStates[i].volume = *row++;
            } else if (note != 0) {
                g_MusicChannelStates[i].volume = NOTE_VOLUME;
            }
            if (flags & XM_PACKED_EFFECT) {
                effect = g_MusicChannelStates[i].effect = *row++;
            } else {
                effect = 0;
            }
            if (flags & XM_PACKED_PARAMETER) {
                parameter = g_MusicChannelStates[i].effectParameter = *row++;
            } else {
                parameter = 0;
            }
        } else {
            note = g_MusicChannelStates[i].note = flags;
            instrument = g_MusicChannelStates[i].rowInstrument = *row++;
            g_MusicChannelStates[i].volume = *row++;
            effect = g_MusicChannelStates[i].effect = *row++;
            parameter = g_MusicChannelStates[i].effectParameter = *row++;
        }
        if (note != 0) {
            g_MusicChannelStates[i].note--;
        }
        if (instrument != 0 && g_MusicChannelStates[i].volume == 0) {
            g_MusicChannelStates[i].volume = NOTE_VOLUME;
        }
        g_MusicChannelStates[i].portamentoSpeed = 0;
        g_MusicChannelStates[i].volumeSlide = 0;
        sampleIndex = g_MusicChannelStates[i].instrument;
        g_MusicChannelStates[i].arpeggioOn = FALSE;

        switch (effect) {
        case XM_EFFECT_ARPEGGIO:
            if (parameter != 0) {
                n = g_MusicChannelStates[i].note & 0xff;
                g_MusicChannelStates[i].arpeggio[1] = n + (u32)((parameter >> 4) & 0xf);
                g_MusicChannelStates[i].arpeggio[2] = n + (u32)(parameter & 0xf);
                g_MusicChannelStates[i].arpeggioOn = TRUE;
                g_MusicChannelStates[i].arpeggio[0] = n;
            }
            break;
        case XM_EFFECT_PORTAMENTO_UP:
            g_MusicChannelStates[i].portamentoSpeed = g_MusicChannelStates[i].effectParameter;
            g_MusicChannelStates[i].portamentoTarget = 0;
            break;
        case XM_EFFECT_PORTAMENTO_DOWN:
            g_MusicChannelStates[i].portamentoSpeed = -g_MusicChannelStates[i].effectParameter;
            g_MusicChannelStates[i].portamentoTarget = 0;
            break;
        case XM_EFFECT_TONE_PORTAMENTO:
            if (sampleIndex != 0) {
                g_MusicChannelStates[i].portamentoSpeed = g_MusicChannelStates[i].effectParameter;
                note = 0;
                n = g_MusicChannelStates[i].note +
                    g_apInstrumentData[sampleIndex - 1]->relativeNote;
                g_MusicChannelStates[i].portamentoTarget = target = NOTE_STEP(n);
                if (g_aMixerChannels[i].step > target) {
                    g_MusicChannelStates[i].portamentoSpeed =
                        -g_MusicChannelStates[i].portamentoSpeed;
                }
            }
            break;
        case XM_EFFECT_VIBRATO_VOLUME_SLIDE:
        case XM_EFFECT_VOLUME_SLIDE:
            if (g_MusicChannelStates[i].effectParameter & 0xf0) {
                g_MusicChannelStates[i].volumeSlide =
                    (g_MusicChannelStates[i].effectParameter >> 4) & 0xf;
            } else {
                g_MusicChannelStates[i].volumeSlide =
                    -(g_MusicChannelStates[i].effectParameter & 0xf);
            }
            break;
        case XM_EFFECT_SET_VOLUME:
            g_MusicChannelStates[i].volume = parameter;
            break;
        case XM_EFFECT_PATTERN_BREAK:
            g_MusicPlayer.rowsLeft = 0;
            break;
        case XM_EFFECT_SET_SPEED:
            g_MusicPlayer.tick = g_MusicPlayer.speed =
                g_MusicChannelStates[i].effectParameter & 0xff;
            break;
        }

        /* A note starts the sample of the instrument of the row from its beginning. */
        if (note != 0) {
            voice = &g_aMixerChannels[i];
            g_MusicChannelStates[i].arpeggio[3] = 0;
            voice->fraction = 0;
            instrument = g_MusicChannelStates[i].instrument = g_MusicChannelStates[i].rowInstrument;
            voice->sample = (s8 *)(g_apInstrumentData[instrument - 1] + 1);
            n = g_MusicChannelStates[i].note + g_apInstrumentData[instrument - 1]->relativeNote;
            sampleIndex = g_MusicChannelStates[i].instrument;
            voice->step = NOTE_STEP(n);
        }
        if (sampleIndex != 0) {
            g_aMixerChannels[i].volume =
                (g_apInstrumentData[sampleIndex - 1]->volume * g_MusicChannelStates[i].volume) >> 6;
        }
    }
    return row;
}

void ResetMusicChannels(void) {
    s32 i;

    g_MusicPlayer.tick = 1;
    g_MusicPlayer.rowsLeft = 0;
    g_MusicPlayer.order = 0xFF;
    for (i = 0; i < g_nAudioChannelCount; i++) {
        g_MusicChannelStates[i].instrument = 0;
        g_MusicChannelStates[i].volume = NOTE_VOLUME;
        g_MusicChannelStates[i].portamentoTarget = 0;
    }
}

void UpdateMusicPlayback(void) {
    s32 i;
    u8 *pattern;
    /* The original player tick reserves this otherwise unused stack slot. */
    s32 unusedIndex;
    s64 period;
    s16 volume;
    u32 instrument;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    s64 step;
#else
    u64 step;
#endif
    u16 note;

    (void)sizeof(unusedIndex);
    if (--g_MusicPlayer.tick == 0) {
        g_MusicPlayer.tick = g_MusicPlayer.speed;
        if (g_MusicPlayer.rowsLeft == 0) {
            g_dwMusicPatternRow = 0;
            if (++g_MusicPlayer.order == g_MusicPlayer.songLength) {
                g_MusicPlayer.order = g_MusicPlayer.restartOrder;
            }
            pattern = g_apPatternData[g_MusicPlayer.orders[g_MusicPlayer.order]];
            g_MusicPlayer.row = pattern + XM_PATTERN_DATA;
            g_MusicPlayer.rowsLeft = pattern[XM_PATTERN_ROWS];
        }
        g_MusicPlayer.rowsLeft--;
        g_MusicPlayer.row = ParseMusicPatternRow(g_MusicPlayer.row);
    }

    for (i = 0; i < g_nAudioChannelCount; i++) {
        instrument = g_MusicChannelStates[i].instrument;
        if (g_MusicChannelStates[i].arpeggioOn && instrument != 0 &&
            g_aMixerChannels[i].step != 0) {
            note = g_MusicChannelStates[i].arpeggio[g_MusicChannelStates[i].arpeggio[3]] +
                   g_apInstrumentData[instrument - 1]->relativeNote;
            g_aMixerChannels[i].step = NOTE_STEP(note);
            /* The original stores the step before it wraps the step. */
            g_MusicChannelStates[i].arpeggio[3]++;
            g_MusicChannelStates[i].arpeggio[3] &= 3;
        }

        /* A portamento slides the period, which is inverse to the step, and stops at its target. */
        if (g_MusicChannelStates[i].portamentoSpeed != 0 && g_aMixerChannels[i].step != 0) {
            period = PERIOD_STEP_PRODUCT / g_aMixerChannels[i].step;
            period -= g_MusicChannelStates[i].portamentoSpeed;
            if (period < 1) {
                period = 1;
            }
            if (g_MusicChannelStates[i].portamentoTarget != 0) {
                step = PERIOD_STEP_PRODUCT / period;
                if (g_MusicChannelStates[i].portamentoSpeed < 0) {
                    if (step < g_MusicChannelStates[i].portamentoTarget) {
                        step = g_MusicChannelStates[i].portamentoTarget;
                    }
                } else if (g_MusicChannelStates[i].portamentoTarget < step) {
                    step = g_MusicChannelStates[i].portamentoTarget;
                }
                g_aMixerChannels[i].step = step;
            } else {
                g_aMixerChannels[i].step = PERIOD_STEP_PRODUCT / period;
            }
        }

        if (g_MusicChannelStates[i].volumeSlide != 0) {
            volume = g_MusicChannelStates[i].volume + g_MusicChannelStates[i].volumeSlide;
            if (volume < 0) {
                volume = 0;
            } else if (volume > NOTE_VOLUME) {
                volume = NOTE_VOLUME;
            }
            g_MusicChannelStates[i].volume = volume;
            if (instrument != 0) {
                g_aMixerChannels[i].volume =
                    (g_apInstrumentData[instrument - 1]->volume * g_MusicChannelStates[i].volume) >>
                    6;
            }
        }
    }
}

/* The SDL game starts and drives the mixers from its audio stream in src/sdl/music.cpp. */
#if TGR_PLATFORM != TGR_PLATFORM_SDL
void InitAudioSystem(void *module, void *work) {
    s32 i, j;
    u32 elapsed;

#if TGR_PLATFORM == TGR_PLATFORM_N64
    g_nSoundEffectSampleMaskUpper = -65538;
    g_dwSoundEffectSampleMaskLower = 0xFFFEFFFEU;
#else
    g_qwSoundEffectSampleMask = SOUND_EFFECT_SAMPLE_MASK;
#endif
    osSyncPrintf("Real Frequency is = %d\n", osAiSetFrequency(AUDIO_FREQUENCY));
    osCreateMesgQueue(&g_AudioMsgQueue, g_aAudioMsgBuf, 2);
    osSetTimer(&g_AudioTimer,
               AUDIO_TIMER_USEC * osClockRate / 1000000,
               AUDIO_TIMER_USEC * osClockRate / 1000000,
               &g_AudioMsgQueue,
               NULL);
    for (i = 0; i < g_nAudioChannelCount; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
        g_aMixerChannels[i].sampleAddress = (unsigned long)g_AudioDmaBuffer;
#else
        g_aMixerChannels[i].sample = (s8 *)g_AudioDmaBuffer;
#endif
        g_aMixerChannels[i].step = 0;
        g_aMixerChannels[i].volume = MUSIC_CHANNEL_VOLUME;
    }
    for (j = 0; j < SOUND_EFFECT_CHANNELS; j++) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
        g_aSoundEffectChannels[j].sampleAddress = (unsigned long)g_SilenceSampleData;
#else
        g_aSoundEffectChannels[j].sample = g_SilenceSampleData;
#endif
        g_aSoundEffectChannels[j].step = 0;
        g_aSoundEffectChannels[j].volume = SOUND_LEVELS_START;
    }
    g_wAudioBufferWritePos = 0;

    /* Time a mix of the whole ring buffer. */
    elapsed = -osGetCount();
    MixAudioSamples(g_AudioDmaBuffer, AUDIO_RING_BYTES);
    elapsed += osGetCount();
    osSyncPrintf("%1.7f", (f64)elapsed / CPU_COUNT_RATE);

    osSyncPrintf("Creating I entries\n");
    LoadMusicModule(module, work);
    osSyncPrintf("Creating Note Frequency entries\n");
    BuildFrequencyScaleTable();
    osSyncPrintf("Starting Mod\n");
    ResetMusicChannels();
    g_bMusicPlaybackEnabled = TRUE;
    osAiSetNextBuffer(g_AudioDmaBuffer, AUDIO_RING_BYTES);
    osCreateThread(&g_AudioThread,
                   AUDIO_THREAD_ID,
                   AudioMixingThreadMain,
                   NULL,
                   g_aAudioThreadStack + AUDIO_THREAD_STACK_WORDS,
                   AUDIO_THREAD_PRIORITY);
    osStartThread(&g_AudioThread);
}
#endif /* TGR_PLATFORM != TGR_PLATFORM_SDL */

void ResetMusicPlaybackState(void) {
    s32 i;

    g_bMusicPlaybackEnabled = FALSE;
    for (i = 0; i < g_nAudioChannelCount; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
        /* The original mixer stores the silence address as a 32-bit word. */
        g_aMixerChannels[i].sample = (s8 *)(u32)g_SilenceSampleData;
#else
        g_aMixerChannels[i].sample = g_SilenceSampleData;
#endif
        g_aMixerChannels[i].step = 0;
        g_aMixerChannels[i].volume = 0;
        g_MusicChannelStates[i].instrument = 0;
    }
}

void InitMusicPlayback(void *module, void *work) {
    s32 i;

    osSyncPrintf("Creating I entries\n");
    LoadMusicModule(module, work);
    osSyncPrintf("Starting Mod\n");
    ResetMusicChannels();
    g_bMusicPlaybackEnabled = TRUE;
    for (i = 0; i < g_nAudioChannelCount; i++) {
        g_aMixerChannels[i].volume = MUSIC_CHANNEL_VOLUME;
    }
}

s16 FindFreeSoundChannel(void) {
    s32 i;

    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        if (g_aSoundEffectChannels[i].step == 0) {
            return i;
        }
    }
    return -1;
}

void InitSoundChannel(s16 channel, s8 *sample, u32 length, u32 loopLength) {
    g_aSoundEffectSamples[channel].start = sample;
    g_aSoundEffectSamples[channel].length = length;
    g_aSoundEffectSamples[channel].loopLength = loopLength;
    g_aSoundEffectChannels[channel].mixVolume = 0;
    g_aSoundEffectChannels[channel].sample = sample;
    g_aSoundEffectChannels[channel].fraction = 0;
    g_aSoundEffectChannels[channel].volume = SOUND_LEVELS_MIDDLE;
    g_aSoundEffectChannels[channel].step = STEP_ONE;
}

void StartSoundChannelMuted(s16 channel, s8 *sample, u32 length, u32 loopLength) {
    g_aSoundEffectSamples[channel].start = sample;
    g_aSoundEffectSamples[channel].length = length;
    g_aSoundEffectSamples[channel].loopLength = loopLength;
    g_aSoundEffectChannels[channel].volume = 0;
    g_aSoundEffectChannels[channel].mixVolume = 0;
    g_aSoundEffectChannels[channel].sample = sample;
    g_aSoundEffectChannels[channel].fraction = 0;
    g_aSoundEffectChannels[channel].step = STEP_ONE;
}

void SetSoundChannelVolume(s16 channel, u32 levels) {
    /* The original masks a level that is too high with the maximum rather than clamping it. */
    if ((levels & 0xFFFF) > SOUND_LEVEL_MAX) {
        levels &= 0xFFFF0000 | SOUND_LEVEL_MAX;
    }
    if ((levels >> 16) > SOUND_LEVEL_MAX) {
        levels &= (SOUND_LEVEL_MAX << 16) | 0xFFFF;
    }
    g_aSoundEffectChannels[channel].volume = levels;
}

void CheckSoundEffectLoopPoints(void) {
    s32 i;

    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        if (g_aSoundEffectChannels[i].step != 0 &&
            g_aSoundEffectChannels[i].sample >=
                g_aSoundEffectSamples[i].start + g_aSoundEffectSamples[i].length) {
            if (g_aSoundEffectSamples[i].loopLength != 0) {
                g_aSoundEffectChannels[i].sample -= g_aSoundEffectSamples[i].loopLength;
            } else {
                g_aSoundEffectChannels[i].step = 0;
            }
        }
    }
}

#if TGR_PLATFORM != TGR_PLATFORM_SDL
#if TGR_PLATFORM == TGR_PLATFORM_N64
void AudioMixingThreadMain(void *arg) {
    static s32 odd = 0;
    static s32 readPos = 0;
    static u32 sfxPos = 0;
    static s32 bytes;
    static s32 aiPos;
    static s32 voice;
    u32 length;
    u16 save;
    u32 effectBytes;
    u32 low;
    u32 high;

    for (;;) {
        odd ^= 1;
        osRecvMesg(&g_AudioMsgQueue, NULL, OS_MESG_BLOCK);
        for (voice = 0; voice < g_nAudioChannelCount; voice++) {
            g_aMixerChannels[voice].mixVolume =
                (u32)(g_aMixerChannels[voice].volume * g_bBgmVolume * g_bMusicVolume) >> 16;
        }
        for (voice = 0; voice < SOUND_EFFECT_CHANNELS; voice++) {
            low = g_aSoundEffectChannels[voice].volume & 0xFFFF;
            high = (u32)g_aSoundEffectChannels[voice].volume >> 16;
            low = g_bSfxMasterVolume * low * g_bSfxVolume >> 16;
            high = g_bSfxMasterVolume * high * g_bSfxVolume >> 16;
            g_aSoundEffectChannels[voice].mixVolume = (high << 16) + low;
        }
        bytes = osAiGetStatus();
        if (!(bytes & AI_STATUS_FIFO_FULL)) {
            bytes = osAiSetNextBuffer(g_AudioDmaBuffer, AUDIO_RING_BYTES);
        }
        length = osAiGetLength();
        aiPos = length - MUSIC_LEAD;
        if (aiPos < 0) {
            aiPos += AUDIO_RING_BYTES;
        }
        bytes = readPos - aiPos;
        readPos = aiPos;
        if (bytes < 0) {
            bytes += AUDIO_RING_BYTES;
        }
        /* Empty checks preserve the original IDO control-flow graph. */
        if (length > AUDIO_RING_BYTES) {
        }
        if (bytes > AUDIO_RING_BYTES) {
        }
        CheckMusicLoopPoints();
        if (odd != 0 && g_bMusicPlaybackEnabled != 0) {
            UpdateMusicPlayback();
        }
        save = g_wAudioBufferWritePos;
        MixAudioSamples(g_AudioDmaBuffer, bytes);
        for (voice = FIRST_PAIRED_CHANNEL; voice < g_nAudioChannelCount; voice += 2) {
            g_wAudioBufferWritePos = save;
            MixAudioStereoSamples(g_AudioDmaBuffer,
                                  bytes,
                                  voice * sizeof(MixerChannel) -
                                      FIRST_PAIRED_CHANNEL * sizeof(MixerChannel));
        }
        effectBytes = (SOUND_EFFECT_LEAD - length) / 8 * 8 - sfxPos;
        if (effectBytes > AUDIO_RING_BYTES) {
            effectBytes -= AUDIO_RING_BYTES;
        }
        MixSoundEffectChannels(g_AudioDmaBuffer, effectBytes, sfxPos);
        CheckSoundEffectLoopPoints();
        sfxPos += effectBytes;
        if (sfxPos > AUDIO_RING_BYTES) {
            sfxPos -= AUDIO_RING_BYTES;
        }
        UpdateRumblePaks(0);
    }
}
#else
void AudioMixingThreadMain(void *arg) {
    MixerChannel *channel;
    u32 levels, length, bytes, offset;
    s32 position, mixBytes, i;
    u16 writePos;

    for (;;) {
        g_dwAudioTickToggle ^= 1;
        osRecvMesg(&g_AudioMsgQueue, NULL, OS_MESG_BLOCK);

        /* Scale the volumes of the music and of the sound effects. */
        for (i = 0; i < g_nAudioChannelCount; i++) {
            channel = &g_aMixerChannels[i];
            channel->mixVolume = ((u32)channel->volume * g_bBgmVolume * g_bMusicVolume) >> 16;
        }
        for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
            levels = g_aSoundEffectChannels[i].volume;
            g_aSoundEffectChannels[i].mixVolume =
                ((((levels >> 16) * g_bSfxMasterVolume * g_bSfxVolume) >> 16) << 16) +
                (((levels & 0xFFFF) * g_bSfxMasterVolume * g_bSfxVolume) >> 16);
        }
        g_dwAudioChannelLoopIdx = i;

        /* Queue the ring again when the interface has room, and find how far it has played. */
        g_dwAudioMixBytes = osAiGetStatus();
        if (!(g_dwAudioMixBytes & AI_STATUS_FIFO_FULL)) {
            g_dwAudioMixBytes = osAiSetNextBuffer(g_AudioDmaBuffer, AUDIO_RING_BYTES);
        }
        length = osAiGetLength();
        position = length - MUSIC_LEAD;
        if (position < 0) {
            position += AUDIO_RING_BYTES;
        }
        mixBytes = g_dwAiDmaPrevLength - position;
        g_dwAiDmaPrevLength = position;
        g_dwAiDmaCurrentLength = position;
        if (mixBytes < 0) {
            mixBytes += AUDIO_RING_BYTES;
        }
        g_dwAudioMixBytes = mixBytes;
        CheckMusicLoopPoints();
        if (g_dwAudioTickToggle) {
            g_dwAudioMixBytes = mixBytes;
            if (g_bMusicPlaybackEnabled) {
                UpdateMusicPlayback();
            }
        }

        /* The music: six channels, then the rest in pairs over the same bytes. */
        writePos = g_wAudioBufferWritePos;
        MixAudioSamples(g_AudioDmaBuffer, g_dwAudioMixBytes);
        g_dwAudioChannelLoopIdx = FIRST_PAIRED_CHANNEL;
        if (g_nAudioChannelCount > FIRST_PAIRED_CHANNEL) {
            i = FIRST_PAIRED_CHANNEL;
            do {
                g_wAudioBufferWritePos = writePos;
                g_dwAudioChannelLoopIdx = i;
                MixAudioStereoSamples(g_AudioDmaBuffer,
                                      g_dwAudioMixBytes,
                                      i * sizeof(MixerChannel) -
                                          FIRST_PAIRED_CHANNEL * sizeof(MixerChannel));
                i = g_dwAudioChannelLoopIdx + 2;
            } while (i < g_nAudioChannelCount);
            g_dwAudioChannelLoopIdx = i;
        }

        /* The sound effects, ahead of the play position. */
        offset = g_dwSoundEffectMixOffset;
        bytes = (((u32)(SOUND_EFFECT_LEAD - length) >> 3) << 3) - offset;
        if (bytes > AUDIO_RING_BYTES) {
            bytes -= AUDIO_RING_BYTES;
        }
        g_dwSoundEffectMixOffset = offset;
        MixSoundEffectChannels(g_AudioDmaBuffer, bytes, offset);
        CheckSoundEffectLoopPoints();
        g_dwSoundEffectMixOffset += bytes;
        if (g_dwSoundEffectMixOffset > AUDIO_RING_BYTES) {
            g_dwSoundEffectMixOffset -= AUDIO_RING_BYTES;
        }

        UpdateRumblePaks(0);
    }
    /* The original ends with an epilogue that the endless loop never arrives at. */
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
#endif /* TGR_PLATFORM != TGR_PLATFORM_SDL */
