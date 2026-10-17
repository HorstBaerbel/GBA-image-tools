#include "testmacros.h"

#include "image/quantizationmethod.h"

using Image::Quantization::Method;

TEST_SUITE("Quantization Method")

TEST_CASE("toString")
{
    CATCH_REQUIRE(Image::Quantization::toString(Method::ClosestColor) == "Closest color");
    CATCH_REQUIRE(Image::Quantization::toString(Method::AtkinsonDither) == "Atkinson dither");
    CATCH_REQUIRE_THROWS(Image::Quantization::toString(Method::None));
}
