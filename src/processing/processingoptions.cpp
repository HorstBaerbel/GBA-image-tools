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

ProcessingOptions::OptionT<double> ProcessingOptions::blackWhite{
    false,
    {"blackwhite", "Convert images to b/w image with intensity threshold at N. N must be in [0.0, 1.0].", cxxopts::value(blackWhite.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(blackWhite.cxxOption.opts_))
        {
            REQUIRE(blackWhite.value >= 0.0 && blackWhite.value <= 1.0, std::runtime_error, "Intensity threshold value must be in [0.0, 1.0]");
            blackWhite.isSet = true;
        }
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::paletted{
    false,
    {"paletted", "Convert images to paletted images with N colors using dithering. N must be in [2, 256].", cxxopts::value(paletted.value)},
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

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::commonPalette{
    false,
    {"commonpalette", "Convert images to a paletted images with a common palette of N colors using dithering. N must be in [2, 256].", cxxopts::value(commonPalette.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(commonPalette.cxxOption.opts_))
        {
            REQUIRE(commonPalette.value >= 1 && commonPalette.value <= 256, std::runtime_error, "Number of palette colors must be in [2, 256]");
            commonPalette.isSet = true;
        }
    }};

ProcessingOptions::OptionT<Color::Format> ProcessingOptions::truecolor{
    false,
    {"truecolor", "Convert images to RGB888, RGB565 or RGB555 true-color", cxxopts::value(truecolor.valueString)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        std::string formatUpper;
        std::transform(truecolor.valueString.cbegin(), truecolor.valueString.cend(), std::back_inserter(formatUpper),
                       [](unsigned char c)
                       { return std::toupper(c); });
        if (r.count(truecolor.cxxOption.opts_))
        {
            if (formatUpper == "RGB888")
            {
                truecolor.value = Color::Format::XRGB8888;
            }
            else if (formatUpper == "RGB565")
            {
                truecolor.value = Color::Format::RGB565;
            }
            else if (formatUpper == "RGB555")
            {
                truecolor.value = Color::Format::XRGB1555;
            }
            else
            {
                THROW(std::runtime_error, "True-color format must be RGB888, RGB565 or RGB555");
            }
            truecolor.isSet = true;
        }
    }};

ProcessingOptions::OptionT<Color::Format> ProcessingOptions::outformat{
    false,
    {"outformat", "Set output color format (direct pixel color + color map) to RGB888, RGB565 or RGB555", cxxopts::value(outformat.valueString)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        std::string formatUpper;
        std::transform(outformat.valueString.cbegin(), outformat.valueString.cend(), std::back_inserter(formatUpper),
                       [](unsigned char c)
                       { return std::toupper(c); });
        if (r.count(outformat.cxxOption.opts_))
        {
            if (formatUpper == "RGB888")
            {
                outformat.value = Color::Format::XRGB8888;
            }
            else if (formatUpper == "RGB565")
            {
                outformat.value = Color::Format::RGB565;
            }
            else if (formatUpper == "RGB555")
            {
                outformat.value = Color::Format::XRGB1555;
            }
            else
            {
                THROW(std::runtime_error, "Output format must be RGB888, RGB565 or RGB555");
            }
            outformat.isSet = true;
        }
    }};

ProcessingOptions::OptionT<Image::Quantization::Method> ProcessingOptions::quantizationmethod{
    true,
    {"quantize", "Set quantization method for color(-space) reduction. Options are closestcolor (default) or atkinsondither", cxxopts::value(quantizationmethod.valueString)},
    {Image::Quantization::Method::ClosestColor},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(quantizationmethod.cxxOption.opts_))
        {
            if (quantizationmethod.valueString == "closestcolor")
            {
                quantizationmethod.value = Image::Quantization::Method::ClosestColor;
            }
            else if (quantizationmethod.valueString == "atkinsondither")
            {
                quantizationmethod.value = Image::Quantization::Method::AtkinsonDither;
            }
            else
            {
                THROW(std::runtime_error, "Quantization method must be closestcolor (default) or atkinsondither if specified");
            }
            quantizationmethod.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::reorderColors{
    false,
    {"reordercolors", "Reorder palette colors to minimize preceived color distance.", cxxopts::value(reorderColors.isSet)}};

ProcessingOptions::OptionT<Color::XRGB8888> ProcessingOptions::addColor0{
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
                addColor0.value = Color::XRGB8888::fromHex(addColor0.valueString);
            }
            catch (const std::runtime_error &e)
            {
                THROW(std::runtime_error, addColor0.valueString << " is not a valid color. Format must be e.g. \"--addcolor0=abc012\"");
            }
            addColor0.isSet = true;
        }
    }};

ProcessingOptions::OptionT<Color::XRGB8888> ProcessingOptions::moveColor0{
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
                moveColor0.value = Color::XRGB8888::fromHex(moveColor0.valueString);
            }
            catch (const std::runtime_error &e)
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

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::pruneIndices{
    false,
    {"prune", "Reduce bit depth of palette indices to N bits, where N is 1, 2 or 4.", cxxopts::value(pruneIndices.value)},
    4,
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(pruneIndices.cxxOption.opts_))
        {
            REQUIRE(pruneIndices.value == 1 || pruneIndices.value == 2 || pruneIndices.value == 4, std::runtime_error, "Bit depth must be 1, 2 or 4");
            pruneIndices.isSet = true;
        }
    }};

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
            REQUIRE(width >= 8 && width % 8 == 0, std::runtime_error, "Sprite width must be >= 8 and a multiple of 8");
            auto height = sprites.value.at(1);
            REQUIRE(height >= 8 && height % 8 == 0, std::runtime_error, "Sprite height must be >= 8 and a multiple of 8");
            sprites.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::tiles{
    false,
    {"tiles", "Cut data into 8x8 tiles and store data tile-wise. The image needs to be paletted and its width and height must be a multiple of 8 pixels.", cxxopts::value(tiles.isSet)}};

