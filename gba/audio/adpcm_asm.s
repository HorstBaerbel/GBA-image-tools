#include "if/adpcm_structs.h"

 .global ADPCM_StepTable
 .global ADPCM_IndexTable_4bit
 .global ADPCM_DitherState

 #define ADPCM_DITHER
 #define ADPCM_DITHER_SHIFT 24

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
    stmfd sp!, {r4 - r12}
    ldr r9, =#0x7FFFFFFF @ used for clamping
    sub r1, r1, #ADPCM_FRAMEHEADER_SIZE @ calculate ADPCM data size
    ldrh r3, [r0], #ADPCM_FRAMEHEADER_SIZE @ load first header half-word header
    and r3, r3, #0x60 @ get number of channels into r3
    mov r3, r3, lsr #5
    cmp r3, #1
    ble .adpcm_ucw32_8_mono
    lsr r1, r1, #1 @ divide size of data by two for stereo
.adpcm_ucw32_8_mono:
    sub r12, r1, #4 @ r12 stores ADPCM sample count per channel
.adpcm_ucw32_8_channel_loop:
    ldrh r3, [r0], #2 @ load first verbatim PCM sample to r3
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    subs r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsbs r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    adds r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r9, r3, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store first 8-bit PCM sample
    ldr r10, =ADPCM_StepTable
    ldr r11, =ADPCM_IndexTable_4bit
    ldrb r4, [r0], #2 @ load first index into r4
.adpcm_ucw32_8_sample_loop:
    ldrb r5, [r0], #1 @ load two ADPCM nibbles to r5
    ldrh r6, [r10, r4] @ load step to r6
    mov r7, r6, lsr #3 @ load delta to r7
    ands r8, r5, #0x01 @ ADPCM value & 1?
    addne r7, r7, r6, lsr #2
    ands r8, r5, #0x02 @ ADPCM value & 2?
    addne r7, r7, r6, lsr #1
    ands r8, r5, #0x04 @ ADPCM value & 4?
    addne r7, r7, r6
    ands r8, r5, #0x08 @ ADPCM value & 8?
    subne r3, r7
    addeq r3, r7
    and r6, r5, #7
    ldrh r7, [r11, r6]
    add r4, r4, r7
    @ clamp index in r4 to [0, 88]
    cmp r4, #0
    movlt r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    subs r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsbs r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    adds r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r9, r3, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store first nibble / 8-bit PCM sample
    @ if not last sample
    cmp r12, #0
    beq .adpcm_ucw32_8_end
    @ decode second nibble
    ldrh r6, [r10, r4] @ load step to r6
    mov r7, r6, lsr #3 @ load delta to r7
    ands r8, r5, #0x10 @ ADPCM value & 0x10?
    addne r7, r7, r6, lsr #2
    ands r8, r5, #0x20 @ ADPCM value & 0x20?
    addne r7, r7, r6, lsr #1
    ands r8, r5, #0x40 @ ADPCM value & 0x40?
    addne r7, r7, r6
    ands r8, r5, #0x80 @ ADPCM value & 0x80?
    subne r3, r7
    addeq r3, r7
    and r6, r5, #7
    mov r6, r6, lsr #4
    ldrh r7, [r11, r6]
    add r4, r4, r7
    @ clamp index in r4 to [0, 88]
    cmp r4, #0
    movlt r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r7, r8} @ load r7 = last_dither, r8 = dither
    subs r3, r3, r7 @ pcmData -= last_dither
    mov r7, r8 @ r7 = dither
    rsbs r8, r7, r8, lsl #4 @ r8 = (dither << 4) - dither
    eor r8, r8, #1 @ r8 ^= 1
    mov r7, r7, lsr #ADPCM_DITHER_SHIFT @ r7 = dither >> ADPCM_DITHER_SHIFT
    adds r3, r3, r7 @ pcmData += dither >> ADPCM_DITHER_SHIFT
    sub r6, r6, #8 @ move back to start of ADPCM_dither
    stmia r6, {r7, r8} @ store r7 = new last_dither, r8 = new dither
#endif
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r8, r3, lsl #16 @ r8 = r3 << 16
    cmp r3, r8, asr #16 @ shift back and sign-extend r8 and compare with r3. check if r3 fits into signed 16-bit
    eorne r8, r9, r3, asr #31 @ extract sign bit of r3. xor with r9 and apply to saturate
    mov r8, r3, asr #8 @ r8 = pcmData >> 8
    strb r8, [r2], #1 @ store second nibble / 8-bit PCM sample
    @ decrease number of ADPCM bytes left. if not last byte, loop again
    subs r12, #1
    bgt .adpcm_ucw32_8_sample_loop
.adpcm_ucw32_8_end:
    ldmfd sp!, {r4 - r12}
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
