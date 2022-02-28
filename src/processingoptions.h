#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Magick++.h>
#include <cxxopts/include/cxxopts.hpp>

class ProcessingOptions
{
public:
    struct Option
    {
        bool isSet = false;
        cxxopts::Option cxxOption;

        operator bool() const;

        std::string helpString() const;
    };

    template <typename T>
    struct OptionT : Option
    {
        T value;
        std::string valueString;
        std::function<void(const cxxopts::ParseResult &)> parse;
    };

    static OptionT<float> blackWhite;
    static OptionT<uint32_t> paletted;
    static OptionT<std::string> truecolor;
    static Option reorderColors;
    static OptionT<Magick::Color> addColor0;
    static OptionT<Magick::Color> moveColor0;
    static OptionT<uint32_t> shiftIndices;
    static OptionT<uint32_t> pruneIndices;
    static OptionT<std::vector<uint32_t>> sprites;
    static Option tiles;
    static OptionT<bool> tilemap;
    static Option deltaImage;
    static Option delta8;
    static Option delta16;
    static Option lz10;
    static Option lz11;
    static Option rle;
    static Option vram;
    static Option dxtg;
    static Option gvid;
    static Option interleavePixels;
    static Option dryRun;
    static Option binary;
};
