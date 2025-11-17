#include "if/adpcm_structs.h"

 .global ADPCM_StepTable
 .global ADPCM_IndexTable_4bit
 .global ADPCM_DitherState

 //#define ADPCM_DITHER
 //#define ADPCM_DITHER_SHIFT 24
 //#define ADPCM_CLAMP

 .arm
 .align
 .global UnCompWrite32bit_8bit
 .type UnCompWrite32bit_8bit,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
UnCompWrite32bit_8bit:
    @ Decode a frame of ADPCM data
    @ r0: pointer to ADPCM frame data, must be 4-byte-aligned (trashed)
    @ r1: size of ADPCM frame data (trashed)
    @ r2: pointer to the 8bit sample buffer, must be 4-byte-aligned (trashed)
    @ r3 trashed, r4-r12 and r14 used and saved / restored
    push {r4 - r12, r14}
    ldr r9, =#0x7FFFFFFF @ used for clamping
    sub r1, r1, #ADPCM_FRAMEHEADER_SIZE @ calculate ADPCM data size
    ldrh r14, [r0], #ADPCM_FRAMEHEADER_SIZE @ load first header half-word and advance past header
    and r14, r14, #0x60 @ get number of channels into r14
    mov r14, r14, lsr #5
    cmp r14, #1
    movgt r1, r1, lsr #1 @ divide size of data by two for stereo data
.adpcm_ucw32_8_channel_loop:
    ldrsh r3, [r0], #2 @ load first verbatim PCM sample to r3
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    sub r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsb r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r3, r9, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
#endif
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store first 8-bit PCM sample
    ldrsh r4, [r0], #2 @ load first index into r4
    ldr r10, =ADPCM_StepTable
    ldr r11, =ADPCM_IndexTable_4bit
    sub r12, r1, #4 @ r12 stores ADPCM byte count per channel for loop (-4 as we have already read 4 bytes)
.adpcm_ucw32_8_sample_loop:
    ldrb r5, [r0], #1 @ load two ADPCM nibbles to r5
    ldrsh r6, [r10, r4] @ load step to r6
    mov r7, r6, asr #3 @ load delta to r7
    tst r5, #0x01 @ ADPCM value & 1?
    addne r7, r7, r6, asr #2
    tst r5, #0x02 @ ADPCM value & 2?
    addne r7, r7, r6, asr #1
    tst r5, #0x04 @ ADPCM value & 4?
    addne r7, r7, r6
    tst r5, #0x08 @ ADPCM value & 8?
    subne r3, r7
    addeq r3, r7
    and r6, r5, #0x07
    ldrsh r7, [r11, r6] @ load index into r7 and 
    adds r4, r4, r7 @ add to old index in r4
    @ clamp index in r4 to [0, 88]
    movmi r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    subs r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsb r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r3, r9, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
#endif
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store first nibble / 8-bit PCM sample
    @ if not last sample
    cmp r12, #0
    beq .adpcm_ucw32_8_sample_end
    @ decode second nibble
    ldrsh r6, [r10, r4] @ load step to r6
    mov r7, r6, asr #3 @ load delta to r7
    tst r5, #0x10 @ ADPCM value & 0x10?
    addne r7, r7, r6, asr #2
    tst r5, #0x20 @ ADPCM value & 0x20?
    addne r7, r7, r6, asr #1
    tst r5, #0x40 @ ADPCM value & 0x40?
    addne r7, r7, r6
    tst r5, #0x80 @ ADPCM value & 0x80?
    subne r3, r7
    addeq r3, r7
    mov r6, r5, lsr #4
    and r6, r6, #0x7
    ldrsh r7, [r11, r6]
    adds r4, r4, r7
    @ clamp index in r4 to [0, 88]
    movmi r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    sub r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsb r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r3, r9, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
#endif
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store second nibble / 8-bit PCM sample
    @ decrease number of ADPCM bytes left. if not last byte, loop again
    subs r12, #1
    bpl .adpcm_ucw32_8_sample_loop
.adpcm_ucw32_8_sample_end:
    @ decrease number of channels left. if not last channel, run sample loop again
    subs r14, #1
    bne .adpcm_ucw32_8_channel_loop
    pop {r4 - r12, r14}
    bx lr

 .arm
 .align
 .global UnCompGetSize_8bit
 .type UnCompGetSize_8bit,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
UnCompGetSize_8bit:
    @ Calculate decompressed PCM data size
    @ r0: pointer to ADPCM frame data, must be 4-byte-aligned
    ldrh r1, [r0] @ load first header half-word header into r1
    ldrh r0, [r0, #2] @ load uncompressed size into r0
    lsl r0, r0, #3
    add r0, r0, #7
    and r1, r1, #0x1F80 @ get PCM bits per sample into r1
    mov r1, r1, lsr #7
    cmp r1, #8 @ 8 bits / sample
    moveq r0, r0, lsr #3
    beq .adpcm_ucgs_8_end
    cmp r1, #16 @ 16 bits / sample
    moveq r0, r0, lsr #4
    beq .adpcm_ucgs_8_end
    cmp r1, #24 @ 24 bits / sample
    ldr r1, =2863311531
    muleq r1, r0, r1
    moveq r0, r1, lsr #4
    beq .adpcm_ucgs_8_end
    cmp r1, #32 @ 32 bits / sample
    moveq r0, r0, lsr #5
    beq .adpcm_ucgs_8_end
    eor r0, r0, r0
.adpcm_ucgs_8_end:
    bx lr
