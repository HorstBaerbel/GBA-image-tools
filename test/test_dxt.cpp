#include <catch2/catch_test_macros.hpp>

#include "codec_dxt.h"

static int Factorial(int number)
{
    return number <= 1 ? number : Factorial(number - 1) * number; // fail
    // return number <= 1 ? 1      : Factorial( number - 1 ) * number;  // pass
}

TEST_CASE("Factorial of 0 is 1 (fail)", "")
{
    REQUIRE(Factorial(0) == 1);
}
