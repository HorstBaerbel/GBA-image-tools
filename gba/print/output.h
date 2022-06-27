#pragma once

#include <cstdarg>

namespace Debug
{

    /// @brief Output string to emulator or console
    void print(const char *s);

    /// @brief Print a formatted string
    void printf(const char *fmt...);

}
