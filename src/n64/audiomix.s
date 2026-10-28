/*
 * The hand-written routines of the audio mixer: two copies of 64-bit words, and three mixers of
 * 8-bit samples into the 16-bit stereo ring buffer of 0x4000 bytes.
 *
 * A mixer channel record holds a 32.32 fixed-point sample address, a 32.32 step, and a volume:
 *
 *     0x00  s64  position, whose upper word addresses the next sample
 *     0x08  s64  step added before each sample
 *     0x10  s32  volume, multiplied by each signed sample
 *
 * The instructions follow the original. Their delay slots are explicit, so the assembler must not
 * reorder them, and the mixers use $gp and $fp as general registers.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

/* Copy 16 bytes between 8-byte aligned addresses: a0 is the destination and a1 the source. */
    .globl CopyVec4
    .type CopyVec4, @function
CopyVec4:
    ld         $t4, 0x0($a1)
    ld         $t6, 0x8($a1)
    sd         $t4, 0x0($a0)
    sd         $t6, 0x8($a0)
    jr         $ra
    nop
    .size CopyVec4, . - CopyVec4

    .balign 16

/*
 * Fill a1 bytes from a0, rounded up to 64, with the 16-bit value of a2. The address must be 8-byte
 * aligned. The value is replicated into the four halfwords of each doubleword.
 */
    .globl MemFill64Aligned
    .type MemFill64Aligned, @function
MemFill64Aligned:
    addiu      $a1, $a1, 0x3F
    andi       $a1, $a1, 0xFFC0
    addu       $a1, $a1, $a0
    dsll       $a3, $a2, 16
    dsll       $a3, $a3, 16
    or         $a2, $a2, $a3
    sd         $a2, 0x0($a0)
.LMemFill64Aligned_loop:
    sd         $a2, 0x8($a0)
    sd         $a2, 0x10($a0)
    sd         $a2, 0x18($a0)
    addiu      $a0, $a0, 0x40
    sd         $a2, -0x20($a0)
    sd         $a2, -0x18($a0)
    sd         $a2, -0x10($a0)
    sd         $a2, -0x8($a0)
    bnel       $a1, $a0, .LMemFill64Aligned_loop
    sd         $a2, 0x0($a0)
    jr         $ra
    nop
    .size MemFill64Aligned, . - MemFill64Aligned

    .balign 16

/*
 * Mix a1 bytes of output from six channels into the ring buffer at a0, from its write position.
 * Channels 0, 2 and 4 of the mixer state feed the left samples, and channels 1, 3 and 5 the right
 * ones. The output replaces what the buffer held. The channel records start 8 bytes into the
 * state, which begins with the 16-bit write position.
 */
    .globl MixAudioSamples
    .type MixAudioSamples, @function
MixAudioSamples:
    addiu      $sp, $sp, -0x30
    sw         $s0, 0x0($sp)
    sw         $s1, 0x4($sp)
    sw         $s2, 0x8($sp)
    sw         $s3, 0xC($sp)
    sw         $a0, 0x10($sp)
    sw         $s4, 0x14($sp)
    sw         $s5, 0x18($sp)
    sw         $s6, 0x1C($sp)
    sw         $s7, 0x20($sp)
    sw         $fp, 0x24($sp)
    sw         $gp, 0x28($sp)
    lui        $t0, %hi(g_wAudioBufferWritePos)
    addiu      $t0, $t0, %lo(g_wAudioBufferWritePos)
    lhu        $a2, 0x0($t0)
    nop
    /* v1 receives the bytes past the end of the ring, which wrap to its start. */
    add        $a3, $a2, $a1
    addi       $v1, $a3, -0x4000
    bgtz       $v1, .LMixAudioSamples_split
    add        $a0, $a0, $a2
    lui        $v1, 0
.LMixAudioSamples_split:
    sub        $v0, $a1, $v1
    ld         $a1, 0x8($t0)
    ld         $a2, 0x10($t0)
    lw         $s0, 0x18($t0)
    ld         $a3, 0x38($t0)
    ld         $t1, 0x40($t0)
    lw         $s1, 0x48($t0)
    ld         $t2, 0x20($t0)
    ld         $t3, 0x28($t0)
    lw         $s2, 0x30($t0)
    ld         $t4, 0x50($t0)
    ld         $t5, 0x58($t0)
    lw         $s3, 0x60($t0)
    ld         $s4, 0x68($t0)
    ld         $s5, 0x70($t0)
    lw         $s6, 0x78($t0)
    ld         $s7, 0x80($t0)
    ld         $fp, 0x88($t0)
    lw         $gp, 0x90($t0)
    beqz       $v0, .LMixAudioSamples_wrap
    nop
