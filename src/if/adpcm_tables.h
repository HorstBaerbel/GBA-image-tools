#pragma once

#include <cstdint>

/// @brief ADPCM step table
extern "C" const uint16_t StepTable[89];

/// @brief ADPCM step index table for 4-bit indices
extern "C" const int16_t IndexTable_4bit[8];
