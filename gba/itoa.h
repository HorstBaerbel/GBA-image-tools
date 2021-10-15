#pragma once

#include <gba_base.h>
#include <cstdint>

char *itoa(uint32_t value, char *result, uint32_t base = 10) IWRAM_CODE;
char *itoa(int32_t value, char *result, uint32_t base = 10) IWRAM_CODE;
char *fptoa(int32_t value, char *result, uint32_t BITSF, uint32_t precision = 0) IWRAM_CODE;
