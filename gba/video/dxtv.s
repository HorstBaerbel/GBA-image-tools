#include "dxtv_constants.h"

 .global C2_ModeHalf_5bit
 .global C2C3_ModeThird_5bit

 .data
 .align 4
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
    dxtcolors: .hword 0,0,0,0

 .arm
 .align
 .global DecodeBlock4x4
 .type DecodeBlock4x4,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
DecodeBlock4x4:
    @ Decode a 4x4 block of DXTV data
    @ r0: pointer to DXTV source data, must be 2-byte-aligned (will point to the next DXTV data block on return)
    @ r1: pointer to the destination pixel buffer, must be 4-byte-aligned (trashed)
    @ r2: line stride in bytes (remains unchanged)
    @ r3: pointer to the previous destination pixel buffer, must be 4-byte-aligned (trashed)
    stmfd sp!, {r4 - r8, lr}
    ldrh r4, [r0], #2 @ load block type / color 0
    ands r5, r4, #BLOCK_IS_REF @ check if block is a motion compensated or DXT block
    beq .db4x4_dxt
.db4x4_mc:
    // ----- copy motion compensated block -----
    ands r5, r4, #BLOCK_FROM_PREV @ check if block is from the previous frame
    moveq r3, r1 @ r3 = same frame (r1) or previous frame (r3)
    // get x-offset of the source block
    and r5, r4, #BLOCK_MOTION_MASK
    subs r5, r5, #BLOCK_HALF_RANGE
    lsl r5, r5, #1 @ multiply x-offset by 2, because pixels are 2 bytes wide
    // get y-offset of the source block
    lsr r4, r4, #BLOCK_MOTION_Y_SHIFT
    and r4, r4, #BLOCK_MOTION_MASK
    subs r4, r4, #BLOCK_HALF_RANGE
    // calculate offset to source block
    mlas r6, r4, r2, r5 @ multiply y-offset by line stride and add x-offset
    adds r3, r3, r6 @ calculate source address
    // check if block is word-aligned
    ands r4, r3, #0x03 @ check if source pointer is aligned
    beq .mc4x4_aligned
