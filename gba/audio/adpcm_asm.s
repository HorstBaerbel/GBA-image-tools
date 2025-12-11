#include "adpcm_structs.h"

 .global ADPCM_DeltaTable_4bit
 .global ADPCM_IndexTable_4bit
 .global ADPCM_DitherState

 #define ADPCM_DITHER
 #define ADPCM_DITHER_SHIFT 24
 //#define ADPCM_ROUNDING
 #define ADPCM_CLAMP

 .arm
 .align
 .global Adpcm_UnCompWrite32bit_8bit
 .type Adpcm_UnCompWrite32bit_8bit,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
Adpcm_UnCompWrite32bit_8bit:
    @ Decode a frame of ADPCM data
    @ r0: pointer to ADPCM frame data, must be 4-byte-aligned (trashed)
    @ r1: size of ADPCM frame data (trashed)
    @ r2: pointer to the 8bit sample buffer, must be 4-byte-aligned (trashed)
    @ r3 trashed, r4-r12 and r14 used and saved / restored
    @ r6,r7 are scratch registers
    push {r4 - r12, r14}

    @ store constants
    ldr r8, =#0x7FFFFFFF @ used for clamping PCM data
    ldr r9, =ADPCM_DeltaTable_4bit
    ldr r10, =ADPCM_IndexTable_4bit
#ifdef ADPCM_DITHER
    @ load dither state
    ldr r6, =ADPCM_DitherState
    ldmia r6, {r11, r12} @ load r11 = last_dither, r12 = dither
#endif

    @ read header
    sub r1, r1, #ADPCM_FRAMEHEADER_SIZE @ calculate ADPCM data size
    ldrh r6, [r0], #ADPCM_FRAMEHEADER_SIZE @ load first header half-word and advance past header
    and r6, r6, #0x60 @ get number of channels into r6
    mov r6, r6, lsr #5
    push {r6} @ push channel loop counter to stack
    cmp r6, #1
    movgt r1, r1, lsr #1 @ divide size of data by two for stereo data

.adpcm_ucw32_8_channel_loop:
    @ r3 = PCM data
    @ r4 = Current ADPCM index
    @ r5 = Current ADPCM byte (2*4 bit data)
    ldrsh r3, [r0], #2 @ load first verbatim PCM sample to r3
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    sub r3, r3, r11 @ pcmData -= last_dither
    mov r11, r12 @ r11 = dither
    rsb r12, r11, r12, lsl #4 @ r12 = (dither << 4) - dither
    eor r12, r12, #1 @ r12 ^= 1
    mov r11, r11, lsr #ADPCM_DITHER_SHIFT @ r11 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r11 @ pcmData += dither >> ADPCM_DITHER_SHIFT
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r7, r3, lsl #16 @ r7 = r3 << 16
    cmp r3, r7, asr #16 @ shift back and sign-extend r7 and compare with r3. check if r3 fits into signed 16-bit
    eorne r7, r3, r8, asr #31 @ extract sign bit of r3. xor with r8 and apply to saturate
#endif
#ifdef ADPCM_ROUNDING
    add r7, r3, #128 @ r7 = pcmData + 128
    mov r7, r7, asr #8 @ r7 = (pcmData + 128) >> 8
#else
    mov r7, r3, asr #8 @ r7 = pcmData >> 8
#endif
    strb r7, [r2], #1 @ store first 8-bit PCM sample
    ldrsh r4, [r0], #2 @ load first index into r4
    sub r14, r1, #4 @ r14 stores ADPCM byte count per channel for loop (-4 as we have already read 4 bytes)

