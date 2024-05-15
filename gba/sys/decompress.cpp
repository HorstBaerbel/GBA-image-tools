#include "decompress.h"
#include "syscall.h"

namespace BIOS
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    void LZ77UnCompReadNormalWrite8bit(const void *source, void *dest)
    {
        SYSCALL(0x11);
    }

    void LZ77UnCompReadNormalWrite16bit(const void *source, void *dest)
    {
        SYSCALL(0x12);
    }

    void HuffUnCompReadNormal(const void *source, void *dest)
    {
        SYSCALL(0x13);
    }

    void RLUnCompReadNormalWrite8bit(const void *source, void *dest)
    {
        SYSCALL(0x14);
    }

    void RLUnCompReadNormalWrite16bit(const void *source, void *dest)
    {
        SYSCALL(0x15);
    }

    void Diff8bitUnFilterWrite8bit(const void *source, void *dest)
    {
        SYSCALL(0x16);
    }

    void Diff8bitUnFilterWrite16bit(const void *source, void *dest)
    {
        SYSCALL(0x17);
    }

    void Diff16bitUnFilter(const void *source, void *dest)
    {
        SYSCALL(0x18);
    }
#pragma GCC diagnostic pop
} // namespace Decompression
