 #include "lz4_constants.h"

#define LZ4_OVERRUN_PROTECTION // If turned off can overrun the buffer max. 3 bytes. Very minor performance impact.

 .arm
 .align
 .global LZ4_MemCopy
 .type LZ4_MemCopy,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
LZ4_MemCopy:
    @ Copy data from r0 to r1, possibly in 4, 2 or 1 byte chunks
    @ ------------------------------
    @ Input:
    @ r0: source pointer (will point to r0 + r4)
    @ r1: destination pointer (will point to r1 + r4)
    @ r4: number of bytes to copy (trashed)
    @ ------------------------------
    @ In function:
    @ r5-r6 trashed
    
    @ if we're copying less than 8 bytes, just do a byte copy
    cmp r4, #8
    blo .lz4_mc_byte_loop
    @ check if both pointers have the same alignment offset for word copies
    and r5, r0, #3
    and r6, r1, #3
    cmp r5, r6
    beq .lz4_mc_word_aligned @ if offset is the same, do a word-aligned copy
    @ no word alignment possible. can we at least get halfword alignment?
    and r5, r0, #1
    and r6, r1, #1
    cmp r5, r6
    beq .lz4_mc_halfword_aligned @ if offset is the same, do a halfword-aligned copy
.lz4_mc_byte_loop:
    ldrb r5, [r0], #1
    strb r5, [r1], #1
    subs r4, r4, #1
    bne .lz4_mc_byte_loop
    bx lr
.lz4_mc_halfword_aligned:
    @ check for an overlapping copy with a distance smaller than 2
    subs r5, r1, r0 @ r5 = r1 - r0
    rsbmi r5, r5, #0 @ r5 = (r1 - r0) < 0 ? 0-r5 : r5
    cmp r5, #2
    blt .lz4_mc_byte_loop @ if |r1 -r0| < 2, do byte copy
    @ check how many bytes we must pre-copy for halfword alignment
    @ offset == 1: pre-copy 1 byte
    cmp r6, #1
    subeq r4, r4, #1 @ offset from halfword == 1, copy length -= 1
    ldreqb r5, [r0], #1 @ offset from halfword == 1, do byte pre-copy
    streqb r5, [r1], #1
.lz4_mc_halfword_loop: @ here we have halfword alignment
    ldrh r5, [r0], #2
    strh r5, [r1], #2
#ifdef LZ4_OVERRUN_PROTECTION
    sub r4, r4, #2
    cmp r4, #2
    bhs .lz4_mc_halfword_loop
    tst r4, #1 @ check for byte tail
    ldrneb r5, [r0], #1 @ bytes left > 0, do byte post-copy
    strneb r5, [r1], #1
#else
    subs r4, #2
    bgt .lz4_mc_halfword_loop
    adds r0, r4
    adds r1, r4
#endif
    bx lr
.lz4_mc_word_aligned:
    @ check for an overlapping copy with a distance smaller than 4
    subs r5, r1, r0 @ r5 = r1 - r0
    rsbmi r5, r5, #0 @ r5 = (r1 - r0) < 0 ? 0-r5 : r5
    cmp r5, #4
    blt .lz4_mc_byte_loop @ if |r1 -r0| < 4, do byte copy
    @ check how many bytes we must pre-copy for word alignment
    @ offset == 1: pre-copy 1 byte + 1 halfword
    @ offset == 2: pre-copy 1 halfword
    @ offset == 3: pre-copy 1 byte
    cmp r6, #0
    beq .lz4_mc_word_loop @ offset from word == 0, do word copy
    cmp r6, #2
    ldrneb r5, [r0], #1 @ offset from word == 1 or 3, do byte pre-copy
    strneb r5, [r1], #1
    subne r4, r4, #1 @ offset from word == 1 or 3, copy length -= 1
    ldrlsh r5, [r0], #2 @ offset from word <= 2, do halfword pre-copy
    strlsh r5, [r1], #2
    subls r4, r4, #2 @ offset from word <= 2, copy length -= 2
.lz4_mc_word_loop:
    ldr r5, [r0], #4
    str r5, [r1], #4
#ifdef LZ4_OVERRUN_PROTECTION
    sub r4, r4, #4
    cmp r4, #4
    bhs .lz4_mc_word_loop
.lz4_mc_word_tail:
    tst r4, #2 @ check for halfword tail
    ldrneh r5, [r0], #2 @ bytes left >= 2, do halfword tail copy
    strneh r5, [r1], #2
    tst r4, #1 @ check for byte tail
    ldrneb r5, [r0], #1 @ bytes left > 0, do byte tail copy
    strneb r5, [r1], #1
