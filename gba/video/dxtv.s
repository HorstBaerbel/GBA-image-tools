 .arm
 .align
 .global C2_ModeHalf_5bit
 .global C2C3_ModeThird_5bit
 .global UnDxtBlock4x4
 .type UnDxtBlock4x4,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
UnDxtBlock4x4:
    @ Uncompress a 4x4 block of DXT pixel data
    @ r0: pointer to source data (trashed)
    @ r1: pointer to the destination buffer (trashed)
    @ r2: line stride in pixels (remains unchanged)
    stmfd sp!, {r3 - r9}
    // get anchor colors c0 and c1
    ldrh r3, [r0], #2 @ load c0 to r3
    ldrh r4, [r0], #2 @ load c1 to r4
    // get blue components of c0 and c1 to r5
    and r5, r3, #0x7c00
    and r6, r4, #0x7c00
    lsr r5, r5, #5
    orr r5, r5, r6, lsr #10
    // get green components of c0 and c1 to r6
    and r6, r3, #0x03E0
    and r7, r4, #0x03E0
    orr r6, r6, r7, lsr #5
    // get red components of c0 and c1 to r7
    and r7, r4, #0x001F
    and r8, r3, #0x001F
    orr r7, r7, r8, lsl #5
    // check which intermediate colors mode we're using
    cmp r3, r4
    bgt .udb4x4_third
.udb4x4_half:
    // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
    ldr r8, =C2_ModeHalf_5bit
    ldrb r5, [r8, r5]
    ldrb r6, [r8, r6]
    ldrb r7, [r8, r7]
    orr r5, r7, r5, lsl #10
    orr r5, r5, r6, lsl #5
    mov r6, #0x0000 @ r6 is c3, which is always black
    b .udb4x4_decode
