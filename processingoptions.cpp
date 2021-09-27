#include "processingoptions.h"

#include "exception.h"

ProcessingOptions::Option::operator bool() const
{
    return isSet;
}

std::string ProcessingOptions::Option::helpString() const
{
    return cxxOption.opts_ + ": " + cxxOption.desc_;
}

ProcessingOptions::OptionT<float> ProcessingOptions::binary{
    false,
    {"binary", "Convert images to binary image with intensity threshold at N. N must be in [0.0, 1.0].", cxxopts::value(binary.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(binary.cxxOption.opts_))
        {
            REQUIRE(binary.value >= 0.0 && binary.value <= 1.0, std::runtime_error, "Binarization threshold value must be in [0.0, 1.0]");
            binary.isSet = true;
        }
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::paletted{
    false,
    {"paletted", "Convert images to paletted image with N colors using dithering. N must be in [2, 256].", cxxopts::value(paletted.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(paletted.cxxOption.opts_))
        {
            REQUIRE(paletted.value >= 1 && paletted.value <= 256, std::runtime_error, "Number of palette colors must be in [2, 256]");
            paletted.isSet = true;
        }
    }};

ProcessingOptions::OptionT<std::string> ProcessingOptions::truecolor{
    false,
    {"truecolor", "Convert images to RGB888, RGB565 or RGB555 true-color", cxxopts::value(truecolor.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(truecolor.cxxOption.opts_))
        {
            REQUIRE(truecolor.value == "RGB888" || truecolor.value == "RGB565" || truecolor.value == "RGB555", std::runtime_error, "Format must be RGB888, RGB565 or RGB555");
            truecolor.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::reorderColors{
    false,
    {"reordercolors", "Reorder palette colors to minimize preceived color distance.", cxxopts::value(reorderColors.isSet)}};

ProcessingOptions::OptionT<Magick::Color> ProcessingOptions::addColor0{
    false,
    {"addcolor0", "Add COLOR at palette index #0 and increase all other color indices by 1. Only usable for paletted images. Color format \"abcd012\".", cxxopts::value(addColor0.valueString)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(addColor0.cxxOption.opts_))
        {
            try
            {
                addColor0.value = Magick::Color(std::string("#") + addColor0.valueString);
            }
            catch (const Magick::Exception &e)
            {
                THROW(std::runtime_error, addColor0.valueString << " is not a valid color. Format must be e.g. \"--addcolor0=abc012\"");
            }
            addColor0.isSet = true;
        }
    }};

ProcessingOptions::OptionT<Magick::Color> ProcessingOptions::moveColor0{
    false,
    {"movecolor0", "Move COLOR to palette index #0 and move all other colors accordingly. Only usable for paletted images. Color format \"abcd012\".", cxxopts::value(moveColor0.valueString)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(moveColor0.cxxOption.opts_))
        {
            try
            {
                moveColor0.value = Magick::Color(std::string("#") + moveColor0.valueString);
            }
            catch (const Magick::Exception &e)
            {
                THROW(std::runtime_error, moveColor0.valueString << " is not a valid color. Format must be e.g. \"--movecolor0=abc012\"");
            }
            moveColor0.isSet = true;
        }
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::shiftIndices{
    false,
    {"shift", "Increase image index values by N, keeping index #0 at 0. N must be in [1, 255] and resulting indices will be clamped to [0, 255]. Only usable for paletted images.", cxxopts::value(shiftIndices.value)},
    0,
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(shiftIndices.cxxOption.opts_))
        {
            REQUIRE(shiftIndices.value >= 1 && shiftIndices.value <= 255, std::runtime_error, "Shift value must be in [1, 255]");
            shiftIndices.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::pruneIndices{
    false,
    {"prune", "Reduce bit depth of palette indices to 4 bit.", cxxopts::value(pruneIndices.isSet)}};

ProcessingOptions::OptionT<std::vector<uint32_t>> ProcessingOptions::sprites{
    false,
    {"sprites", "Cut data into sprites of size W x H and store data sprite- and 8x8-tile-wise. The image needs to be paletted and its width and height must be a multiple of W and H and also a multiple of 8 pixels. Sprite data is stored in \"1D mapping\" order and can be read with memcpy.", cxxopts::value(sprites.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(sprites.cxxOption.opts_))
        {
            REQUIRE(sprites.value.size() == 2, std::runtime_error, "Sprite size format must be \"W,H\", e.g. \"--sprites=32,16\"");
            auto width = sprites.value.at(0);
            REQUIRE(width >= 8 && width <= 64 && width % 8 == 0, std::runtime_error, "Sprite width must be in [8,64] and a multiple of 8");
            auto height = sprites.value.at(1);
            REQUIRE(height >= 8 && height <= 64 && height % 8 == 0, std::runtime_error, "Sprite height must be in [8,64] and a multiple of 8");
            sprites.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::tiles{
    false,
    {"tiles", "Cut data into 8x8 tiles and store data tile-wise. The image needs to be paletted and its width and height must be a multiple of 8 pixels.", cxxopts::value(tiles.isSet)}};

ProcessingOptions::Option ProcessingOptions::deltaImage{
    false,
    {"deltaimage", "Delta encoding between succesive images.", cxxopts::value(deltaImage.isSet)}};

ProcessingOptions::Option ProcessingOptions::delta8{
    false,
    {"delta8", "8-bit delta encoding.", cxxopts::value(delta8.isSet)}};

ProcessingOptions::Option ProcessingOptions::delta16{
    false,
    {"delta16", "16-bit delta encoding.", cxxopts::value(delta16.isSet)}};

ProcessingOptions::Option ProcessingOptions::lz10{
    false,
    {"lz10", "Use LZ compression variant 10.", cxxopts::value(lz10.isSet)}};

ProcessingOptions::Option ProcessingOptions::lz11{
    false,
    {"lz11", "Use LZ compression variant 11.", cxxopts::value(lz11.isSet)}};

ProcessingOptions::Option ProcessingOptions::rle{
    false,
    {"rle", "Use RLE compression.", cxxopts::value(rle.isSet)}};

ProcessingOptions::Option ProcessingOptions::vram{
    false,
    {"vram", "Make compression VRAM-safe.", cxxopts::value(vram.isSet)}};

ProcessingOptions::Option ProcessingOptions::dxt1{
    false,
    {"dxt1", "Use DXT1 RGB565 compression.", cxxopts::value(dxt1.isSet)}};

ProcessingOptions::Option ProcessingOptions::interleavePixels{
    false,
    {"interleavepixels", "Interleave pixels from different images into one array.", cxxopts::value(interleavePixels.isSet)}};

ProcessingOptions::Option ProcessingOptions::dryRun{
    false,
    {"dryrun", "Test processing, but do not write output files.", cxxopts::value(dryRun.isSet)}};
