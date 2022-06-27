@ === void memcpy32(void *dst, const void *src, u32 wcount) IWRAM_CODE; =============
@ r0, r1: dst, src
@ r2: wcount, then wcount>>3
@ r3-r10: data buffer
@ r12: wcount & 7
.section .iwram,"ax", %progbits
.arm
.cpu arm7tdmi
.align  2
.global memcpy32
.type memcpy32 STT_FUNC;	
memcpy32:
	and		r12, r2, #7
	movs	r2, r2, lsr #3
	beq		.Lres_cpy32
	push	{r4-r10}
	@ copy 32byte chunks with 8fold xxmia
.Lmain_cpy32:
		ldmia	r1!, {r3-r10}	
		stmia	r0!, {r3-r10}
		subs	r2, r2, #1
		bhi		.Lmain_cpy32
	pop		{r4-r10}
	@ and the residual 0-7 words
.Lres_cpy32:
		subs	r12, r12, #1
		ldmcsia	r1!, {r3}
		stmcsia	r0!, {r3}
		bhi		.Lres_cpy32
	bx	lr
.size	memcpy32, .-memcpy32