#else
    subs r4, #4
    bgt .lz4_mc_word_loop
    adds r0, r4
    adds r1, r4
#endif
    bx lr


 .arm
 .align
 .global LZ4_UnCompWrite8bit
 .type LZ4_UnCompWrite8bit,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
LZ4_UnCompWrite8bit:
    @ Decode LZ4 data
    @ ------------------------------
    @ Input:
    @ r0: pointer to LZ4 data, must be 4-byte-aligned (trashed)
    @ r1: pointer to destination buffer, must be 4-byte-aligned (trashed)
    @ ------------------------------
    @ In function:
    @ r2: size of uncompressed data (trashed)
    @ r3 trashed, r4-r6 used and saved / restored
    push {r4 - r6, lr}

    @ read header word:
    @ Bit 0-7: Compressed type (40h for LZ4)
    @ Bit 8-31: Size of uncompressed data
    ldrb r2, [r0, #0]
    @ stop if this isn't LZ4
    cmp r2, #0x40
    bne .lz4_ucw8_end

    @ r2 = uncompressed size
    ldrb r2, [r0, #1]
    ldrb r3, [r0, #2]
    lsl r3, r3, #8
    orr r2, r2, r3
    ldrb r3, [r0, #3]
    lsl r3, r3, #16
    orrs r2, r2, r3
    @ stop if uncompressed size is 0
    beq .lz4_ucw8_end

    @ move input pointer past header
    add r0, r0, #4
.lz4_ucw8_decode_loop:
    @ r3 = token
    ldrb r3, [r0], #1
    @ ----- literal decoding -----
    @ r4 = literal length
    mov r4, r3, lsr #LZ4_CONSTANTS_LITERAL_LENGTH_SHIFT
    ands r4, r4, #LZ4_CONSTANTS_LENGTH_MASK
    beq .lz4_ucw8_literals_end
    @ read extra literal length if initial length == 15
    cmp r4, #15
    blo .lz4_ucw8_copy_literals
.lz4_ucw8_read_literals_length:
    ldrb r5, [r0], #1
    add r4, r4, r5
    cmp r5, #255
    beq .lz4_ucw8_read_literals_length
.lz4_ucw8_copy_literals:
    sub r2, r2, r4 @ uncompressed size -= length of literals
    bl LZ4_MemCopy
.lz4_ucw8_literals_end:
    @ ----- match decoding -----
    @ r4 = match length
    ands r4, r3, #LZ4_CONSTANTS_LENGTH_MASK
    beq .lz4_ucw8_match_end
    @ r6 = 16-bit match offset
    ldrb r6, [r0], #1
    ldrb r5, [r0], #1
    orr r6, r5, r6, lsl #8
    @ read extra match length if initial length == 15
    cmp r4, #15
    blo .lz4_ucw8_copy_match
.lz4_ucw8_read_match_length:
    ldrb r5, [r0], #1
    add r4, r4, r5
    cmp r5, #255
    beq .lz4_ucw8_read_match_length
.lz4_ucw8_copy_match:
    add r4, r4, #LZ4_CONSTANTS_MIN_MATCH_LENGTH - 1
    sub r2, r2, r4 @ uncompressed size -= length of match
    mov r3, r0 @ save r0
    sub r0, r1, r6 @ src pointer = dst pointer - match offset
    bl LZ4_MemCopy
    mov r0, r3 @ restore r0
.lz4_ucw8_match_end:
    cmp r2, #0 @ still data left to decompress?
    bne .lz4_ucw8_decode_loop
.lz4_ucw8_end:
    pop {r4 - r6, lr}
    bx lr

.arm
 .align
 .global LZ4_UnCompGetSize
 .type LZ4_UnCompGetSize,function
#ifdef __NDS__
 .section .itcm, "ax", %progbits
#else
 .section .iwram, "ax", %progbits
#endif
LZ4_UnCompGetSize:
    @ Decode LZ4 data
    @ r0: pointer to LZ4 data, must be 4-byte-aligned (trashed)
    @ r2,r3 are scratch registers (trashed)

    @ read header word:
    @ Bit 0-7: Compressed type (40h for LZ4)
    @ Bit 8-31: Size of uncompressed data
    ldrb r2, [r0, #0]
    @ stop if this isn't LZ4 and return 0
    cmp r2, #0x40
    movne r0, #0
    bne .lz4_ugs_end
    @ read uncompressed size
    ldrb r2, [r0, #1]
    ldrb r3, [r0, #2]
    lsl r3, r3, #8
    orr r2, r2, r3
    ldrb r3, [r0, #3]
    add r0, r0, #4
    lsl r3, r3, #16
    orrs r2, r2, r3
    mov r0, r2
.lz4_ugs_end:
    bx lr
