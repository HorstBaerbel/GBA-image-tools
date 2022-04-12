#pragma once

#if defined (__thumb__)
    #define SYSCALL(Number) __asm("swi " #Number "\n" ::: "r0", "r1", "r2", "r3")
#else
    #define SYSCALL(Number) __asm("swi " #Number " << 16\n" ::: "r0", "r1", "r2", "r3")
#endif