.LMixAudioSamples_loop:
    daddu      $a1, $a1, $a2
    daddu      $a3, $a3, $t1
    daddu      $t2, $t2, $t3
    daddu      $t4, $t4, $t5
    daddu      $s4, $s4, $s5
    daddu      $s7, $s7, $fp
    addi       $v0, $v0, -0x4
    dsra32     $t9, $a1, 0
    lb         $t8, 0x0($t9)
    mult       $s0, $t8
    mflo       $t8
    dsra32     $t6, $a3, 0
    lb         $t9, 0x0($t6)
    mult       $s1, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    dsra32     $t6, $s4, 0
    lb         $t9, 0x0($t6)
    mult       $s6, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    sh         $t8, 0x0($a0)
    dsra32     $t7, $t2, 0
    lb         $t7, 0x0($t7)
    mult       $s2, $t7
    mflo       $t7
    dsra32     $t9, $t4, 0
    lb         $t9, 0x0($t9)
    mult       $s3, $t9
    mflo       $t9
    add        $t9, $t9, $t7
    dsra32     $t7, $s7, 0
    lb         $t7, 0x0($t7)
    mult       $gp, $t7
    mflo       $t7
    add        $t9, $t9, $t7
    sh         $t9, 0x2($a0)
    bgtz       $v0, .LMixAudioSamples_loop
    addi       $a0, $a0, 0x4
.LMixAudioSamples_wrap:
    beqz       $v1, .LMixAudioSamples_done
    addi       $v0, $v1, 0x0
    addi       $a0, $a0, -0x4000
    j          .LMixAudioSamples_loop
    sub        $v1, $v1, $v1
.LMixAudioSamples_done:
    sd         $a1, 0x8($t0)
    sd         $a3, 0x38($t0)
    sd         $t2, 0x20($t0)
    sd         $t4, 0x50($t0)
    sd         $s4, 0x68($t0)
    sd         $s7, 0x80($t0)
    lw         $a2, 0x10($sp)
    lw         $s0, 0x0($sp)
    lw         $s1, 0x4($sp)
    sub        $a0, $a0, $a2
    lw         $s2, 0x8($sp)
    lw         $s3, 0xC($sp)
    sh         $a0, 0x0($t0)
    lw         $s4, 0x14($sp)
    lw         $s5, 0x18($sp)
    lw         $s6, 0x1C($sp)
    lw         $s7, 0x20($sp)
    lw         $fp, 0x24($sp)
    lw         $gp, 0x28($sp)
    jr         $ra
    addiu      $sp, $sp, 0x30
    .size MixAudioSamples, . - MixAudioSamples

/*
 * Add a1 bytes of output from one pair of channels to the ring buffer at a0. a2 is the offset of
 * the pair from the mixer state, whose records for these channels start at 0x98.
 */
    .globl MixAudioStereoSamples
    .type MixAudioStereoSamples, @function
MixAudioStereoSamples:
    addiu      $sp, $sp, -0x30
    sw         $s0, 0x0($sp)
    sw         $s1, 0x4($sp)
    sw         $s2, 0x8($sp)
    sw         $a0, 0x10($sp)
    sw         $gp, 0x28($sp)
    lui        $t0, %hi(g_wAudioBufferWritePos)
    addiu      $t0, $t0, %lo(g_wAudioBufferWritePos)
    addu       $s2, $a2, $t0
    lhu        $a2, 0x0($t0)
    nop
    add        $a3, $a2, $a1
    addi       $v1, $a3, -0x4000
    bgtz       $v1, .LMixAudioStereoSamples_split
    add        $a0, $a0, $a2
    lui        $v1, 0
.LMixAudioStereoSamples_split:
    sub        $v0, $a1, $v1
    ld         $a1, 0x98($s2)
    ld         $a2, 0xA0($s2)
    lw         $s0, 0xA8($s2)
    ld         $a3, 0xB0($s2)
    ld         $t1, 0xB8($s2)
    lw         $s1, 0xC0($s2)
    beqz       $v0, .LMixAudioStereoSamples_wrap
    nop
.LMixAudioStereoSamples_loop:
    daddu      $a1, $a1, $a2
    daddu      $a3, $a3, $t1
    addi       $v0, $v0, -0x4
    dsra32     $t9, $a1, 0
    lb         $t8, 0x0($t9)
    mult       $s0, $t8
    mflo       $t8
    dsra32     $t6, $a3, 0
    lb         $t9, 0x0($t6)
    mult       $s1, $t9
    mflo       $t9
    lh         $t7, 0x0($a0)
    add        $t8, $t7, $t8
    sh         $t8, 0x0($a0)
    lh         $t7, 0x2($a0)
    add        $t9, $t7, $t9
    sh         $t9, 0x2($a0)
    bgtz       $v0, .LMixAudioStereoSamples_loop
    addi       $a0, $a0, 0x4
.LMixAudioStereoSamples_wrap:
    beqz       $v1, .LMixAudioStereoSamples_done
    addi       $v0, $v1, 0x0
    addi       $a0, $a0, -0x4000
    j          .LMixAudioStereoSamples_loop
    sub        $v1, $v1, $v1
.LMixAudioStereoSamples_done:
    sd         $a1, 0x98($s2)
    sd         $a3, 0xB0($s2)
    lw         $a2, 0x10($sp)
    lw         $s0, 0x0($sp)
    lw         $s1, 0x4($sp)
    lw         $s2, 0x8($sp)
    sub        $a0, $a0, $a2
    sh         $a0, 0x0($t0)
    lw         $gp, 0x28($sp)
    jr         $ra
    addiu      $sp, $sp, 0x30
    .size MixAudioStereoSamples, . - MixAudioStereoSamples

