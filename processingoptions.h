#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>

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
        std::string longOption;
        std::function<bool(const cxxopts::ParseResult &)> parse;
    };

    static Option reorderColors;
    static OptionT<Magick::Color> addColor0;
    static OptionT<Magick::Color> moveColor0;
    static OptionT<uint32_t> shiftIndices;
    static Option pruneIndices;
    static OptionT<std::vector<uint32_t>> sprites;
    static Option tiles;
    static Option delta8;
    static Option delta16;
    static Option lz10;
    static Option lz11;
    static Option vram;
    static Option interleaveData;
};
