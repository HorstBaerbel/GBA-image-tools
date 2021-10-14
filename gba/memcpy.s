@ === void memcpy32(void *dst, const void *src, u32 wcount) IWRAM_CODE; =============
@ r0, r1: dst, src
@ r2: wcount, then wcount>>3
@ r3-r10: data buffer
@ r12: wcount & 7
.section .iwram,"ax", %progbits
.arm
.cpu arm7tdmi
.align  2
.code   32
.global memcpy32
memcpy32:
    and     r12, r2, #7     @ r12= residual word count
    movs    r2, r2, lsr #3  @ r2=block count
    beq     .Lres_cpy32
    push    {r4-r10}
    @ Copy 32byte chunks with 8fold xxmia
    @ r2 in [1,inf>
.Lmain_cpy32:
        ldmia   r1!, {r3-r10}   
        stmia   r0!, {r3-r10}
        subs    r2, #1
        bne     .Lmain_cpy32
    pop     {r4-r10}
    @ And the residual 0-7 words. r12 in [0,7]
.Lres_cpy32:
        subs    r12, #1
        ldrcs   r3, [r1], #4
        strcs   r3, [r0], #4
        bcs     .Lres_cpy32
    bx  lr