.adpcm_ucw32_8_sample_loop:
    ldrb r5, [r0], #1 @ load two ADPCM nibbles to r5

    @ decode first nibble
    and r6, r5, #0x07 @ r6=nibble&7 
    mov r7, r4, lsl #4 @ r7=index*2*8, because uint16_t and 8 entries per index
    add r7, r6, lsl #1 @ r7=index*2*8 + (nibble&7)*2
    ldrh r7, [r9, r7] @ load delta to r7
    tst r5, #0x08 @ ADPCM value & 8?
    subne r3, r3, r7
    addeq r3, r3, r7
    ldrsb r7, [r10, r6] @ load index into r7 and 
    adds r4, r4, r7 @ add to old index in r4. sets flags
    @ clamp index in r4 to [0, 88]
    movmi r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    sub r3, r3, r11 @ pcmData -= last_dither
    mov r11, r12 @ r11 = dither
    rsb r12, r11, r12, lsl #4 @ r12 = (dither << 4) - dither
    eor r12, r12, #1 @ r12 ^= 1
    mov r11, r11, lsr #ADPCM_DITHER_SHIFT @ r11 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r11 @ pcmData += dither >> ADPCM_DITHER_SHIFT
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r7, r3, lsl #16 @ r7 = r3 << 16
    cmp r3, r7, asr #16 @ shift back and sign-extend r7 and compare with r3. check if r3 fits into signed 16-bit
    eorne r7, r3, r8, asr #31 @ extract sign bit of r3. xor with r8 and apply to saturate
#endif
#ifdef ADPCM_ROUNDING
    add r7, r3, #128 @ r7 = pcmData + 128
    mov r7, r7, asr #8 @ r7 = (pcmData + 128) >> 8
#else
    mov r7, r3, asr #8 @ r7 = pcmData >> 8
#endif
    strb r7, [r2], #1 @ store first nibble / 8-bit PCM sample

    @ if not last sample
    subs r14, r14, #1
    beq .adpcm_ucw32_8_sample_end

    @ decode second nibble
    mov r5, r5, lsr #4 @ r5=(nibble>>4) 
    and r6, r5, #0x07 @ r6=(nibble>>4)&7
    mov r7, r4, lsl #4 @ r7=index*2*8, because uint16_t and 8 entries per index
    add r7, r6, lsl #1 @ r7=index*2*8 + ((nibble>>4)&7)*2
    ldrh r7, [r9, r7] @ load delta to r7
    tst r5, #0x08 @ ADPCM value & 8?
    subne r3, r3, r7
    addeq r3, r3, r7
    ldrsb r7, [r10, r6] @ load index into r7 and 
    adds r4, r4, r7 @ add to old index in r4. sets flags
    @ clamp index in r4 to [0, 88]
    movmi r4, #0
    cmp r4, #88
    movgt r4, #88
#ifdef ADPCM_DITHER
    @ dither PCM data in r3
    sub r3, r3, r11 @ pcmData -= last_dither
    mov r11, r12 @ r11 = dither
    rsb r12, r11, r12, lsl #4 @ r12 = (dither << 4) - dither
    eor r12, r12, #1 @ r12 ^= 1
    mov r11, r11, lsr #ADPCM_DITHER_SHIFT @ r11 = dither >> ADPCM_DITHER_SHIFT
    add r3, r3, r11 @ pcmData += dither >> ADPCM_DITHER_SHIFT
#endif
#ifdef ADPCM_CLAMP
    @ clamp PCM data in r3 to [-32768, 32767]
    mov r7, r3, lsl #16 @ r7 = r3 << 16
    cmp r3, r7, asr #16 @ shift back and sign-extend r7 and compare with r3. check if r3 fits into signed 16-bit
    eorne r7, r3, r8, asr #31 @ extract sign bit of r3. xor with r8 and apply to saturate
#endif
#ifdef ADPCM_ROUNDING
    add r7, r3, #128 @ r7 = pcmData + 128
    mov r7, r7, asr #8 @ r7 = (pcmData + 128) >> 8
#else
    mov r7, r3, asr #8 @ r7 = pcmData >> 8
#endif
    strb r7, [r2], #1 @ store second nibble / 8-bit PCM sample

    @ decrease number of ADPCM bytes left. if not last byte, loop again
    cmp r14, #0
    bgt .adpcm_ucw32_8_sample_loop

.adpcm_ucw32_8_sample_end:
    @ decrease number of channels left. if not last channel, run sample loop again
    pop {r6}
    subs r6, r6, #1
    push {r6}
    bgt .adpcm_ucw32_8_channel_loop
    pop {r6}
#ifdef ADPCM_DITHER
    @ store dither state
    ldr r6, =ADPCM_DitherState
    stmia r6, {r11, r12} @ load r11 = last_dither, r12 = dither
#endif

    pop {r4 - r12, r14}
    bx lr

 .arm
 .align
 .global Adpcm_UnCompGetSize_8bit
 .type Adpcm_UnCompGetSize_8bit,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
Adpcm_UnCompGetSize_8bit:
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
