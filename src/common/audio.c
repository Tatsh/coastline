#include "common/audio.h"

#include <stddef.h>

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/syncprintf.h"

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

/* A little-endian word of the module, read a byte at a time from the high byte. */
#define READ_LE32(p) (((u32)(p)[3] << 24) + (p)[0] + ((p)[1] << 8) + ((p)[2] << 16))

/* The packed size of a pattern, after its header. */
#define PATTERN_SIZE(p)                                                                            \
    (READ_LE32(p) + ((p)[XM_PATTERN_PACKED_SIZE + 1] << 8) + (p)[XM_PATTERN_PACKED_SIZE])

void LoadMusicModule(u8 *module, u8 *work) {
    u8 *out = work;
    u8 *in, *sampleHeader, *loop;
    u32 i, j, size, length, patterns, instruments, sum;

    g_MusicPlayer.orders = work;
    g_MusicPlayer.songLength = module[XM_SONG_LENGTH];
    g_MusicPlayer.speed = module[XM_TEMPO];
    g_nAudioChannelCount = module[XM_CHANNELS];
    for (i = 0; i < g_MusicPlayer.songLength; i++) {
        *out++ = module[XM_ORDERS + i];
    }
    out = ALIGN4(out);
    osSyncPrintf("Length = %d\n", g_MusicPlayer.songLength);
    g_MusicPlayer.restartOrder = module[XM_RESTART];
    osSyncPrintf("Restartfrom = %d\n", g_MusicPlayer.restartOrder);
    patterns = module[XM_PATTERNS];
    osSyncPrintf("%d Patterns found\n", patterns);
    instruments = module[XM_INSTRUMENTS];
    osSyncPrintf("%d Instruments found\n", instruments);

    /* Each pattern is copied whole, its header with its packed rows. */
    in = module + READ_LE32(&module[XM_HEADER_SIZE]) + XM_HEADER_SIZE;
    for (i = 0; i < patterns; i++) {
        g_apPatternData[i] = out;
        size = PATTERN_SIZE(in);
        for (j = 0; j < size; j++) {
            *out++ = in[j];
        }
        /* The original reads the size again rather than reuse it. */
        in += PATTERN_SIZE(in);
    }
    out = ALIGN4(out);

    /*
     * The first sample of each instrument is kept: its header, its data decoded from the deltas of
     * the module, and then silence, or the start of its loop again, for the mixer to run into.
     */
    osSyncPrintf("Now doSamples");
    for (i = 0; i < instruments; i++) {
        osSyncPrintf(".");
        sum = 0;
        sampleHeader = in + READ_LE32(in);
        if (in[XM_INSTRUMENT_SAMPLES] <= 0) {
            g_apInstrumentData[i] = NULL;
            in = sampleHeader;
            continue;
        }
        length = READ_LE32(sampleHeader);
        g_apInstrumentData[i] = (XmSample *)out;
        for (j = 0; j < sizeof(XmSample); j++) {
            *out++ = sampleHeader[j];
        }
        g_apInstrumentData[i]->length = READ_LE32(sampleHeader);
        g_apInstrumentData[i]->loopLength = READ_LE32(&sampleHeader[XM_SAMPLE_LOOP_LENGTH]);
        for (j = 0; j < length; j++) {
            sum += sampleHeader[sizeof(XmSample) + j];
            *out++ = sum;
        }
        switch (sampleHeader[XM_SAMPLE_TYPE]) {
        case XM_SAMPLE_NO_LOOP:
            for (j = 0; j < SAMPLE_TAIL; j++) {
                *out++ = 0;
            }
            break;
        case XM_SAMPLE_FORWARD_LOOP:
            loop = out - READ_LE32(&sampleHeader[XM_SAMPLE_LOOP_LENGTH]);
            for (j = 0; j < SAMPLE_TAIL; j++) {
                *out++ = *loop++;
            }
            break;
        default:
            osSyncPrintf("WANKER fuck off no 16bit, no Ping fucking pong\n");
            break;
        }
        out = ALIGN4(out);
        in = sampleHeader + length + sizeof(XmSample);
    }
    osSyncPrintf("\n\nSample Space used = %d bytes\n", out - work);
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
    s8 *end;
    XmSample *sample;
    s32 instrument;

    for (i = 0; i < g_nAudioChannelCount; i++) {
        instrument = g_MusicChannelStates[i].instrument;
        if (instrument != 0) {
            sample = g_apInstrumentData[instrument - 1];
            end = (s8 *)sample + sample->length + sizeof(XmSample);
            if (end < g_aMixerChannels[i].sample) {
                if (sample->type == XM_SAMPLE_NO_LOOP) {
                    g_aMixerChannels[i].step = 0;
                } else {
                    g_aMixerChannels[i].sample -= sample->loopLength;
                }
            }
        }
    }
}

