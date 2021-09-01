#include "processingoptions.h"

ProcessingOptions::Option::operator bool() const
{
    return isSet;
}

std::string ProcessingOptions::Option::helpString() const
{
    return cxxOption.opts_ + ": " + cxxOption.desc_;
}

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::binary{
    false,
    {"binary", "Convert images to binary image with intensity threshold at N. N must be in [1, 255].", cxxopts::value<uint32_t>(binary.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(binary.cxxOption.opts_))
        {
            if (binary.value < 1 || binary.value > 255)
            {
                std::cerr << "Binarization threshold value must be in [1, 255]." << std::endl;
                return false;
            }
            binary.isSet = true;
        }
        return true;
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::palette{
    false,
    {"palette", "Convert images to paletted image with N colors using dithering. N must be in [2, 256].", cxxopts::value<uint32_t>(palette.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(palette.cxxOption.opts_))
        {
            if (palette.value < 1 || palette.value > 256)
            {
                std::cerr << "Number of palette colors must be in [2, 256]." << std::endl;
                return false;
            }
            palette.isSet = true;
        }
        return true;
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::truecolor{
    false,
    {"truecolor", "Convert images to RGB true-color with N bits per color. N must be in [2, 5].", cxxopts::value<uint32_t>(truecolor.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(truecolor.cxxOption.opts_))
        {
            if (truecolor.value < 2 || truecolor.value > 5)
            {
                std::cerr << "Bits per color must be in [2, 5]." << std::endl;
                return false;
            }
            truecolor.isSet = true;
        }
        return true;
    }};

ProcessingOptions::Option ProcessingOptions::reorderColors{
    false,
    {"reordercolors", "Reorder palette colors to minimize preceived color distance.", cxxopts::value<bool>(reorderColors.isSet)}};

ProcessingOptions::OptionT<Magick::Color> ProcessingOptions::addColor0{
    false,
    {"addcolor0", "Add COLOR at palette index #0 and increase all other color indices by 1. Only usable for paletted images. Color format \"abcd012\".", cxxopts::value<std::string>(addColor0.valueString)},
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
                std::cerr << addColor0.valueString << " is not a valid color. Format must be e.g. \"--addcolor0=abc012\"" << std::endl;
                return false;
            }
            addColor0.isSet = true;
        }
        return true;
    }};

ProcessingOptions::OptionT<Magick::Color> ProcessingOptions::moveColor0{
    false,
    {"movecolor0", "Move COLOR to palette index #0 and move all other colors accordingly. Only usable for paletted images. Color format \"abcd012\".", cxxopts::value<std::string>(moveColor0.valueString)},
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
                std::cerr << moveColor0.valueString << " is not a valid color. Format must be e.g. \"--movecolor0=abc012\"" << std::endl;
                return false;
            }
            moveColor0.isSet = true;
        }
        return true;
    }};

ProcessingOptions::OptionT<uint32_t> ProcessingOptions::shiftIndices{
    false,
    {"shift", "Increase image index values by N, keeping index #0 at 0. N must be in [1, 255] and resulting indices will be clamped to [0, 255]. Only usable for paletted images.", cxxopts::value<uint32_t>(shiftIndices.value)},
    0,
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(shiftIndices.cxxOption.opts_))
        {
            if (shiftIndices.value < 1 || shiftIndices.value > 255)
            {
                std::cerr << "Shift value must be in [1, 255]." << std::endl;
                return false;
            }
            shiftIndices.isSet = true;
        }
        return true;
    }};

ProcessingOptions::Option ProcessingOptions::pruneIndices{
    false,
    {"prune", "Reduce bit depth of palette indices to 4 bit.", cxxopts::value<bool>(pruneIndices.isSet)}};

ProcessingOptions::OptionT<std::vector<uint32_t>> ProcessingOptions::sprites{
    false,
    {"sprites", "Cut data into sprites of size W x H and store data sprite- and 8x8-tile-wise. The image needs to be paletted and its width and height must be a multiple of W and H and also a multiple of 8 pixels. Sprite data is stored in \"1D mapping\" order and can be read with memcpy.", cxxopts::value<std::vector<uint32_t>>(sprites.value)},
    {},
    {},
    [](const cxxopts::ParseResult &r)
    {
        if (r.count(sprites.cxxOption.opts_))
        {
            if (sprites.value.size() != 2)
            {
                std::cerr << "Sprite size format must be \"W,H\", e.g. \"--sprites=32,16\"." << std::endl;
                return false;
            }
            auto width = sprites.value.at(0);
            if (width < 8 || width > 64 || width % 8 != 0)
            {
                std::cerr << "Sprite width must be in [8,64] and a multiple of 8." << std::endl;
                return false;
            }
            auto height = sprites.value.at(1);
            if (height < 8 || height > 64 || height % 8 != 0)
            {
                std::cerr << "Sprite height must be in [8,64] and a multiple of 8." << std::endl;
                return false;
            }
            sprites.isSet = true;
        }
        return true;
    }};

ProcessingOptions::Option ProcessingOptions::tiles{
    false,
    {"tiles", "Cut data into 8x8 tiles and store data tile-wise. The image needs to be paletted and its width and height must be a multiple of 8 pixels.", cxxopts::value<bool>(tiles.isSet)}};

ProcessingOptions::Option ProcessingOptions::delta8{
    false,
    {"delta8", "8-bit delta encoding.", cxxopts::value<bool>(delta8.isSet)}};

ProcessingOptions::Option ProcessingOptions::delta16{
    false,
    {"delta16", "16-bit delta encoding.", cxxopts::value<bool>(delta16.isSet)}};

ProcessingOptions::Option ProcessingOptions::lz10{
    false,
    {"lz10", "Use LZ compression variant 10.", cxxopts::value<bool>(lz10.isSet)}};

ProcessingOptions::Option ProcessingOptions::lz11{
    false,
    {"lz11", "Use LZ compression variant 11.", cxxopts::value<bool>(lz11.isSet)}};

ProcessingOptions::Option ProcessingOptions::vram{
    false,
    {"vram", "Make compression VRAM-safe.", cxxopts::value<bool>(vram.isSet)}};

ProcessingOptions::Option ProcessingOptions::interleavePixels{
    false,
    {"interleavepixels", "Interleave pixels from different images into one array.", cxxopts::value<bool>(interleavePixels.isSet)}};