ProcessingOptions::OptionT<bool> ProcessingOptions::tilemap{
    false,
    {"tilemap", "Output optimized screen and tile map for the input image. Will detect flipped tiles if --tilemap=true. Implies --tiles. The image needs to be paletted and its width and height must be a multiple of 8 pixels.", cxxopts::value(tilemap.value)},
    false,
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(tilemap.cxxOption.opts_))
        {
            tilemap.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::deltaImage{
    false,
    {"deltaimage", "Pixel-wise delta encoding between successive images.", cxxopts::value(deltaImage.isSet)}};

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

/*ProcessingOptions::Option ProcessingOptions::rle{
    false,
    {"rle", "Use RLE compression.", cxxopts::value(rle.isSet)}};*/

ProcessingOptions::Option ProcessingOptions::vram{
    false,
    {"vram", "Make compression VRAM-safe.", cxxopts::value(vram.isSet)}};

ProcessingOptions::Option ProcessingOptions::dxt{
    false,
    {"dxt", "Use DXT1-ish RGB555 compression.", cxxopts::value(dxt.isSet)}};

ProcessingOptions::OptionT<std::vector<double>> ProcessingOptions::dxtv{
    false,
    {"dxtv", "Use DXT1-ish RGB555 compression. With intra- and inter-frame compression. Parameters are keyframe interval in [0,60] (0 = none) and max. block error in [0.01,1], e.g. \"--dxtv=5,0.15\"", cxxopts::value(dxtv.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(dxtv.cxxOption.opts_))
        {
            REQUIRE(dxtv.value.size() == 2, std::runtime_error, "DXTV parameter format must be \"Keyframe interval, Max. block error\", e.g. \"--dxtv=5,0.15\"");
            auto keyframeInterval = static_cast<int32_t>(dxtv.value.at(0));
            REQUIRE(keyframeInterval >= 0 && keyframeInterval <= 60, std::runtime_error, "Keyframe interval must be in [0,60] (0 = none)");
            auto maxBlockError = dxtv.value.at(1);
            REQUIRE(maxBlockError >= 0.01 && maxBlockError <= 1, std::runtime_error, "Max. block error must be in [0.01,1]");
            dxtv.isSet = true;
        }
    }};

ProcessingOptions::Option ProcessingOptions::gvid{
    false,
    {"gvid", "Use GVID video compression.", cxxopts::value(gvid.isSet)}};

ProcessingOptions::Option ProcessingOptions::interleavePixels{
    false,
    {"interleavepixels", "Interleave pixels from different images into one array.", cxxopts::value(interleavePixels.isSet)}};

ProcessingOptions::Option ProcessingOptions::dryRun{
    false,
    {"dryrun", "Process data, but do not write output files.", cxxopts::value(dryRun.isSet)}};

ProcessingOptions::Option ProcessingOptions::dumpResults{
    false,
    {"dump", "Dump image conversion result before output (to \"result/*.png\").", cxxopts::value(dumpResults.isSet)}};

ProcessingOptions::Option ProcessingOptions::binary{
    false,
    {"binary", "Output data as binary blob .bin file instead of .h / .c files.", cxxopts::value(binary.isSet)}};
