#include "color/colorhelpers.h"
#include "color/conversions.h"
#include "compression/lzss.h"
#include "io/textio.h"
#include "io/streamio.h"
#include "io/videoreader.h"
#include "processing/datahelpers.h"
#include "processing/imagehelpers.h"
#include "processing/imageprocessing.h"
#include "processing/processingoptions.h"
#include "processing/spritehelpers.h"
#include "statistics/statistics_window.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <filesystem>

#include "cxxopts/include/cxxopts.hpp"

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
        opts.add_option("", options.dxtg.cxxOption);
        opts.add_option("", options.dxtv.cxxOption);
        // opts.add_option("", options.gvid.cxxOption);
        // opts.add_option("", options.rle.cxxOption);
        opts.add_option("", options.lz10.cxxOption);
        opts.add_option("", options.lz11.cxxOption);
        opts.add_option("", options.vram.cxxOption);
        opts.add_option("", options.dryRun.cxxOption);
        opts.parse_positional({"infile", "outname"});
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
            if (!std::filesystem::exists(m_inFile))
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
        options.colorformat.parse(result);
        if (!options.colorformat)
        {
            std::cerr << "Output color format must be set." << std::endl;
            return false;
        }
        options.quantizationmethod.parse(result);
        options.addColor0.parse(result);
        options.moveColor0.parse(result);
        options.shiftIndices.parse(result);
        options.pruneIndices.parse(result);
        options.sprites.parse(result);
        options.dxtv.parse(result);
    }
    catch (const cxxopts::exceptions::parsing &e)
    {
        std::cerr << "Argument error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void printUsage()
{
    // 80 chars:  --------------------------------------------------------------------------------
    std::cout << "Converts and compresses a video file to a .c and .h file to compile it into a" << std::endl;
    std::cout << "GBA executable." << std::endl;
    std::cout << "Usage: vid2h FORMAT [CONVERT] [IMAGE COMPRESS] [COMPRESS] INFILE OUTNAME" << std::endl;
    std::cout << "FORMAT options (mutually exclusive):" << std::endl;
    std::cout << options.blackWhite.helpString() << std::endl;
    std::cout << options.paletted.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "Color format (must be set):" << std::endl;
    std::cout << options.colorformat.helpString() << std::endl;
    std::cout << "CONVERT options (all optional):" << std::endl;
    std::cout << options.quantizationmethod.helpString() << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.deltaImage.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << "IMAGE COMPRESS options (mutually exclusive):" << std::endl;
    std::cout << options.dxtg.helpString() << std::endl;
    std::cout << options.dxtv.helpString() << std::endl;
    // std::cout << options.gvid.helpString() << std::endl;
    std::cout << "COMPRESS options (mutually exclusive):" << std::endl;
    // std::cout << options.rle.helpString() << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << options.lz11.helpString() << std::endl;
    std::cout << "COMPRESS modifiers (optional):" << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "MISC options (all optional):" << std::endl;
    std::cout << options.dryRun.helpString() << std::endl;
    std::cout << "help: Show this help." << std::endl;
    std::cout << "ORDER: input, color conversion, addcolor0, movecolor0, shift, sprites, tiles," << std::endl;
    std::cout << "deltaimage, dxtg / dtxv / gvid, delta8 / delta16, rle, lz10 / lz11, output" << std::endl;
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
            std::cerr << "No output name passed. Aborting." << std::endl;
            return 1;
        }
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
            processing.addStep(Image::ProcessingType::ConvertBlackWhite, {options.colorformat.value, options.quantizationmethod.value, options.blackWhite.value});
        }
        else if (options.paletted)
        {
            // add palette conversion using a RGB555 or RGB565 reference color map
            processing.addStep(Image::ProcessingType::ConvertPaletted, {options.colorformat.value, options.quantizationmethod.value, options.paletted.value});
        }
        else if (options.truecolor)
        {
            processing.addStep(Image::ProcessingType::ConvertTruecolor, {options.colorformat.value, options.quantizationmethod.value});
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
            processing.addStep(Image::ProcessingType::ConvertColorMap, {Color::Format::XRGB1555});
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
        if (options.deltaImage)
        {
            processing.addStep(Image::ProcessingType::DeltaImage, {});
        }
        if (options.dxtg)
        {
            processing.addStep(Image::ProcessingType::CompressDXTG, {}, true, true);
        }
        if (options.dxtv)
        {
            processing.addStep(Image::ProcessingType::CompressDXTV, {options.dxtv.value.at(0), options.dxtv.value.at(1)}, true, true);
        }
        if (options.gvid)
        {
            processing.addStep(Image::ProcessingType::CompressGVID, {}, true, true);
        }
        if (options.delta8)
        {
            processing.addStep(Image::ProcessingType::ConvertDelta8, {});
        }
        if (options.delta16)
        {
            processing.addStep(Image::ProcessingType::ConvertDelta16, {});
        }
        /*if (options.rle)
        {
            processing.addStep(Image::ProcessingType::CompressRLE, {options.vram.isSet}, true);
        }*/
        if (options.lz10)
        {
            processing.addStep(Image::ProcessingType::CompressLZ10, {options.vram.isSet}, true);
        }
        if (options.lz11)
        {
            processing.addStep(Image::ProcessingType::CompressLZ11, {options.vram.isSet}, true);
        }
        processing.addStep(Image::ProcessingType::PadPixelData, {uint32_t(4)});
        // create statistics window
        Statistics::Window window(2 * videoInfo.width, 2 * videoInfo.height);
        processing.setStatisticsContainer(window.getStatisticsContainer());
        // apply image processing pipeline
        const auto processingDescription = processing.getProcessingDescription();
        std::cout << "Applying processing: " << processingDescription << std::endl;
        // start reading frames from video
        uint32_t lastProgress = 0;
        auto startTime = std::chrono::steady_clock::now();
        uint32_t frameIndex = 0;
        std::vector<Image::Data> images;
        do
        {
            auto frame = videoReader.readFrame();
            if (frame.empty())
            {
                break;
            }
            REQUIRE(frame.size() == videoInfo.width * videoInfo.height, std::runtime_error, "Unexpected frame size");
            // build image from frame and apply processing
            images.push_back(processing.processStream(Image::Data{frameIndex++, "", {videoInfo.width, videoInfo.height}, Image::DataType::Bitmap, {}, Color::convertTo<Color::XRGB8888>(frame)}));
            // calculate progress
            uint32_t newProgress = ((100 * images.size()) / videoInfo.nrOfFrames);
            if (lastProgress != newProgress)
            {
                lastProgress = newProgress;
                auto newTime = std::chrono::steady_clock::now();
                auto timePassedMs = std::chrono::duration<double>(newTime - startTime);
                auto fps = static_cast<double>(images.size()) / timePassedMs.count();
                auto restS = (videoInfo.nrOfFrames - images.size()) / fps;
                std::cout << std::fixed << std::setprecision(1) << lastProgress << "%, " << fps << " fps, " << restS << "s remaining" << std::endl;
            }
            // update statistics
            window.update();
        } while (true);
        // set up some image info
        const bool allColorMapsSame = false;
        const uint32_t maxColorMapColors = options.paletted ? (options.pruneIndices ? 16 : (options.paletted.value + (options.addColor0 ? 1 : 0))) : 0;
        // output some info about data
        const auto inputSize = videoInfo.width * videoInfo.height * 3 * videoInfo.nrOfFrames;
        std::cout << "Input size: " << static_cast<double>(inputSize) / (1024 * 1024) << " MB" << std::endl;
        const auto compressedSize = std::accumulate(images.cbegin(), images.cend(), 0, [](const auto &v, const auto &img)
                                                    { return v + img.imageData.pixels().size() + (options.paletted ? img.imageData.colorMap().size() * 2 : 0); });
        std::cout << "Compressed size: " << std::fixed << std::setprecision(2) << static_cast<double>(compressedSize) / (1024 * 1024) << " MB" << std::endl;
        std::cout << "Avg. bit rate: " << std::fixed << std::setprecision(2) << (static_cast<double>(compressedSize) / 1024) / videoInfo.durationS << " kB/s" << std::endl;
        std::cout << "Avg. frame size: " << std::fixed << std::setprecision(1) << static_cast<double>(compressedSize) / images.size() << " Byte" << std::endl;
        if (videoInfo.fps > 255 || (videoInfo.fps - std::round(videoInfo.fps)) != 0)
        {
            std::cout << "Frame rate of " << std::fixed << std::setprecision(2) << videoInfo.fps << " will be set to ";
            videoInfo.fps = std::round(videoInfo.fps);
            videoInfo.fps = videoInfo.fps > 255 ? 255 : videoInfo.fps;
            std::cout << videoInfo.fps << std::endl;
        }
        // find out the max. memory needed to decompress
        const auto maxMemoryNeeded = std::max_element(images.cbegin(), images.cend(), [](const auto &img0, const auto &img1)
                                                      { return img0.maxMemoryNeeded < img1.maxMemoryNeeded; })
                                         ->maxMemoryNeeded;
        std::cout << "Max. intermediate memory for decompression: " << maxMemoryNeeded << " Byte" << std::endl;
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
                    IO::Stream::writeFileHeader(binFile, images, static_cast<uint8_t>(videoInfo.fps), maxMemoryNeeded);
                    IO::Stream::writeFrames(binFile, images);
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