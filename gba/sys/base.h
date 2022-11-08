#pragma once

#ifdef BIT
#undef BIT
#endif

/// @brief Base address of GBA video ram
#define VRAM 0x06000000

/// @brief Base address of GBA internal work RAM
#define IWRAM 0x03000000

/// @brief Base address of GBA external work RAM
#define EWRAM 0x02000000
/// @brief End address of GBA external work RAM
#define EWRAM_END 0x02040000

/// @brief Base address of GBA cart save ram
#define SRAM 0x0E000000

/// @brief Base address of GBA hardware registers
#define REG_BASE 0x04000000

/// @brief Set nth bit in number
#define BIT(number) (1 << (number))

// undefine all sections so we can have code and data in one segment in a compile unit
#ifdef IWRAM_CODE
#undef IWRAM_CODE
#endif
#ifdef EWRAM_CODE
#undef EWRAM_CODE
#endif
#ifdef IWRAM_DATA
#undef IWRAM_DATA
#endif
#ifdef EWRAM_DATA
#undef EWRAM_DATA
#endif
#ifdef EWRAM_BSS
#undef EWRAM_BSS
#endif

#ifdef ALIGN
#undef ALIGN
#endif

#define STRINGIFY_2(a) #a
#define STRINGIFY(a) STRINGIFY_2(a)
#define SECTION_COUNTER STRINGIFY(__COUNTER__)

#define ALIGN(n) __attribute__((aligned(n)))
#define ALIGN_PACK(n) __attribute__((aligned(n), packed))
#define NORETURN __attribute__((__noreturn__))
#define NOINLINE __attribute__((noinline))
#define FORCEINLINE inline __attribute__((always_inline))
#define ARM_CODE __attribute__((target("arm")))
#define THUMB_CODE __attribute__((target("thumb")))

#define SECTION(name) __attribute__((section(name)))

#define IWRAM_FUNC __attribute__((section(".iwram.text." SECTION_COUNTER))) NOINLINE
#define IWRAM_DATA __attribute__((section(".iwram.data." SECTION_COUNTER)))
#define IWRAM_BSS __attribute__((section(".bss." SECTION_COUNTER)))
#define EWRAM_FUNC __attribute__((section(".ewram.text." SECTION_COUNTER))) NOINLINE
#define EWRAM_DATA __attribute__((section(".ewram.data." SECTION_COUNTER)))
#define EWRAM_BSS __attribute__((section(".sbss." SECTION_COUNTER)))

/// @brief Use for default 0 initialized static variables
#define STATIC_BSS static EWRAM_BSS
/// @brief Use for initialized static variables
#define STATIC_DATA static EWRAM_DATA
