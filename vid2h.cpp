// Converts and (optionally) compresses image files using
// intra-frame-funkystuff-LZ77 compression to a GBA-compatible format.
// Only paletted and true color images are allowed. The alpha channel is ignored.
// The compressed file is converted to arrays in a .c file and
// a proper .h is generated.
// You can compile / link the resulting files to your program like
// normal .c files. All images are stored as 32-bit hex strings
// and padded to a multiple of 4 bytes as needed.

// You'll need ffmpeg installed
// You'll need libmagick++-dev installed!

#include "colorhelpers.h"
#include "datahelpers.h"
#include "filehelpers.h"
#include "videoreader.h"
#include "imagehelpers.h"
#include "imageprocessing.h"
#include "processingoptions.h"
#include "lzsshelpers.h"
#include "spritehelpers.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>

enum class ConversionMode
{
    None,
    Binary,
    Palette,
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
    cxxopts::Options opts("vid2h", "Convert and compress video file to a .h / .c file to compile it into a program");
    opts.allow_unrecognised_options();
    opts.add_option("", {"h,help", "Print help"});
    opts.add_option("", {"infile", "Input video file to convert, e.g. \"foo.avi\"", cxxopts::value<std::string>()});
    opts.add_option("", {"outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
    opts.add_option("", options.binary.cxxOption);
    opts.add_option("", options.palette.cxxOption);
    opts.add_option("", options.truecolor.cxxOption);
    opts.add_option("", options.addColor0.cxxOption);
    opts.add_option("", options.moveColor0.cxxOption);
    opts.add_option("", options.shiftIndices.cxxOption);
    opts.add_option("", options.pruneIndices.cxxOption);
    opts.add_option("", options.sprites.cxxOption);
    opts.add_option("", options.tiles.cxxOption);
    opts.add_option("", options.delta8.cxxOption);
    opts.add_option("", options.delta16.cxxOption);
    opts.add_option("", options.lz10.cxxOption);
    opts.add_option("", options.lz11.cxxOption);
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
    if ((options.binary + options.palette + options.truecolor) == 0)
    {
        std::cerr << "One format option is needed." << std::endl;
        return false;
    }
    if ((options.binary + options.palette + options.truecolor) > 1)
    {
        std::cerr << "Only a single format option is allowed." << std::endl;
        return false;
    }
    if (options.lz10 && options.lz11)
    {
        std::cerr << "Only a single LZ-compression option is allowed." << std::endl;
        return false;
    }
    return options.binary.parse(result) && options.palette.parse(result) && options.truecolor.parse(result) &&
           options.addColor0.parse(result) && options.moveColor0.parse(result) &&
           options.shiftIndices.parse(result) && options.sprites.parse(result);
}

void printUsage()
{
    std::cout << "Converts an compresses a video file to a .c and .h file to compile it into a" << std::endl;
    std::cout << "GBA executable." << std::endl;
    std::cout << "Usage: vid2h FORMAT [CONVERSION] [COMPRESSION] INFILE OUTNAME" << std::endl;
    std::cout << "FORMAT options (mutually exclusive):" << std::endl;
    std::cout << options.binary.helpString() << std::endl;
    std::cout << options.palette.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "CONVERSION options (all optional):" << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << "COMPRESSION options (mutually exclusive):" << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << options.lz11.helpString() << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "EXECUTION ORDER: input, color conversion, addcolor0, movecolor0, shift, sprites," << std::endl;
    std::cout << "tiles, lz10 / lz11, output" << std::endl;
}

int main(int argc, const char *argv[])
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
    if (m_conversionMode == ConversionMode::None)
    {
        std::cerr << "No conversion mode passed. Aborting." << std::endl;
        return 1;
    }
    // resolve wildcards in input files and check if all input files exist
    /*auto result = resolveFilePaths(m_inFile);
    if (!result.first || result.second.empty())
    {
        std::cerr << "Failed to find input files. Aborting. " << std::endl;
        return -4;
    }
    m_inFile = result.second;*/
    try
    {
        // fire up ImageMagick and build GBA reference color map
        Magick::InitializeMagick(*argv);
        const auto ColorMapRGB555 = buildColorMapRGB555();
        // fire up video reader and open video file
        VideoReader videoReader;
        VideoReader::VideoInfo videoInfo;
        try
        {
            std::cout << "Opening " << m_inFile << "..." << std::endl;
            videoReader.open(m_inFile);
            videoInfo = videoReader.getInfo();
            std::cout << "Video stream #" << videoInfo.videoStreamIndex << ": " << videoInfo.codecName << ", " << videoInfo.width << "x" << videoInfo.height << "@" << videoInfo.fps << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to open video file: " << e.what() << std::endl;
            return 1;
        }
        // start reading frames from video
        do
        {
            auto frame = videoReader.readFrame();
            if (frame.empty())
            {
                break;
            }
            Image image(videoInfo.width, videoInfo.height, "RGB", StorageType::CharPixel, frame.data());
            // convert frame image to color mode
            if (m_conversionMode == ConversionMode::Binary)
            {
                // TODO
                std::cout << "Currently not supported" << std::endl;
                return 1;
            }
            else if (m_conversionMode == ConversionMode::Palette)
            {
                // map image to GBA color map, no dithering
                image.map(ColorMapRGB555);
                // convert image to paletted
                image.quantizeDither(true);
                image.quantizeDitherMethod(DitherMethod::RiemersmaDitherMethod);
                image.quantizeColors(options.palette.value);
                // get image data and color map
                auto data = getImageData(image);
                auto colorMap = getColorMap(image);
                // reorder palette colors
                reorderColors(colorMap, data);
                // add extra color #0 if wanted
                if (m_doAddColor0)
                {
                    addColor0(colorMap, data, m_addColor0);
                }
                // shift indices if wanted
                if (m_doShiftIndices)
                {
                    shiftIndices(data, m_shiftIndicesBy);
                }
            }
            else if (m_conversionMode == ConversionMode::Truecolor)
            {
                // TODO
                std::cout << "Currently not supported" << std::endl;
                return 1;
                // map image to GBA color map with dithering
                //image.map(ColorMapRGB555, true);
            }
        } while (true);
        // we have the data. pad to multiple of 4 and compress
        std::cout << "Converting" << std::endl;
        std::vector<std::vector<uint8_t>> processedData;
        uint32_t imageNr = 0;
        for (auto &data : imgData)
        {
            std::cout << "Image #" << imageNr++;
            if (data.empty())
            {
                std::cerr << std::endl
                          << "Empty image data" << std::endl;
                return 1;
            }
            if (m_asTiles)
            {
                std::cout << " tiles";
                data = convertToTiles(data, imgSize.width(), imgSize.height(), nrOfBitsPerPixel);
            }
            else if (m_asSprites)
            {
                std::cout << " sprites";
                auto dataSize = imgSize;
                if (dataSize.width() != m_spriteWidth)
                {
                    data = convertToWidth(data, dataSize.width(), dataSize.height(), nrOfBitsPerPixel, m_spriteWidth);
                    dataSize = Magick::Geometry(m_spriteWidth, (dataSize.width() * dataSize.height()) / m_spriteWidth);
                }
                std::cout << " tiles";
                data = convertToTiles(data, dataSize.width(), dataSize.height(), nrOfBitsPerPixel);
            }
            if (mustCompress())
            {
                if (m_deltaEncoding8)
                {
                    std::cout << " delta-8";
                    data = deltaEncode(data);
                }
                else if (m_deltaEncoding16)
                {
                    std::cout << " delta-16";
                    if (data.size() & 1)
                    {
                        std::cerr << std::endl
                                  << "Image data size must be a multiple of 2 for 16-bit delta-encoding" << std::endl;
                        return 1;
                    }
                    data = convertTo<uint8_t>(deltaEncode(convertTo<uint16_t>(data)));
                }
                if (m_lz10Compression)
                {
                    std::cout << " LZ10";
                    data = compressLzss(data, m_vramCompatible, false);
                }
                else if (m_lz11Compression)
                {
                    std::cout << " LZ11";
                    data = compressLzss(data, m_vramCompatible, true);
                }
                /*std::ofstream of("compressed.hex", std::ios::binary | std::ios::out);
            of.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());
            of.close();*/
                if (data.empty())
                {
                    std::cerr << std::endl
                              << "Compressing image data failed" << std::endl;
                    return 1;
                }
            }
            processedData.push_back(fillUpToMultipleOf(data, 4));
            std::cout << std::endl;
        }
        // adjust image size when using sprites as we've converted them to m_spriteWidth now...
        if (m_asTiles && imgSize.width() != 8)
        {
            imgSize = Magick::Geometry(8, (imgSize.width() * imgSize.height()) / 8);
        }
        else if (m_asSprites && imgSize.width() != m_spriteWidth)
        {
            imgSize = Magick::Geometry(m_spriteWidth, (imgSize.width() * imgSize.height()) / m_spriteWidth);
        }
        // do data and palette interleaving
        if (m_interleaveData)
        {
            try
            {
                auto tempData = interleave(processedData, nrOfBitsPerPixel);
                processedData.clear();
                processedData.push_back(tempData);
                std::cout << "Interleaved image data" << std::endl;
            }
            catch (std::runtime_error e)
            {
                std::cerr << "Failed to interleave image data: " << e.what() << std::endl;
                return 1;
            }
        }
        // open output files
        std::ofstream hFile(m_outFile + ".h", std::ios::out);
        std::ofstream cFile(m_outFile + ".c", std::ios::out);
        if (hFile.is_open() && cFile.is_open())
        {
            std::cout << "Writing output files " << m_outFile << ".h, " << m_outFile << ".c" << std::endl;
            try
            {
                std::string baseName = getBaseNameFromFilePath(m_outFile);
                std::string varName = baseName;
                std::transform(varName.begin(), varName.end(), varName.begin(), [](char c)
                               { return std::toupper(c, std::locale()); });
                // output header
                hFile << "// Converted with img2h " << getCommandLine(argc, argv) << std::endl;
                if (mustCompress())
                {
                    hFile << "// Compression type";
                    if (m_deltaEncoding8 || m_deltaEncoding16)
                    {
                        hFile << " Diff" << (m_deltaEncoding8 ? "8" : "16");
                    }
                    if (m_lz10Compression || m_lz11Compression)
                    {
                        hFile << " LZSS variant " << (m_lz11Compression ? "11" : "10");
                    }
                    hFile << (m_vramCompatible ? ", VRAM-safe" : "") << std::endl;
                }
                hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                      << std::endl;
                // output image and palette info
                const bool storeTileOrSpriteWise = (imgData.size() == 1) && (m_asTiles || m_asSprites);
                uint32_t nrOfBytesPerImageOrSprite = imgSize.width() * imgSize.height();
                uint32_t nrOfImagesOrSprites = imgData.size();
                if (nrOfImagesOrSprites == 1)
                {
                    // if we have a single input image, store data per tile or sprite
                    if (m_asTiles)
                    {
                        // calculate number of 8*8 pixel tiles
                        nrOfImagesOrSprites = (imgSize.width() * imgSize.height()) / 64;
                        nrOfBytesPerImageOrSprite = 64;
                        imgSize = Magick::Geometry(8, 8);
                    }
                    else if (m_asSprites)
                    {
                        // calculate number of w*h sprites
                        nrOfImagesOrSprites = (imgSize.width() * imgSize.height()) / (m_spriteWidth * m_spriteHeight);
                        nrOfBytesPerImageOrSprite = m_spriteWidth * m_spriteHeight;
                        imgSize = Magick::Geometry(m_spriteWidth, m_spriteHeight);
                    }
                }
                nrOfBytesPerImageOrSprite = imgType == ImageType::PaletteType ? (maxColorMapColors <= 16 ? (nrOfBytesPerImageOrSprite / 2) : nrOfBytesPerImageOrSprite) : (nrOfBytesPerImageOrSprite * 2);
                // convert image data to uint32_ts and palette to BGR555 uint16_ts
                auto imageData32 = combineTo<uint32_t>(processedData);
                auto paletteData16 = allColorMapsSame ? convertToBGR555(colorMaps.front()) : combineTo<uint16_t>(convertToBGR555(colorMaps));
                // get start indices for image data and palettes
                auto imageOrSpriteStartIndices = divideBy<uint32_t>(getStartIndices(processedData), 4);
                auto colorMapsStartIndices = getStartIndices(colorMaps);
                // make sure we have the correct number of images. sprites and tiles will have no start indices, thus we need to use nrOfImagesOrSprites
                nrOfImagesOrSprites = imageOrSpriteStartIndices.size() > 1 ? imageOrSpriteStartIndices.size() : nrOfImagesOrSprites;
                // output header
                writeImageInfoToH(hFile, varName, imageData32, imgSize.width(), imgSize.height(), nrOfBytesPerImageOrSprite, nrOfImagesOrSprites, storeTileOrSpriteWise);
                if (imgType == ImageType::PaletteType)
                {
                    writePaletteInfoToHeader(hFile, varName, paletteData16, maxColorMapColors, allColorMapsSame || colorMapsStartIndices.size() <= 1, storeTileOrSpriteWise);
                }
                hFile << std::endl;
                hFile.close();
                // output image and palette data
                writeImageDataToC(cFile, varName, baseName, imageData32, imageOrSpriteStartIndices, storeTileOrSpriteWise);
                if (imgType == ImageType::PaletteType)
                {
                    writePaletteDataToC(cFile, varName, paletteData16, colorMapsStartIndices, storeTileOrSpriteWise);
                }
                cFile.close();
            }
            catch (std::runtime_error e)
            {
                hFile.close();
                cFile.close();
                std::cerr << "Failed to write data to output files: " << e.what() << std::endl;
                return 1;
            }
        }
        else
        {
            hFile.close();
            cFile.close();
            std::cerr << "Failed to open " << m_outFile << ".h, " << m_outFile << ".c for writing" << std::endl;
            return 1;
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