#include "memory.h"

namespace Memory
{

    alignas(4) uint8_t IWRAM_BLOCK[IWRAM_ALLOC_SIZE] IWRAM_DATA;
    alignas(4) uint8_t EWRAM_BLOCK[EWRAM_ALLOC_SIZE] EWRAM_DATA;

}