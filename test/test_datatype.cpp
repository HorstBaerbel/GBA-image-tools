#include "testmacros.h"

#include "image/datatype.h"

using Image::DataType;

TEST_SUITE("DataType")

TEST_CASE("DefaultConstruction")
{
    DataType dt;
    CATCH_REQUIRE_FALSE(dt.isBitmap());
    CATCH_REQUIRE_FALSE(dt.isSprites());
    CATCH_REQUIRE_FALSE(dt.isTiles());
    CATCH_REQUIRE_FALSE(dt.isCompressed());
}

TEST_CASE("SettersAndGetters")
{
    DataType dt;
    dt.setBitmap();
    CATCH_REQUIRE(dt.isBitmap());
    dt.setBitmap(false);
    CATCH_REQUIRE_FALSE(dt.isBitmap());
    dt.setSprites();
    CATCH_REQUIRE(dt.isSprites());
    dt.setSprites(false);
    CATCH_REQUIRE_FALSE(dt.isSprites());
    dt.setTiles();
    CATCH_REQUIRE(dt.isTiles());
    dt.setTiles(false);
    CATCH_REQUIRE_FALSE(dt.isTiles());
    dt.setCompressed();
    CATCH_REQUIRE(dt.isCompressed());
    dt.setCompressed(false);
    CATCH_REQUIRE_FALSE(dt.isCompressed());
}

TEST_CASE("FlagsAreIndependent")
{
    DataType dt;
    dt.setBitmap();
    dt.setSprites();
    dt.setTiles();
    dt.setCompressed();
    // all flags set at once
    CATCH_REQUIRE(dt.isBitmap());
    CATCH_REQUIRE(dt.isSprites());
    CATCH_REQUIRE(dt.isTiles());
    CATCH_REQUIRE(dt.isCompressed());
    // clearing one does not affect the others
    dt.setBitmap(false);
    CATCH_REQUIRE_FALSE(dt.isBitmap());
    CATCH_REQUIRE(dt.isSprites());
    CATCH_REQUIRE(dt.isTiles());
    CATCH_REQUIRE(dt.isCompressed());
}

TEST_CASE("ConstructionFromFlags")
{
    DataType dt = DataType(DataType::Flags::Bitmap);
    CATCH_REQUIRE(dt.isBitmap());
    CATCH_REQUIRE_FALSE(dt.isSprites());
    CATCH_REQUIRE_FALSE(dt.isTiles());
    CATCH_REQUIRE_FALSE(dt.isCompressed());
}
