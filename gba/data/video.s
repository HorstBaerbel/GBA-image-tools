.section .rodata
    .global VIDEO_DATA
    .type VIDEO_DATA, %object
    .align 2
VIDEO_DATA:
    .incbin "../data/video.bin"
VIDEO_DATA_END:
    .global VIDEO_DATA_SIZE
    .type VIDEO_DATA_SIZE, %object
    .align 2
VIDEO_DATA_SIZE:
    .int VIDEO_DATA_END - VIDEO_DATA