.udb4x4_third:
    // calculate intermediate colors c2 and c3 at 1/3 and 2/3 of c0 and c1
    ldr r8, =C2C3_ModeThird_5bit
    ldr r5, [r8, r5, lsl #2]
    ldr r6, [r8, r6, lsl #2]
    ldr r7, [r8, r7, lsl #2]
    orr r6, r7, r6, lsl #5
    orr r6, r6, r5, lsl #10
    mov r5, r6, lsl #16 @ move c3 from the lower part of r6 to r5
    mov r5, r5, lsr #16
    lsr r6, r6, #16 @ move c2 to the lower part of r6
.udb4x4_decode:
    // we now have c0, c1, c2 and c3 in r3, r4, r5 and r6 respectively
    sub r1, r1, r2 @ decrease destination pointer by line stride (for first line)
    mov r9, #2 @ number of lines to decode
.udb4x4_line:
    ldrh r7, [r0], #2 @ load indices to r7
    // pixel 0, line 1
    ands r8, r7, #0x03 @ get the index of the color
    streqh r3, [r1, r2]! @ store c0, cost 5
    beq .udb4x4_skip0
    cmp r8, #2
    strloh r4, [r1, r2]! @ store c1, cost 7
    streqh r5, [r1, r2]! @ store c2, cost 7
    strhih r6, [r1, r2]! @ store c3, cost 7
.udb4x4_skip0:
    // pixel 1
    ands r8, r7, #0x0C @ get the index of the color
    streqh r3, [r1, #2] @ store c0
    beq .udb4x4_skip1
    cmp r8, #8
    strloh r4, [r1, #2] @ store c1
    streqh r5, [r1, #2] @ store c2
    strhih r6, [r1, #2] @ store c3
.udb4x4_skip1:
    // pixel 2
    ands r8, r7, #0x30 @ get the index of the color
    streqh r3, [r1, #4] @ store c0
    beq .udb4x4_skip2
    cmp r8, #32
    strloh r4, [r1, #4] @ store c1
    streqh r5, [r1, #4] @ store c2
    strhih r6, [r1, #4] @ store c3
.udb4x4_skip2:
    // pixel 3
    ands r8, r7, #0xC0 @ get the index of the color
    streqh r3, [r1, #6] @ store c0
    beq .udb4x4_skip3
    cmp r8, #128
    strloh r4, [r1, #6] @ store c1
    streqh r5, [r1, #6] @ store c2
    strhih r6, [r1, #6] @ store c3
.udb4x4_skip3:
    // pixel 0, line 2
    ands r8, r7, #0x300 @ get the index of the color
    streqh r3, [r1, r2]! @ store c0
    beq .udb4x4_skip4
    cmp r8, #512
    strloh r4, [r1, r2]! @ store c1
    streqh r5, [r1, r2]! @ store c2
    strhih r6, [r1, r2]! @ store c3
.udb4x4_skip4:
    // pixel 1
    ands r8, r7, #0xC00 @ get the index of the color
    streqh r3, [r1, #2] @ store c0
    beq .udb4x4_skip5
    cmp r8, #2048
    strloh r4, [r1, #2] @ store c1
    streqh r5, [r1, #2] @ store c2
    strhih r6, [r1, #2] @ store c3
.udb4x4_skip5:    
    // pixel 2
    ands r8, r7, #0x3000 @ get the index of the color
    streqh r3, [r1, #4] @ store c0
    beq .udb4x4_skip6
    cmp r8, #8192
    strloh r4, [r1, #4] @ store c1
    streqh r5, [r1, #4] @ store c2
    strhih r6, [r1, #4] @ store c3
.udb4x4_skip6:
    // pixel 3
    ands r8, r7, #0xC000 @ get the index of the color
    streqh r3, [r1, #6] @ store c0
    beq .udb4x4_skip7
    cmp r8, #32768
    strloh r4, [r1, #6] @ store c1
    streqh r5, [r1, #6] @ store c2
    strhih r6, [r1, #6] @ store c3
.udb4x4_skip7:
    subs r9, r9, #1 @ decrement line counter
    bne .udb4x4_line @ repeat for the next line
.udb4x4_end:
    ldmfd sp!, {r3 - r9}
    bx lr

.arm
 .align
 .global CopyBlock4x4
 .type CopyBlock4x4,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
CopyBlock4x4:
    @ Copy a 4x4 block of pixels
    @ r0: pointer to source buffer (trashed)
    @ r1: pointer to the destination buffer (trashed)
    @ r2: line stride in pixels (remains unchanged)
    stmfd sp!, {r3 - r4}
    // check if block is word-aligned
    ands r3, r0, #0x03 @ check if source pointer is aligned
    beq .cb4x4_aligned
.cb4x4_unaligned:
    // unaligned block copy
    // line 0
    ldrh r3, [r0, #0]
    strh r3, [r1, #0]
    ldrh r3, [r0, #2]
    strh r3, [r1, #2]
    ldrh r3, [r0, #4]
    strh r3, [r1, #4]
    ldrh r3, [r0, #6]
    strh r3, [r1, #6]
    // line 1
    ldrh r3, [r0, r2]!
    strh r3, [r1, r2]!
    ldrh r3, [r0, #2]
    strh r3, [r1, #2]
    ldrh r3, [r0, #4]
    strh r3, [r1, #4]
    ldrh r3, [r0, #6]
    strh r3, [r1, #6]
    // line 2
    ldrh r3, [r0, r2]!
    strh r3, [r1, r2]!
    ldrh r3, [r0, #2]
    strh r3, [r1, #2]
    ldrh r3, [r0, #4]
    strh r3, [r1, #4]
    ldrh r3, [r0, #6]
    strh r3, [r1, #6]
    // line 3
    ldrh r3, [r0, r2]!
    strh r3, [r1, r2]!
    ldrh r3, [r0, #2]
    strh r3, [r1, #2]
    ldrh r3, [r0, #4]
    strh r3, [r1, #4]
    ldrh r3, [r0, #6]
    strh r3, [r1, #6]
    b .cb4x4_end
.cb4x4_aligned:
    // handle aligned block copy
    // line 0
    ldmia r0, {r3 - r4}
    stmia r1, {r3 - r4}
    add r0, r0, r2
    add r1, r1, r2
    // line 1
    ldmia r0, {r3 - r4}
    stmia r1, {r3 - r4}
    add r0, r0, r2
    add r1, r1, r2
    // line 2
    ldmia r0, {r3 - r4}
    stmia r1, {r3 - r4}
    add r0, r0, r2
    add r1, r1, r2
    // line 3
    ldmia r0, {r3 - r4}
    stmia r1, {r3 - r4}
.cb4x4_end:
    ldmfd sp!, {r3 - r4}
    bx lr

 .arm
 .align
 .global C2_ModeHalf_5bit
 .global C2C3_ModeThird_5bit
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
    stmfd sp!, {r3 - r9}
    // get anchor colors c0 and c1
    ldrh r3, [r0], #2 @ load c0 to r3
    ldrh r4, [r0], #2 @ load c1 to r4
    // get blue components of c0 and c1 to r5
    and r5, r3, #0x7c00
    and r6, r4, #0x7c00
    lsr r5, r5, #5
    orr r5, r5, r6, lsr #10
    // get green components of c0 and c1 to r6
    and r6, r3, #0x03E0
    and r7, r4, #0x03E0
    orr r6, r6, r7, lsr #5
    // get red components of c0 and c1 to r7
    and r7, r4, #0x001F
    and r8, r3, #0x001F
    orr r7, r7, r8, lsl #5
    // check which intermediate colors mode we're using
    cmp r3, r4
    bgt .udb8x8_third
.udb8x8_half:
    // calculate intermediate color c2 at 1/2 of c0 and c1 with rounding and set c3 to black
    ldr r8, =C2_ModeHalf_5bit
    ldrb r5, [r8, r5]
    ldrb r6, [r8, r6]
    ldrb r7, [r8, r7]
    orr r5, r7, r5, lsl #10
    orr r5, r5, r6, lsl #5
    mov r6, #0x0000 @ r6 is c3, which is always black
    b .udb8x8_decode
.udb8x8_third:
    // calculate intermediate colors c2 and c3 at 1/3 and 2/3 of c0 and c1
    ldr r8, =C2C3_ModeThird_5bit
    ldr r5, [r8, r5, lsl #2]
    ldr r6, [r8, r6, lsl #2]
    ldr r7, [r8, r7, lsl #2]
    orr r6, r7, r6, lsl #5
    orr r6, r6, r5, lsl #10
    mov r5, r6, lsl #16 @ move c3 from the lower part of r6 to r5
    mov r5, r5, lsr #16
    lsr r6, r6, #16 @ move c2 to the lower part of r6
.udb8x8_decode:
    // we now have c0, c1, c2 and c3 in r3, r4, r5 and r6 respectively
    sub r1, r1, r2 @ decrease destination pointer by line stride (for first line)
    mov r9, #8 @ number of lines to decode
.udb8x8_line:
    ldrh r7, [r0], #2 @ load indices to r7
    // pixel 0, line 1
    ands r8, r7, #0x03 @ get the index of the color
    streqh r3, [r1, r2]! @ store c0, cost 5
    beq .udb8x8_skip0
    cmp r8, #2
    strloh r4, [r1, r2]! @ store c1, cost 7
    streqh r5, [r1, r2]! @ store c2, cost 7
    strhih r6, [r1, r2]! @ store c3, cost 7
.udb8x8_skip0:
    // pixel 1
    ands r8, r7, #0x0C @ get the index of the color
    streqh r3, [r1, #2] @ store c0
    beq .udb8x8_skip1
    cmp r8, #8
    strloh r4, [r1, #2] @ store c1
    streqh r5, [r1, #2] @ store c2
    strhih r6, [r1, #2] @ store c3
.udb8x8_skip1:
    // pixel 2
    ands r8, r7, #0x30 @ get the index of the color
    streqh r3, [r1, #4] @ store c0
    beq .udb8x8_skip2
    cmp r8, #32
    strloh r4, [r1, #4] @ store c1
    streqh r5, [r1, #4] @ store c2
    strhih r6, [r1, #4] @ store c3
.udb8x8_skip2:
    // pixel 3
    ands r8, r7, #0xC0 @ get the index of the color
    streqh r3, [r1, #6] @ store c0
    beq .udb8x8_skip3
    cmp r8, #128
    strloh r4, [r1, #6] @ store c1
    streqh r5, [r1, #6] @ store c2
    strhih r6, [r1, #6] @ store c3
.udb8x8_skip3:
    // pixel 0, line 2
    ands r8, r7, #0x300 @ get the index of the color
    streqh r3, [r1, #8] @ store c0
    beq .udb8x8_skip4
    cmp r8, #512
    strloh r4, [r1, #8] @ store c1
    streqh r5, [r1, #8] @ store c2
    strhih r6, [r1, #8] @ store c3
.udb8x8_skip4:
    // pixel 1
    ands r8, r7, #0xC00 @ get the index of the color
    streqh r3, [r1, #10] @ store c0
    beq .udb8x8_skip5
    cmp r8, #2048
    strloh r4, [r1, #10] @ store c1
    streqh r5, [r1, #10] @ store c2
    strhih r6, [r1, #10] @ store c3
.udb8x8_skip5:
    // pixel 2
    ands r8, r7, #0x3000 @ get the index of the color
    streqh r3, [r1, #12] @ store c0
    beq .udb8x8_skip6
    cmp r8, #8192
    strloh r4, [r1, #12] @ store c1
    streqh r5, [r1, #12] @ store c2
    strhih r6, [r1, #12] @ store c3
.udb8x8_skip6:
    // pixel 3
    ands r8, r7, #0xC000 @ get the index of the color
    streqh r3, [r1, #14] @ store c0
    beq .udb8x8_skip7
    cmp r8, #32768
    strloh r4, [r1, #14] @ store c1
    streqh r5, [r1, #14] @ store c2
    strhih r6, [r1, #14] @ store c3
.udb8x8_skip7:
    subs r9, r9, #1 @ decrement line counter
    bne .udb8x8_line @ repeat for the next line
.udb8x8_end:
    ldmfd sp!, {r3 - r9}
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