u8 *ParseMusicPatternRow(u8 *row) {
    MusicChannel *state;
    MixerChannel *mixer = g_aMixerChannels;
    XmSample *sample;
    s8 flags, note, effect, parameter;
    u8 instrument;

    for (state = g_MusicChannelStates; state < &g_MusicChannelStates[g_nAudioChannelCount];
         state++, mixer++) {
        flags = *row++;
        note = 0;
        instrument = 0;
        if (flags < 0) {
            if (flags & XM_PACKED_NOTE) {
                state->note = *row;
                note = *row++;
            }
            if (flags & XM_PACKED_INSTRUMENT) {
                state->rowInstrument = *row;
                instrument = *row++;
            }
            if (flags & XM_PACKED_VOLUME) {
                state->volume = *row++;
            } else if (note != 0) {
                state->volume = NOTE_VOLUME;
            }
            if (flags & XM_PACKED_EFFECT) {
                state->effect = *row;
                effect = *row++;
            } else {
                effect = 0;
            }
            if (flags & XM_PACKED_PARAMETER) {
                state->effectParameter = *row;
                parameter = *row++;
            } else {
                parameter = 0;
            }
        } else {
            state->note = flags;
            note = flags;
            state->rowInstrument = row[0];
            instrument = row[0];
            state->volume = row[1];
            state->effect = row[2];
            effect = row[2];
            state->effectParameter = row[3];
            parameter = row[3];
            row += 4;
        }
        if (note != 0) {
            state->note--;
        }
        if (instrument != 0 && state->volume == 0) {
            state->volume = NOTE_VOLUME;
        }
        state->portamentoSpeed = 0;
        state->volumeSlide = 0;
        instrument = state->instrument;
        state->arpeggioOn = FALSE;

        switch (effect) {
        case XM_EFFECT_ARPEGGIO:
            if (parameter != 0) {
                state->arpeggio[1] = state->note + ((parameter >> 4) & 0xF);
                state->arpeggio[2] = state->note + (parameter & 0xF);
                state->arpeggioOn = TRUE;
                state->arpeggio[0] = state->note;
            }
            break;
        case XM_EFFECT_PORTAMENTO_UP:
            state->portamentoTarget = 0;
            state->portamentoSpeed = state->effectParameter;
            break;
        case XM_EFFECT_PORTAMENTO_DOWN:
            state->portamentoSpeed = -state->effectParameter;
            state->portamentoTarget = 0;
            break;
        case XM_EFFECT_TONE_PORTAMENTO:
            /* Slide to the note rather than start it. */
            if (instrument != 0) {
                state->portamentoSpeed = state->effectParameter;
                sample = g_apInstrumentData[instrument - 1];
                note = 0;
                state->portamentoTarget = NOTE_STEP(sample->relativeNote + state->note);
                if (state->portamentoTarget < mixer->step) {
                    state->portamentoSpeed = -state->portamentoSpeed;
                }
            }
            break;
        case XM_EFFECT_VIBRATO_VOLUME_SLIDE:
        case XM_EFFECT_VOLUME_SLIDE:
            if (state->effectParameter & 0xF0) {
                state->volumeSlide = (state->effectParameter >> 4) & 0xF;
            } else {
                state->volumeSlide = -(state->effectParameter & 0xF);
            }
            break;
        case XM_EFFECT_SET_VOLUME:
            state->volume = parameter;
            break;
        case XM_EFFECT_PATTERN_BREAK:
            g_MusicPlayer.rowsLeft = 0;
            break;
        case XM_EFFECT_SET_SPEED:
            g_MusicPlayer.speed = state->effectParameter;
            g_MusicPlayer.tick = state->effectParameter;
            break;
        }

        /* A note starts the sample of the instrument of the row from its beginning. */
        if (note != 0) {
            instrument = state->rowInstrument;
            state->arpeggio[3] = 0;
            sample = g_apInstrumentData[instrument - 1];
            mixer->fraction = 0;
            state->instrument = instrument;
            mixer->sample = (s8 *)(sample + 1);
            mixer->step = NOTE_STEP(sample->relativeNote + state->note);
        }
        if (instrument != 0) {
            sample = g_apInstrumentData[instrument - 1];
            mixer->volume = (state->volume * sample->volume) >> 6;
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
    MusicChannel *state;
    MixerChannel *mixer = g_aMixerChannels;
    XmSample *sample;
    u8 *pattern;
    u8 instrument;
    s64 period;
    u64 step;
    s16 volume;

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

    for (state = g_MusicChannelStates; state < &g_MusicChannelStates[g_nAudioChannelCount];
         state++, mixer++) {
        instrument = state->instrument;
        if (state->arpeggioOn && instrument != 0 && mixer->step != 0) {
            sample = g_apInstrumentData[instrument - 1];
            mixer->step = NOTE_STEP(sample->relativeNote + state->arpeggio[state->arpeggio[3]]);
            /* The original stores the step before it wraps the step. */
            state->arpeggio[3]++;
            state->arpeggio[3] &= 3;
        }

        /* A portamento slides the period, which is inverse to the step, and stops at its target. */
        if (state->portamentoSpeed != 0 && mixer->step != 0) {
            period = (s64)(PERIOD_STEP_PRODUCT / mixer->step) - state->portamentoSpeed;
            if (period < 1) {
                period = 1;
            }
            if (state->portamentoTarget != 0) {
                step = PERIOD_STEP_PRODUCT / period;
                if (state->portamentoSpeed < 0) {
                    if (step < state->portamentoTarget) {
                        step = state->portamentoTarget;
                    }
                } else if (state->portamentoTarget < step) {
                    step = state->portamentoTarget;
                }
                mixer->step = step;
            } else {
                mixer->step = PERIOD_STEP_PRODUCT / period;
            }
        }

        if (state->volumeSlide != 0) {
            volume = state->volumeSlide + state->volume;
            if (volume < 0) {
                volume = 0;
            } else if (volume > NOTE_VOLUME) {
                volume = NOTE_VOLUME;
            }
            state->volume = volume;
            if (instrument != 0) {
                sample = g_apInstrumentData[instrument - 1];
                mixer->volume = (state->volume * sample->volume) >> 6;
            }
        }
    }
}

/* The SDL game starts and drives the mixers from its audio stream in src/sdl/music.cpp. */
#if TGR_PLATFORM != TGR_PLATFORM_SDL
void InitAudioSystem(void *module, void *work) {
    u32 elapsed;
    s32 frequency, i;

    g_qwSoundEffectSampleMask = SOUND_EFFECT_SAMPLE_MASK;
    frequency = osAiSetFrequency(AUDIO_FREQUENCY);
    osSyncPrintf("Real Frequency is = %d\n", frequency);
    osCreateMesgQueue(&g_AudioMsgQueue, g_aAudioMsgBuf, 2);
    osSetTimer(&g_AudioTimer,
               AUDIO_TIMER_USEC * osClockRate / 1000000,
               AUDIO_TIMER_USEC * osClockRate / 1000000,
               &g_AudioMsgQueue,
               NULL);
    for (i = 0; i < g_nAudioChannelCount; i++) {
        g_aMixerChannels[i].sample = (s8 *)g_AudioDmaBuffer;
        g_aMixerChannels[i].step = 0;
        g_aMixerChannels[i].volume = MUSIC_CHANNEL_VOLUME;
    }
    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
        g_aSoundEffectChannels[i].step = 0;
        g_aSoundEffectChannels[i].volume = SOUND_LEVELS_START;
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
        g_aMixerChannels[i].sample = g_SilenceSampleData;
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
#endif /* TGR_PLATFORM != TGR_PLATFORM_SDL */
