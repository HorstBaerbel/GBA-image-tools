#include "colorhelpers.h"
#include "compresshelpers.h"
#include "datahelpers.h"
#include "filehelpers.h"
#include "imagehelpers.h"
#include "imageio.h"
#include "imageprocessing.h"
#include "processingoptions.h"
#include "spritehelpers.h"
#include "videoreader.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>

enum class ConversionMode
{
    None,
    BlackWhite,
    Paletted,
    Truecolor
};
ConversionMode m_conversionMode = ConversionMode::None;

std::string m_inFile;
std::string m_outFile;
ProcessingOptions options;

std::string getCommandLine(int argc, const char *argv[])
{
    std::string result;
    for (int i = 1; i < argc; i++)
    {
        result += std::string(argv[i]);
        if (i < (argc - 1))
        {
            result += " ";
        }
    }
    return result;
}

bool readArguments(int argc, const char *argv[])
{
    try
    {
        cxxopts::Options opts("vid2h", "Convert and compress a video file to .h / .c files or a binary file");
        opts.allow_unrecognised_options();
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"infile", "Input video file to convert, e.g. \"foo.avi\"", cxxopts::value<std::string>()});
        opts.add_option("", {"outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
        opts.add_option("", options.blackWhite.cxxOption);
        opts.add_option("", options.paletted.cxxOption);
        opts.add_option("", options.truecolor.cxxOption);
        opts.add_option("", options.addColor0.cxxOption);
        opts.add_option("", options.moveColor0.cxxOption);
        opts.add_option("", options.shiftIndices.cxxOption);
        opts.add_option("", options.pruneIndices.cxxOption);
        opts.add_option("", options.sprites.cxxOption);
        opts.add_option("", options.tiles.cxxOption);
        opts.add_option("", options.deltaImage.cxxOption);
        opts.add_option("", options.delta8.cxxOption);
        opts.add_option("", options.delta16.cxxOption);
        opts.add_option("", options.dxt1.cxxOption);
        opts.add_option("", options.rle.cxxOption);
        opts.add_option("", options.lz10.cxxOption);
        opts.add_option("", options.lz11.cxxOption);
        opts.add_option("", options.vram.cxxOption);
        opts.add_option("", options.dryRun.cxxOption);
        opts.add_option("", {"positional", "", cxxopts::value<std::vector<std::string>>()});
        opts.parse_positional({"infile", "outname", "positional"});
        auto result = opts.parse(argc, argv);
        // check if help was requested
        if (result.count("h"))
        {
            return false;
        }
        // get output file / name
        if (result.count("outname"))
        {
            m_outFile = result["outname"].as<std::string>();
        }
        // get input file
        if (result.count("infile"))
        {
            m_inFile = result["infile"].as<std::string>();
            if (!stdfs::exists(m_inFile))
            {
                std::cout << "Input file \"" << m_inFile << "\" does not exist!" << std::endl;
                return false;
            }
        }
        // if input or output are empty, get positional arguments as input first, then output
        if (result.count("positional"))
        {
            auto positional = result["positional"].as<std::vector<std::string>>();
            auto pIt = positional.cbegin();
            while (pIt != positional.cend() && (m_inFile.empty() || m_outFile.empty()))
            {
                if (m_inFile.empty())
                {
                    m_inFile = *pIt++;
                }
                if (m_outFile.empty())
                {
                    m_outFile = *pIt++;
                }
            }
        }
        // check if exclusive options set
        options.blackWhite.parse(result);
        options.paletted.parse(result);
        options.truecolor.parse(result);
        if ((options.blackWhite + options.paletted + options.truecolor) == 0)
        {
            std::cerr << "One format option is needed." << std::endl;
            return false;
        }
        if ((options.blackWhite + options.paletted + options.truecolor) > 1)
        {
            std::cerr << "Only a single format option is allowed." << std::endl;
            return false;
        }
        if (options.lz10 && options.lz11)
        {
            std::cerr << "Only a single LZ-compression option is allowed." << std::endl;
            return false;
        }
        options.addColor0.parse(result);
        options.moveColor0.parse(result);
        options.shiftIndices.parse(result);
        options.pruneIndices.parse(result);
        options.sprites.parse(result);
    }
    catch (const cxxopts::OptionException &e)
    {
        std::cerr << "Argument error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void printUsage()
{
    std::cout << "Converts an compresses a video file to a .c and .h file to compile it into a" << std::endl;
    std::cout << "GBA executable." << std::endl;
    std::cout << "Usage: vid2h FORMAT [CONVERSION] [IMAGE COMPRESSION] [COMPRESSION] INFILE OUTNAME" << std::endl;
    std::cout << "FORMAT options (mutually exclusive):" << std::endl;
    std::cout << options.blackWhite.helpString() << std::endl;
    std::cout << options.paletted.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "CONVERSION options (all optional):" << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.deltaImage.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << "IMAGE COMPRESSION options (mutually exclusive):" << std::endl;
    std::cout << options.dxt1.helpString() << std::endl;
    std::cout << "COMPRESSION options (mutually exclusive):" << std::endl;
    std::cout << options.rle.helpString() << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << options.lz11.helpString() << std::endl;
    std::cout << "COMPRESSION modifiers (optional):" << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "MISC options (all optional):" << std::endl;
    std::cout << options.dryRun.helpString() << std::endl;
    std::cout << "ORDER: input, color conversion, addcolor0, movecolor0, shift, sprites," << std::endl;
    std::cout << "tiles, deltaimage, dxt1, delta8 / delta16, rle, lz10 / lz11, output" << std::endl;
}

int main(int argc, const char *argv[])
{
    try
    {
        // check arguments
        if (argc < 3 || !readArguments(argc, argv))
        {
            printUsage();
            return 2;
        }
        // check input and output
        if (m_inFile.empty())
        {
            std::cerr << "No input file passed. Aborting." << std::endl;
            return 1;
        }
        if (m_outFile.empty())
        {
            std::cerr << "No output file passed. Aborting." << std::endl;
            return 1;
        }
        // fire up ImageMagick
        Magick::InitializeMagick(*argv);
        // fire up video reader and open video file
        VideoReader videoReader;
        VideoReader::VideoInfo videoInfo;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            videoReader.open(m_inFile);
            videoInfo = videoReader.getInfo();
            std::cout << "Video stream #" << videoInfo.videoStreamIndex << ": " << videoInfo.codecName << ", " << videoInfo.width << "x" << videoInfo.height << "@" << videoInfo.fps;
            std::cout << ", duration " << videoInfo.durationS << "s, " << videoInfo.nrOfFrames << " frames" << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // build processing pipeline - input
        Image::Processing processing;
        if (options.blackWhite)
        {
            processing.addStep(Image::ProcessingType::InputBlackWhite, {options.blackWhite.value});
        }
        else if (options.paletted)
        {
            // add palette conversion using GBA RGB555 reference color map
            processing.addStep(Image::ProcessingType::InputPaletted, {buildColorMapRGB555(), options.paletted.value});
        }
        else if (options.truecolor)
        {
            processing.addStep(Image::ProcessingType::InputTruecolor, {options.truecolor.value});
        }
        // build processing pipeline - conversion
        if (options.paletted)
        {
            processing.addStep(Image::ProcessingType::ReorderColors, {});
            if (options.addColor0)
            {
                processing.addStep(Image::ProcessingType::AddColor0, {options.addColor0.value});
            }
            if (options.moveColor0)
            {
                processing.addStep(Image::ProcessingType::MoveColor0, {options.moveColor0.value});
            }
            if (options.shiftIndices)
            {
                processing.addStep(Image::ProcessingType::ShiftIndices, {options.shiftIndices.value});
            }
            if (options.pruneIndices)
            {
                processing.addStep(Image::ProcessingType::PruneIndices, {});
                // TODO store 1, 2, 4 bits
                processing.addStep(Image::ProcessingType::PadColorMap, {uint32_t(16)});
            }
            else
            {
                processing.addStep(Image::ProcessingType::PadColorMap, {options.paletted.value + (options.addColor0 ? 1 : 0)});
            }
            processing.addStep(Image::ProcessingType::ConvertColorMap, {Image::ColorFormat::RGB555});
            processing.addStep(Image::ProcessingType::PadColorMapData, {uint32_t(4)});
        }
        if (options.sprites)
        {
            processing.addStep(Image::ProcessingType::ConvertSprites, {options.sprites.value.front()});
        }
        if (options.tiles)
        {
            processing.addStep(Image::ProcessingType::ConvertTiles, {});
        }
        processing.addStep(Image::ProcessingType::Uncompressed, {}, true);
        if (options.deltaImage)
        {
            processing.addStep(Image::ProcessingType::DeltaImage, {});
        }
        if (options.dxt1)
        {
            processing.addStep(Image::ProcessingType::CompressDXT1, {}, true);
        }
        if (options.delta8)
        {
            processing.addStep(Image::ProcessingType::ConvertDelta8, {});
        }
        if (options.delta16)
        {
            processing.addStep(Image::ProcessingType::ConvertDelta16, {});
        }
        if (options.rle)
        {
            processing.addStep(Image::ProcessingType::CompressRLE, {options.vram.isSet}, true);
        }
        if (options.lz10)
        {
            processing.addStep(Image::ProcessingType::CompressLz10, {options.vram.isSet}, true);
        }
        if (options.lz11)
        {
            processing.addStep(Image::ProcessingType::CompressLz11, {options.vram.isSet}, true);
        }
        processing.addStep(Image::ProcessingType::PadImageData, {uint32_t(4)});
        // apply image processing pipeline
        const auto processingDescription = processing.getProcessingDescription();
        std::cout << "Applying processing: " << processingDescription << std::endl;
        // start reading frames from video
        uint32_t lastProgress = 0;
        auto startTime = std::chrono::steady_clock::now();
        std::vector<Image::Data> images;
        do
        {
            auto frame = videoReader.readFrame();
            if (frame.empty())
            {
                break;
            }
            // build image from frame and apply processing
            images.push_back(processing.processStream(Magick::Image(videoInfo.width, videoInfo.height, "RGB", Magick::StorageType::CharPixel, frame.data())));
            uint32_t newProgress = ((100 * images.size()) / videoInfo.nrOfFrames);
            if (lastProgress != newProgress)
            {
                lastProgress = newProgress;
                auto newTime = std::chrono::steady_clock::now();
                auto timePassedMs = std::chrono::duration<double>(newTime - startTime);
                auto fps = static_cast<double>(images.size()) / timePassedMs.count();
                auto restS = (videoInfo.nrOfFrames - images.size()) / fps;
                std::cout << lastProgress << "%, " << fps << " fps, " << restS << "s remaining" << std::endl;
            }
        } while (true);
        // set up some image info
        const auto imgType = images.front().type;
        auto imgSize = images.front().size;
        const bool allColorMapsSame = false;
        const uint32_t maxColorMapColors = options.paletted ? (options.pruneIndices ? 16 : (options.paletted.value + (options.addColor0 ? 1 : 0))) : 0;
        // output some info about data
        auto inputSize = videoInfo.width * videoInfo.height * 3 * videoInfo.nrOfFrames;
        std::cout << "Input size: " << inputSize / (1024 * 1024) << " MB" << std::endl;
        auto compressedSize = std::accumulate(images.cbegin(), images.cend(), 0, [](const auto &v, const auto &img)
                                              { return v + img.data.size() + (options.paletted ? img.colorMap.size() * 2 : 0); });
        std::cout << "Compressed size: " << static_cast<double>(compressedSize) / (1024 * 1024) << " MB" << std::endl;
        std::cout << "Bit rate: " << (static_cast<double>(compressedSize) / 1024) / videoInfo.durationS << " kB/s" << std::endl;
        if (videoInfo.fps > 255 || (videoInfo.fps - std::trunc(videoInfo.fps)) != 0)
        {
            std::cout << "Frame rate of " << videoInfo.fps << " will be set to ";
            videoInfo.fps = std::round(videoInfo.fps);
            videoInfo.fps = videoInfo.fps > 255 ? 255 : videoInfo.fps;
            std::cout << videoInfo.fps << std::endl;
        }
        // check if we want to write output files
        if (!options.dryRun)
        {
            // open output file
            std::ofstream binFile(m_outFile + ".bin", std::ios::out | std::ios::binary);
            if (binFile.is_open())
            {
                std::cout << "Writing output file " << m_outFile << ".bin" << std::endl;
                try
                {
                    Image::IO::writeFileHeader(binFile, images, static_cast<uint8_t>(videoInfo.fps));
                    Image::IO::writeFrames(binFile, images);
                }
                catch (const std::runtime_error &e)
                {
                    binFile.close();
                    std::cerr << "Failed to write data to output file: " << e.what() << std::endl;
                    return 1;
                }
            }
            else
            {
                std::cerr << "Failed to open " << m_outFile << ".bin for writing" << std::endl;
                return 1;
            }
        }
        std::cout << "Done" << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}