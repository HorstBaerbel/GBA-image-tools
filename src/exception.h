#pragma once

#include <iosfwd>
#include <stdexcept>
#include <sstream>

/// @brief Throw an exception with variable arguments
#define THROW(EXCEPTION_T, ...)      \
    do                               \
    {                                \
        std::stringstream ss;        \
        ss << __VA_ARGS__;           \
        throw EXCEPTION_T(ss.str()); \
    } while (0)

/// @brief Require COND to be true, else throw an exception with variable arguments
#define REQUIRE(COND, EXCEPTION_T, ...) \
    if (!(COND))                        \
    {                                   \
        std::stringstream ss;           \
        ss << __VA_ARGS__;              \
        throw EXCEPTION_T(ss.str());    \
    }                                   \
    else                                \
        do                              \
        {                               \
        } while (0)
