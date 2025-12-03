#pragma once

#include "audio/audioformat.h"
#include "color/colorformat.h"
#include "color/xrgb8888.h"
#include "image/quantizationmethod.h"

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

    // Image options
    static Option video;
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
    static Option rans;
    static Option lz10;
    // static Option rle;
    static Option vram;
    static Option dxt;
    static OptionT<double> dxtv;
    // static Option gvid;
    static Option interleavePixels;

    // Audio options
    static Option audio;
    static OptionT<uint32_t> sampleRateHz;
    static OptionT<Audio::ChannelFormat> channelFormat;
    static OptionT<Audio::SampleFormat> sampleFormat;
    static Option adpcm;

    // Meta data options
    static OptionT<std::string> metaFile;
    static OptionT<std::string> metaString;

    // General options
    static Option printStats;
    static Option dryRun;
    static Option dumpImage;
    static Option dumpAudio;
    static Option dumpMeta;
    static Option outputStats;
    static Option binary;
};
