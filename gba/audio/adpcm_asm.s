#include "if/adpcm_structs.h"

 .global ADPCM_StepTable
 .global ADPCM_IndexTable_4bit

 ADPCM_dither:      .word 0x159A55A0 @ From the KISS generator
 ADPCM_last_dither: .word 0

 #define DITHER_SHIFT 24

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
    ldr r9,= #0x7FFFFFFF @ load clamp value into r9
    ldrh r3, [r0], #ADPCM_FRAMEHEADER_SIZE @ load first header half-word header
    sub r1, r1, #ADPCM_FRAMEHEADER_SIZE @ calculate ADPCM data size
    and r3, r3, #0x60, lsr #5 @ get number of channels into r3
    cmp r3, #1
    ble .adpcm_ucw32_8_mono
    lsr r1, r1, #1 @ divide size of data by two for stereo
.adpcm_ucw32_8_mono:
    sub r12, r1, #4 @ r12 stores ADPCM sample count per channel
.adpcm_ucw32_8_channel_loop:
    ldrh r3, [r0], #2 @ load first verbatim PCM sample to r3
#ifdef DITHER
    @ dither sample
    ldr r4, [dither]
    adds r3, dither, #lsr #DITHER_SHIFT
    subs r3, 
#endif
    @ clamp PCM data in r3 to [-32768, 32767], truncate to 8-bit and store in r8
    mov r8, #32768
    rsb r8, r8, #0
    cmp r3, r8
    movlt r3, r8
    mov r8, #32768
    sub r8, #1
    cmp r3, r8
    movgt r3, r8
    lsr r8, r3, #8
    strb r8, [r2], #1 @ store first PCM sample
    ldr r10, =ADPCM_StepTable
    ldr r11, =ADPCM_IndexTable_4bit
    ldrb r4, [r0], #2 @ load first index into r4
.adpcm_ucw32_8_sample_loop:
    ldrb r5, [r0], #1 @ load two ADPCM nibbles to r5
    ldrh r6, [r10, r4] @ load step to r6
    lsr r7, r6, #3 @ load delta to r7
    ands r8, r5, #0x01 @ ADPCM value & 1?
    addne r7, r7, r6, #lsr 2
    ands r8, r5, #0x02 @ ADPCM value & 2?
    addne r7, r7, r6, #lsr 1
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
    @ clamp PCM data in r3 to [-32768, 32767], truncate to 8-bit and store in r8
    mov r8, #32768
    rsb r8, r8, #0
    cmp r3, r8
    movlt r3, r8
    mov r8, #32768
    sub r8, #1
    cmp r3, r8
    movgt r3, r8
    lsr r8, r3, #8
    strb r8, [r2], #1 @ store first nibble/sample
    @ decode second nibble only if not last sample
    cmp r12, #0
    beq .adpcm_ucw32_8_end
    ldrh r6, [r10, r4] @ load step to r6
    lsr r7, r6, #3 @ load delta to r7
    ands r8, r5, #0x10 @ ADPCM value & 0x10?
    addne r7, r7, r6, #lsr 2
    ands r8, r5, #0x20 @ ADPCM value & 0x20?
    addne r7, r7, r6, #lsr 1
    ands r8, r5, #0x40 @ ADPCM value & 0x40?
    addne r7, r7, r6
    ands r8, r5, #0x80 @ ADPCM value & 0x80?
    subne r3, r7
    addeq r3, r7
    and r6, r5, #7, #lsr 4
    ldrh r7, [r11, r6]
    add r4, r4, r7
    @ clamp index in r4 to [0, 88]
    cmp r4, #0
    movlt r4, #0
    cmp r4, #88
    movgt r4, #88
    @ clamp PCM data in r3 to [-32768, 32767], truncate to 8-bit and store in r8
    mov r8, #32768
    rsb r8, r8, #0
    cmp r3, r8
    movlt r3, r8
    mov r8, #32768
    sub r8, #1
    cmp r3, r8
    movgt r3, r8
    lsr r8, r3, #8
    strb r8, [r2], #1 @ store second nibble/sample
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
    and r1, r1, 0x1F80, #lsr 7 @ get PCM bits per sample r1
    lsl r0, #8
    add r0, #7
    div r0, r1
    bx lr
