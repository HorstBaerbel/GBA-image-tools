#include "dma.h"

namespace DMA
{

    struct DMA_REC
    {
        const void *source;
        void *destination;
        uint16_t count;
        uint16_t mode;
    } __attribute__((aligned(4), packed));

#define REG_DMA ((volatile DMA_REC *)0x040000B0)
#define DMA_DST_INC (0 << 5)
#define DMA_DST_DEC (1 << 5)
#define DMA_DST_FIXED (2 << 5)
#define DMA_DST_RELOAD (3 << 5)
#define DMA_SRC_INC (0 << 7)
#define DMA_SRC_DEC (1 << 7)
#define DMA_SRC_FIXED (2 << 7)
#define DMA_REPEAT (1 << 9)
#define DMA16 (0 << 10)
#define DMA32 (1 << 10)
#define GAMEPAK_DRQ (1 << 11)
#define DMA_IMMEDIATE (0 << 12)
#define DMA_VBLANK (1 << 12)
#define DMA_HBLANK (2 << 12)
#define DMA_SPECIAL (3 << 12)
#define DMA_IRQ (1 << 14)
#define DMA_ENABLE (1 << 15)

    static uint32_t DMAFillTempValue;

    /*//! General DMA transfer macro
#define DMA_TRANSFER(_destination, _source, count, channel, mode) \
    do                                                            \
    {                                                             \
        REG_DMA[channel].mode = 0;                               \
        REG_DMA[channel].source = (const void *)(_source);        \
        REG_DMA[channel].destination = (void *)(_destination);    \
        REG_DMA[channel].count = (count);                         \
        REG_DMA[channel].mode = (mode);                           \
    } while (0)
    */

    void dma_fill32(void *destination, uint32_t value, uint16_t nrOfWords)
    {
        // wait for previous transfer to finish
        while (REG_DMA[3].mode & DMA_ENABLE)
        {
        }
        DMAFillTempValue = value;
        REG_DMA[3].source = reinterpret_cast<const void *>(&DMAFillTempValue);
        REG_DMA[3].destination = destination;
        REG_DMA[3].count = nrOfWords;
        REG_DMA[3].mode = DMA32 | DMA_DST_INC | DMA_SRC_FIXED | DMA_ENABLE;
        // wait for transfer to finish
        while (REG_DMA[3].mode & DMA_ENABLE)
        {
        }
    }

    void dma_copy32(void *destination, const uint32_t *source, uint16_t nrOfWords)
    {
        // wait for previous transfer to finish
        while (REG_DMA[3].mode & DMA_ENABLE)
        {
        }
        REG_DMA[3].source = reinterpret_cast<const void *>(source);
        REG_DMA[3].destination = destination;
        REG_DMA[3].count = nrOfWords;
        REG_DMA[3].mode = DMA32 | DMA_DST_INC | DMA_SRC_INC | DMA_ENABLE;
        // wait for transfer to finish
        while (REG_DMA[3].mode & DMA_ENABLE)
        {
        }
    }

} // namespace DMA