@ === void memset32(void *dst, u32 fill, size_t wcount) IWRAM_CODE; =============
.section .iwram, "ax",%progbits
.arm
.cpu arm7tdmi
.align
.global memset32
memset32:
    mov     r2, r2, lsl #1
    cmp     r2, #16
    bhs     .Lms16_entry32
    b       .Lms16_word_loop

@ === void memset16(void *dst, u16 fill, size_t hwcount) IWRAM_CODE; =============
.section .iwram, "ax", %progbits
.arm
.cpu arm7tdmi
.align
.global memset16
memset16:
    cmp     r2, #0              @ if(count != 0)
    movnes  r3, r0, ror #2      @   if(dst && (dst&1))
    strmih  r1, [r0], #2        @   {   *dst++= fill;   count--;    }
    submis  r2, r2, #1          @ if(count == 0 || dst == NULL)
    bxeq    lr                  @   return;

    orr     r1, r1, lsl #16     @ Prep for word fills.
    cmp     r2, #16
    blo     .Lms16_word_loop
.Lms16_entry32:

    @ --- Block run ---
    stmfd   sp!, {r4-r8}    
    mov     r3, r1
    mov     r4, r1
    mov     r5, r1
    mov     r6, r1
    mov     r7, r1
    mov     r8, r1
    mov     r12, r1
.Lms16_block_loop:
        subs    r2, r2, #16
        stmhsia r0!, {r1, r3-r8, r12}
        bhi     .Lms16_block_loop
    ldmfd   sp!, {r4-r8}
    bxeq    lr
    addne   r2, r2, #16         @ Correct for overstep in loop

    @ --- Word run (+ trailing halfword) ---
.Lms16_word_loop:
        subs    r2, r2, #2
        strhs   r1, [r0], #4
        bhi     .Lms16_word_loop
    strneh  r1, [r0], #2        @ r2 != 0 means spare hword left
    bx  lr