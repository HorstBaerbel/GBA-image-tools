 #include "lz4_constants.h"
 
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
    push {r4 - r6}

    @ read header word:
    @ Bit 0-3: Compressed type (0 for LZ4)
    @ Bit 4-7: Compressed type (4 for LZ4)
    @ Bit 8-31: Size of uncompressed data
    ldrb r2, [r0, #0]
    @ stop if this isn't LZ4
    cmp r2, #0x40
    bne .lz4_ucw8_end

    @ read uncompressed size into r2
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
.lz4_ucw8_literals_loop:
    ldrb r5, [r0], #1
    strb r5, [r1], #1
    subs r4, r4, #1
    bne .lz4_ucw8_literals_loop
.lz4_ucw8_literals_end:
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
    sub r3, r1, r6 @ src pointer = dst pointer - match offset
.lz4_ucw8_match_loop:
    ldrb r5, [r3], #1
    strb r5, [r1], #1
    subs r4, r4, #1
    bne .lz4_ucw8_match_loop
.lz4_ucw8_match_end:
    cmp r2, #0 @ still data left to decompress?
    bne .lz4_ucw8_decode_loop
.lz4_ucw8_end:
    pop {r4 - r6}
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
    @ r2,r3 are scratch registers

    @ read header word:
    @ Bit 0-3: Compressed type (0 for LZ4)
    @ Bit 4-7: Compressed type (4 for LZ4)
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
