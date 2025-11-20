#pragma once

#include <cstdint>

/// @brief ADPCM IWRAM dither state (last, current) dither value
extern "C" uint32_t ADPCM_DitherState[2];

/// @brief ADPCM step table
extern "C" const uint16_t ADPCM_StepTable[89];

/// @brief ADPCM step index table for 4-bit indices
extern "C" const int8_t ADPCM_IndexTable_4bit[8];

/// @brief ADPCM precalculated delta table. This combines ADPCM_StepTable and the logi to calculate delta.
extern "C" const uint16_t ADPCM_DeltaTable_4bit[89][8];