.mc4x4_unaligned:
    // unaligned block copy
    // line 0
    ldrh r4, [r3, #0]
    strh r4, [r1, #0]
    ldrh r4, [r3, #2]
    strh r4, [r1, #2]
    ldrh r4, [r3, #4]
    strh r4, [r1, #4]
    ldrh r4, [r3, #6]
    strh r4, [r1, #6]
    // line 1
    ldrh r4, [r3, r2]!
    strh r4, [r1, r2]!
    ldrh r4, [r3, #2]
    strh r4, [r1, #2]
    ldrh r4, [r3, #4]
    strh r4, [r1, #4]
    ldrh r4, [r3, #6]
    strh r4, [r1, #6]
    // line 2
    ldrh r4, [r3, r2]!
    strh r4, [r1, r2]!
    ldrh r4, [r3, #2]
    strh r4, [r1, #2]
    ldrh r4, [r3, #4]
    strh r4, [r1, #4]
    ldrh r4, [r3, #6]
    strh r4, [r1, #6]
    // line 3
    ldrh r4, [r3, r2]!
    strh r4, [r1, r2]!
    ldrh r4, [r3, #2]
    strh r4, [r1, #2]
    ldrh r4, [r3, #4]
    strh r4, [r1, #4]
    ldrh r4, [r3, #6]
    strh r4, [r1, #6]
    b .db4x4_end
.mc4x4_aligned:
    // handle aligned block copy
    // line 0
    ldmia r3, {r4, r5}
    stmia r1, {r4, r5}
    add r3, r3, r2
    add r1, r1, r2
    // line 1
    ldmia r3, {r4, r5}
    stmia r1, {r4, r5}
    add r3, r3, r2
    add r1, r1, r2
    // line 2
    ldmia r3, {r4, r5}
    stmia r1, {r4, r5}
    add r3, r3, r2
    add r1, r1, r2
    // line 3
    ldmia r3, {r4, r5}
    stmia r1, {r4, r5}
    b .db4x4_end
.db4x4_dxt:
    // ----- uncompress DXT block -----
    ldrh r5, [r0], #2 @ load c1 to r5
    // get blue components of c0 and c1 to r6
    and r3, r4, #0x7c00
    and r8, r5, #0x7c00
    lsr r3, r3, #5
    orr r6, r3, r8, lsr #10
    // get green components of c0 and c1 to r7
    and r3, r4, #0x03E0
    and r8, r5, #0x03E0
    orr r7, r3, r8, lsr #5
    // get red components of c0 and c1 to r8
    and r3, r4, #0x001F
    and r8, r5, #0x001F
    orr r8, r8, r3, lsl #5
    // check which intermediate colors mode we're using
    cmp r4, r5
    bgt .dxt4x4_third
.dxt4x4_half:
    // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
    ldr r3, =C2_ModeHalf_5bit
    ldrb r6, [r3, r6]
    ldrb r7, [r3, r7]
    ldrb r8, [r3, r8]
    orr r6, r8, r6, lsl #10
    orr r6, r6, r7, lsl #5
    mov r7, #0x0000 @ r7 is c3, which is always black
    b .dxt4x4_store_colors
.dxt4x4_third:
    // calculate intermediate colors c2 and c3 at 1/3 and 2/3 of c0 and c1
    ldr r3, =C2C3_ModeThird_5bit
    ldr r6, [r3, r6, lsl #2]
    ldr r7, [r3, r7, lsl #2]
    ldr r8, [r3, r8, lsl #2]
    orr r7, r8, r7, lsl #5
    orr r7, r7, r6, lsl #10
    mov r6, r7, lsl #16 @ move c3 from the lower part of r7 to r6
    mov r6, r6, lsr #16
    lsr r7, r7, #16 @ move c2 to the lower part of r7
.dxt4x4_store_colors:
    // we now have c0, c1, c2 and c3 in r4, r5, r6 and r7. store them in the LUT
    ldr r3, =dxtcolors
    strh r4, [r3, #0] @ store c0
    strh r5, [r3, #2] @ store c1
    strh r6, [r3, #4] @ store c2
    strh r7, [r3, #6] @ store c3
.dxt4x4_decode:
    mov r8, #0x06 @ = index mask 0x03 << 1 because of halfword-indexing for ldrh
    // line 0
    ldrh r4, [r0], #2 @ load indices to r4
    and r5, r8, r4, lsl #1
    and r6, r8, r4, lsr #1
    ldrh r5, [r3, r5]
    ldrh r6, [r3, r6]
    orr r5, r5, r6, lsl #16
    and r6, r8, r4, lsr #3
    and r7, r8, r4, lsr #5
    ldrh r6, [r3, r6]
    ldrh r7, [r3, r7]
    orr r6, r6, r7, lsl #16
    stmia r1, {r5, r6}
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 1
    and r5, r8, r4, lsr #7
    and r6, r8, r4, lsr #9
    ldrh r5, [r3, r5]
    ldrh r6, [r3, r6]
    orr r5, r5, r6, lsl #16
    and r6, r8, r4, lsr #11
    and r7, r8, r4, lsr #13
    ldrh r6, [r3, r6]
    ldrh r7, [r3, r7]
    orr r6, r6, r7, lsl #16
    stmia r1, {r5, r6}
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 2
    ldrh r4, [r0], #2 @ load indices to r4
    and r5, r8, r4, lsl #1
    and r6, r8, r4, lsr #1
    ldrh r5, [r3, r5]
    ldrh r6, [r3, r6]
    orr r5, r5, r6, lsl #16
    and r6, r8, r4, lsr #3
    and r7, r8, r4, lsr #5
    ldrh r6, [r3, r6]
    ldrh r7, [r3, r7]
    orr r6, r6, r7, lsl #16
    stmia r1, {r5, r6}
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 3
    and r5, r8, r4, lsr #7
    and r6, r8, r4, lsr #9
    ldrh r5, [r3, r5]
    ldrh r6, [r3, r6]
    orr r5, r5, r6, lsl #16
    and r6, r8, r4, lsr #11
    and r7, r8, r4, lsr #13
    ldrh r6, [r3, r6]
    ldrh r7, [r3, r7]
    orr r6, r6, r7, lsl #16
    stmia r1, {r5, r6}
.db4x4_end:
    ldmfd sp!, {r4 - r8, lr}
    bx lr

 .arm
 .align
 .global UnDxtBlock8x8
 .type UnDxtBlock8x8,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
UnDxtBlock8x8:
    @ Uncompress a 4x4 block of DXT pixel data
    @ r0: pointer to source data (trashed)
    @ r1: pointer to the destination buffer (trashed)
    @ r2: line stride in pixels (remains unchanged)
    stmfd sp!, {r3 - r11}
    // get anchor colors c0 and c1
    ldrh r4, [r0], #2 @ load c0 to r3
    ldrh r5, [r0], #2 @ load c1 to r4
   // get blue components of c0 and c1 to r6
    and r6, r4, #0x7c00
    and r7, r5, #0x7c00
    lsr r6, r6, #5
    orr r6, r6, r7, lsr #10
    // get green components of c0 and c1 to r7
    and r7, r4, #0x03E0
    and r8, r5, #0x03E0
    orr r7, r7, r8, lsr #5
    // get red components of c0 and c1 to r8
    and r8, r5, #0x001F
    and r9, r4, #0x001F
    orr r8, r8, r9, lsl #5
    // check which intermediate colors mode we're using
    cmp r4, r5
    bgt .udb8x8_third
.udb8x8_half:
    // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
    ldr r9, =C2_ModeHalf_5bit
    ldrb r6, [r9, r6]
    ldrb r7, [r9, r7]
    ldrb r8, [r9, r8]
    orr r6, r8, r6, lsl #10
    orr r6, r6, r7, lsl #5
    mov r7, #0x0000 @ r7 is c3, which is always black
    b .udb8x8_store_colors
.udb8x8_third:
    // calculate intermediate colors c2 and c3 at 1/3 and 2/3 of c0 and c1
    ldr r9, =C2C3_ModeThird_5bit
    ldr r6, [r9, r6, lsl #2]
    ldr r7, [r9, r7, lsl #2]
    ldr r8, [r9, r8, lsl #2]
    orr r7, r8, r7, lsl #5
    orr r7, r7, r6, lsl #10
    mov r6, r7, lsl #16 @ move c3 from the lower part of r7 to r6
    mov r6, r6, lsr #16
    lsr r7, r7, #16 @ move c2 to the lower part of r7
.udb8x8_store_colors:
    // we now have c0, c1, c2 and c3 in r4, r5, r6 and r7. store them in the LUT
    ldr r3, =dxtcolors
    strh r4, [r3, #0] @ store c0
    strh r5, [r3, #2] @ store c1
    strh r6, [r3, #4] @ store c2
    strh r7, [r3, #6] @ store c3
.udb8x8_decode:
    mov r10, #0x06 @ = index mask 0x03 << 1 because of halfword-indexing for ldrh
    mov r11, #8 @ line counter, 8 lines to decode
.udb8x8_line:
    ldrh r4, [r0], #2 @ load indices to r4
    // load first 4 pixels
    and r5, r10, r4, lsl #1
    and r6, r10, r4, lsr #1
    ldrh r5, [r3, r5]
    ldrh r6, [r3, r6]
    orr r5, r5, r6, lsl #16
    and r6, r10, r4, lsr #3
    and r7, r10, r4, lsr #5
    ldrh r6, [r3, r6]
    ldrh r7, [r3, r7]
    orr r6, r6, r7, lsl #16
    // load second 4 pixels
    and r7, r10, r4, lsr #7
    and r8, r10, r4, lsr #9
    ldrh r7, [r3, r7]
    ldrh r8, [r3, r8]
    orr r7, r7, r8, lsl #16
    and r8, r10, r4, lsr #11
    and r9, r10, r4, lsr #13
    ldrh r8, [r3, r8]
    ldrh r9, [r3, r9]
    orr r8, r8, r9, lsl #16
    stmia r1, {r5 - r8} @ store all 8 pixels in the destination buffer
    add r1, r1, r2 @ increment destination pointer by line stride
    subs r11, r11, #1 @ decrement line counter
    bne .udb8x8_line @ repeat for the next line
.udb8x8_end:
    ldmfd sp!, {r3 - r11}
    bx lr

.arm
 .align
 .global CopyBlock8x8
 .type CopyBlock8x8,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
CopyBlock8x8:
    @ Copy a 8x8 block of pixels
    @ r0: pointer to source buffer (trashed)
    @ r1: pointer to the destination buffer (trashed)
    @ r2: line stride in pixels (remains unchanged)
    stmfd sp!, {r3 - r7}
    // check if block is word-aligned
    ands r3, r0, #0x03 @ check if source pointer is aligned
    beq .cb8x8_aligned
.cb8x8_unaligned:
    // unaligned block copy
    // line 0
    ldrh r3, [r0, #0] @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 1
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 2
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 3
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 4
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 5
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 6
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    add r1, r1, r2 @ increment destination pointer by line stride
    // line 7
    ldrh r3, [r0, r2]! @ r3 = hw0 (bits 15:0)
    ldr r4, [r0, #2] @ load 3 consecutive aligned words into r4, r5, r6
    ldr r5, [r0, #6]
    ldr r6, [r0, #10]
    ldrh r7, [r0, #14] @ r7 = hw7 (bits 15:0)
    orr r3, r3, r4, lsl #16 @ r3 now holds (hw1 << 16) | hw0 (word0)
    mov r4, r4, lsr #16     @ r4 now holds hw2
    orr r4, r4, r5, lsl #16 @ r4 now holds (hw3 << 16) | hw2 (word1)
    mov r5, r5, lsr #16     @ r5 now holds hw4
    orr r5, r5, r6, lsl #16 @ r5 now holds (hw5 << 16) | hw4 (word2)
    mov r6, r6, lsr #16     @ r6 now holds hw6
    orr r6, r6, r7, lsl #16 @ r6 now holds (hw7 << 16) | hw6 (word3)
    stmia r1, {r3, r4, r5, r6} @ store the 4 reassembled words from to destination
    b .cb8x8_end
.cb8x8_aligned:
    // aligned block copy
    mov r7, #7 @ number of lines to copy
.cb8x8_aligned_loop:
    ldmia r0, {r3 - r6}
    stmia r1, {r3 - r6}
    add r0, r0, r2
    add r1, r1, r2
    subs r7, r7, #1 @ decrement line counter
    bne .cb8x8_aligned_loop @ repeat for the next line
    ldmia r0, {r3 - r6}
    stmia r1, {r3 - r6}
.cb8x8_end:
    ldmfd sp!, {r3 - r7}
    bx lr
