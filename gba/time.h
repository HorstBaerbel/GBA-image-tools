#pragma once

#include <cstdint>

namespace Time
{

	/// @brief Enable timer. now() value increases every ~1ms now.
	void start();

	/// @brief Stop timer. now() value will not increase anymore.
	void stop();

	/// @brief Return time since timer was started.
	/// @return Returns the current time in seconds in 16:16 format.
	/// @note This will wrap around ~4,5 hours after start() was called.
	int32_t now();

} // namespace Time
