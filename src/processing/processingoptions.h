#pragma once

#include "color/colorformat.h"
#include "color/xrgb8888.h"
#include "quantizationmethod.h"

#include <cstdint>
#include <string>
#include <vector>

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

    static OptionT<double> blackWhite;
    static OptionT<uint32_t> paletted;
    static OptionT<uint32_t> commonPalette;
    static OptionT<Color::Format> truecolor;
    static OptionT<Color::Format> outformat;
    static OptionT<Image::Quantization::Method> quantizationmethod;
    static Option reorderColors;
    static OptionT<Color::XRGB8888> addColor0;
    static OptionT<Color::XRGB8888> moveColor0;
    static OptionT<uint32_t> shiftIndices;
    static OptionT<uint32_t> pruneIndices;
    static OptionT<std::vector<uint32_t>> sprites;
    static Option tiles;
    static OptionT<bool> tilemap;
    static Option deltaImage;
    static Option delta8;
    static Option delta16;
    static Option lz10;
    // static Option rle;
    static Option vram;
    static Option dxt;
    static OptionT<std::vector<double>> dxtv;
    static Option gvid;
    static Option interleavePixels;
    static Option dryRun;
    static Option dumpResults;
    static Option binary;
};
