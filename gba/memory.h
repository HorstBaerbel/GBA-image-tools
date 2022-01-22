#pragma once

#include <cstdint>
#include <gba_base.h>

/// @brief Use for default 0 initialized static variables
#define STATIC_BSS static EWRAM_BSS
/// @brief Use for initialized static variables
#define STATIC_DATA static EWRAM_DATA

namespace Memory
{

	/// @brief Register for Game Pak SRAM and ROM wait states
	inline auto &RegWaitCnt{*reinterpret_cast<volatile uint16_t *>(REG_BASE + 0x0204)};

	/// @brief Minimum wait states possible for Game Pak SRAM and ROM
	/// See: http://problemkaputt.de/gbatek.htm#gbasystemcontrol
	constexpr uint16_t WaitCntFast = 0x46DA;

	/// @brief Regular wait states possible for Game Pak SRAM and ROM
	/// See: http://problemkaputt.de/gbatek.htm#gbasystemcontrol
	constexpr uint16_t WaitCntNormal = 0x4317;

	/// @brief Register for EWRAM wait states
	inline auto &RegWaitEwram{*reinterpret_cast<volatile uint32_t *>(REG_BASE + 0x0800)};

	/// @brief Wait states for EWRAM that crash the GBA (1/1/2)
	/// See: http://problemkaputt.de/gbatek.htm#gbasystemcontrol
	constexpr uint32_t WaitEwramLudicrous = 0x0F000020;

	/// @brief Minimum wait states possible for EWRAM (2/2/4)
	/// See: http://problemkaputt.de/gbatek.htm#gbasystemcontrol
	constexpr uint32_t WaitEwramFast = 0x0E000020;

	/// @brief Regular wait states possible for EWRAM (3/3/6)
	/// See: http://problemkaputt.de/gbatek.htm#gbasystemcontrol
	constexpr uint32_t WaitEwramNormal = 0x0D000020;

	// PLEASE NOTE: We're on ARM, so word means 32-bits, half-word means 16-bit!

	/// @brief Copy dwords from source to destination.
	/// @param destination Copy destination.
	/// @param source Copy source.
	/// @param nrOfWords Number of dwords to copy.
	extern "C" void memcpy32(void *destination, const void *source, uint32_t nrOfWords) IWRAM_CODE;

	/// @brief Set words in destination to value.
	/// @param destination Set destination.
	/// @param value Value to set destination half-words to.
	/// @param nrOfHwords Number of half-words to set.
	extern "C" void memset16(void *destination, uint16_t value, uint32_t nrOfHwords) IWRAM_CODE;

	/// @brief Set dwords in destination to value.
	/// @param destination Set destination.
	/// @param value Value to set destination dwords to.
	/// @param nrOfWords Number of dwords to set.
	extern "C" void memset32(void *destination, uint32_t value, uint32_t nrOfWords) IWRAM_CODE;

} // namespace Memory
