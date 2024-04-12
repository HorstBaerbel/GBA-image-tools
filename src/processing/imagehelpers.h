#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// @brief Convert data to 1-bit-sized values and put 8*1 bits into 1 byte
/// @param data The first bytes will be converted to the lowest bits, thus {0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01}
/// will be converted to {0xDD}. Thus the bitstream will arrive left-to-right.
/// @note Data size must be divisible by 8 and all indices must be < 2.
std::vector<uint8_t> convertDataTo1Bit(const std::vector<uint8_t> &data);

/// @brief Convert data to 2-bit-sized values and put 4*2 bits into one byte
/// @param data The first bytes will be converted to the lowest bits, thus {0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, 0x01}
/// will be converted to {0x61, 0x43}. Thus the bitstream will arrive left-to-right.
/// @note Data size must be divisible by 4 and all indices must be < 4.
std::vector<uint8_t> convertDataTo2Bit(const std::vector<uint8_t> &data);

/// @brief Convert data to nibble-sized values and put 2*4 bits into one byte
/// @param data The first bytes will be converted to the lowest bits, thus {0x01, 0x07, 0x0A, 0x03}
/// will be converted to {0x71, 0x3A}. Thus the bitstream will arrive left-to-right.
/// @note Data size must be divisible by 2 and all indices must be < 16.
std::vector<uint8_t> convertDataTo4Bit(const std::vector<uint8_t> &data);

/// @brief Increase all values by 1
/// @note All values must be < 255.
std::vector<uint8_t> incValuesBy1(const std::vector<uint8_t> &data);

/// @brief Swap value in data with 0
/// {0x02, 0x00, 0x03, 0x02, 0x01, 0x00}, value = 2 -> {0x02, 0x03, 0x00, 0x02, 0x01, 0x03}
std::vector<uint8_t> swapValueWith0(const std::vector<uint8_t> &data, uint8_t value);

/// @brief Swap values in data according to a table with new values
/// @param data All values found in data must exist in newValues
/// @param newValues Mapping of old value -> new value
/// MIGHT BE BROKEN!!!
std::vector<uint8_t> swapValues(const std::vector<uint8_t> &data, const std::vector<uint8_t> &newValues);