/*
 * Add a1 bytes of five sound effect channels to the ring buffer at a0 from offset a2, eight bytes
 * at a time. The sound effect state begins with the 64-bit sample mask, which clears the low bit of
 * each 16-bit sample, and its channel records start 8 bytes in. The volume of a record holds the
 * right level in its upper half and the left in its lower, so one product gives a stereo frame.
 * When the word at 0x98 of the state is set, channel 0 adds a second sample at that offset from
 * its own, an echo. The masked frame goes into both words of the doubleword, which is added to the
 * masked output.
 */
    .globl MixSoundEffectChannels
    .type MixSoundEffectChannels, @function
MixSoundEffectChannels:
    addiu      $sp, $sp, -0x28
    sw         $s0, 0x0($sp)
    sw         $s1, 0x4($sp)
    sw         $s2, 0x8($sp)
    sw         $s3, 0xC($sp)
    sw         $a0, 0x10($sp)
    sw         $s4, 0x14($sp)
    sw         $s5, 0x18($sp)
    sw         $s6, 0x1C($sp)
    lui        $t0, %hi(g_qwSoundEffectSampleMask)
    addiu      $t0, $t0, %lo(g_qwSoundEffectSampleMask)
    add        $a3, $a2, $a1
    addi       $v1, $a3, -0x4000
    bgtz       $v1, .LMixSoundEffectChannels_split
    add        $a0, $a0, $a2
    lui        $v1, 0
.LMixSoundEffectChannels_split:
    sub        $v0, $a1, $v1
    ld         $a1, 0x8($t0)
    ld         $a2, 0x10($t0)
    lw         $s0, 0x18($t0)
    ld         $a3, 0x20($t0)
    ld         $t1, 0x28($t0)
    lw         $s1, 0x30($t0)
    ld         $t2, 0x38($t0)
    ld         $t3, 0x40($t0)
    lw         $s2, 0x48($t0)
    ld         $t4, 0x50($t0)
    ld         $t5, 0x58($t0)
    lw         $s3, 0x60($t0)
    ld         $s4, 0x68($t0)
    ld         $s5, 0x70($t0)
    lw         $s6, 0x78($t0)
    beqz       $v0, .LMixSoundEffectChannels_wrap
    nop
.LMixSoundEffectChannels_loop:
    daddu      $a1, $a1, $a2
    daddu      $a3, $a3, $t1
    daddu      $t2, $t2, $t3
    daddu      $t4, $t4, $t5
    daddu      $s4, $s4, $s5
    lw         $t8, 0x98($t0)
    ld         $t7, 0x0($a0)
    beqz       $t8, .LMixSoundEffectChannels_noDelay
    dsra32     $t9, $a1, 0
    addu       $t8, $t8, $t9
    lb         $t8, 0x0($t8)
.LMixSoundEffectChannels_noDelay:
    lb         $t9, 0x0($t9)
    add        $t9, $t8, $t9
    mult       $s0, $t9
    mflo       $t8
    dsra32     $t9, $a3, 0
    lb         $t9, 0x0($t9)
    mult       $s1, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    dsra32     $t9, $t2, 0
    lb         $t9, 0x0($t9)
    mult       $s2, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    dsra32     $t9, $t4, 0
    lb         $t9, 0x0($t9)
    mult       $s3, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    dsra32     $t9, $s4, 0
    lb         $t9, 0x0($t9)
    mult       $s6, $t9
    mflo       $t9
    add        $t8, $t8, $t9
    ld         $t6, 0x0($t0)
    and        $t8, $t8, $t6
    dsll32     $t9, $t8, 0
    daddu      $t9, $t9, $t8
    and        $t7, $t7, $t6
    daddu      $t9, $t9, $t7
    and        $t9, $t9, $t6
    addi       $v0, $v0, -0x8
    sd         $t9, 0x0($a0)
    bgtz       $v0, .LMixSoundEffectChannels_loop
    addi       $a0, $a0, 0x8
.LMixSoundEffectChannels_wrap:
    beqz       $v1, .LMixSoundEffectChannels_done
    addi       $v0, $v1, 0x0
    addi       $a0, $a0, -0x4000
    j          .LMixSoundEffectChannels_loop
    sub        $v1, $v1, $v1
.LMixSoundEffectChannels_done:
    sd         $a1, 0x8($t0)
    sd         $a3, 0x20($t0)
    sd         $t2, 0x38($t0)
    sd         $t4, 0x50($t0)
    sd         $s4, 0x68($t0)
    lw         $s0, 0x0($sp)
    lw         $s1, 0x4($sp)
    lw         $s2, 0x8($sp)
    lw         $s3, 0xC($sp)
    lw         $a0, 0x10($sp)
    lw         $s4, 0x14($sp)
    lw         $s5, 0x18($sp)
    lw         $s6, 0x1C($sp)
    jr         $ra
    addiu      $sp, $sp, 0x28
    .size MixSoundEffectChannels, . - MixSoundEffectChannels
