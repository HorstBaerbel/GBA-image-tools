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

using namespace Magick;

enum class ConversionMode
{
    None,
    Binary,
    Palette,
    Truecolor
};
ConversionMode m_conversionMode = ConversionMode::None;
std::string m_binaryThresholdString;
uint32_t m_binaryThreshold = 0;
std::string m_paletteColorsString;
uint32_t m_paletteColors = 0;
std::string m_truecolorBitDepthString;
uint32_t m_truecolorBitDepth = 0;

bool m_lz10Compression = false;
bool m_lz11Compression = false;
bool m_asTiles = false;
bool m_asSprites = false;
uint32_t m_spriteWidth = 0;
uint32_t m_spriteHeight = 0;
std::vector<uint8_t> m_spriteSizes;
bool m_doAddColor0 = false;
std::string m_addColor0String;
Color m_addColor0;
bool m_doShiftIndices = false;
std::string m_shiftIndicesString;
uint32_t m_shiftIndicesBy = 0;

std::string m_inFile;
std::string m_outFile;

std::string m_gbalzssPath;

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
    cxxopts::Options options("vid2h", "Convert and compress video file to a .h / .c file to compile it into a program");
    options.allow_unrecognised_options();
    options.add_option("", {"h,help", "Print help"});
    options.add_option("", {"n,shift", "Optional. Increase image index values by N, keeping index #0 at 0. N must be in [1, 255] and resulting indices will be clamped to [0, 255]. Only usable for palette conversion.", cxxopts::value<std::string>(m_shiftIndicesString)});
    options.add_option("", {"a,addcolor0", "Optional. Add COLOR at palette index #0 and increase all other color indices by 1. Only usable for palette conversion. Color format \"abcd012\"", cxxopts::value<std::string>(m_addColor0String)});
    options.add_option("", {"i,infile", "Input video file to convert, e.g. \"foo.avi\"", cxxopts::value<std::string>()});
    options.add_option("", {"o,outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
    options.add_option("", {"b,binary", "Exclusive: Convert to binary image with intensity threshold at N. N will be clamped to [1, 255]", cxxopts::value<std::string>(m_binaryThresholdString)});
    options.add_option("", {"p,palette", "Exclusive: Convert to paletted image with N colors using dithering. N will be clamped to [2, 256]", cxxopts::value<std::string>(m_paletteColorsString)});
    options.add_option("", {"c,truecolor", "Exclusive: Convert to RGB true-color with N bits per color. N will be clamped to [1, 5]", cxxopts::value<std::string>(m_truecolorBitDepthString)});
    options.add_option("", {"0,lz10", "Optional: Use LZ compression variant 10", cxxopts::value<bool>(m_lz10Compression)});
    options.add_option("", {"1,lz11", "Optional: Use LZ compression variant 11", cxxopts::value<bool>(m_lz11Compression)});
    options.add_option("", {"t,tiles", "Optional. Cut data into 8x8 tiles and store data tile-wise. The image needs to be converted to palette and its width and height must be a multiple of 8 pixels", cxxopts::value<bool>(m_asTiles)});
    options.add_option("", {"s,sprites", "Optional. Cut data into sprites of size W x H and store data sprite- and 8x8-tile-wise. The image needs to be converted to palette and its width and height must be a multiple of W and H and also a multiple of 8 pixels. Sprite data is stored in \"1D mapping\" order and can be read with memcpy", cxxopts::value<std::vector<uint8_t>>(m_spriteSizes)});
    options.add_option("", {"positional", "", cxxopts::value<std::vector<std::string>>()});
    options.parse_positional({"infile", "outname", "positional"});
    auto result = options.parse(argc, argv);
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
    // get binary conversion threshold
    if (result.count("binary"))
    {
        if (m_conversionMode != ConversionMode::None)
        {
            std::cerr << "Only one color conversion mode can be selected. Aborting. " << std::endl;
            return false;
        }
        // try converting argument to a number
        try
        {
            auto value = std::atoi(m_binaryThresholdString.c_str());
            if (value < 1 || value > 255)
            {
                std::cerr << "Threshold value must be in [1, 255]. Aborting. " << std::endl;
                return false;
            }
            m_conversionMode = ConversionMode::Binary;
            m_binaryThreshold = value;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_binaryThresholdString << " is not a valid number. Aborting. " << std::endl;
            return false;
        }
    }
    // get palette conversion color count
    if (result.count("palette"))
    {
        if (m_conversionMode != ConversionMode::None)
        {
            std::cerr << "Only one color conversion mode can be selected. Aborting. " << std::endl;
            return false;
        }
        // try converting argument to a number
        try
        {
            auto value = std::atoi(m_paletteColorsString.c_str());
            if (value < 2 || value > 256)
            {
                std::cerr << "Palette color count must be in [2, 256]. Aborting. " << std::endl;
                return false;
            }
            m_conversionMode = ConversionMode::Palette;
            m_paletteColors = value;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_paletteColorsString << " is not a valid number. Aborting. " << std::endl;
            return false;
        }
    }
    // get truecolor bit depth
    if (result.count("truecolor"))
    {
        if (m_conversionMode != ConversionMode::None)
        {
            std::cerr << "Only one color conversion mode can be selected. Aborting. " << std::endl;
            return false;
        }
        // try converting argument to a number
        try
        {
            auto value = std::atoi(m_truecolorBitDepthString.c_str());
            if (value < 1 || value > 5)
            {
                std::cerr << "Truecolor bit depth must be in [1, 5]. Aborting. " << std::endl;
                return false;
            }
            m_conversionMode = ConversionMode::Truecolor;
            m_truecolorBitDepth = value;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_truecolorBitDepthString << " is not a valid number. Aborting. " << std::endl;
            return false;
        }
    }
    // add color #0
    if (result.count("addcolor0"))
    {
        // try converting argument to a color
        std::string colorArg = m_addColor0String;
        colorArg = std::string("#") + colorArg;
        try
        {
            m_addColor0 = Color(colorArg);
            m_doAddColor0 = true;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_addColor0String << " is not a valid color. Format must be e.g. \"--addcolor0=abc012\". Aborting. " << std::endl;
            return false;
        }
    }
    // check index increase
    if (result.count("shift"))
    {
        // try converting argument to a number
        try
        {
            auto value = std::atoi(m_shiftIndicesString.c_str());
            if (value < 1 || value > 255)
            {
                std::cerr << "Shift value must be in [1, 255]. Aborting. " << std::endl;
                return false;
            }
            m_doShiftIndices = true;
            m_shiftIndicesBy = value;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_shiftIndicesString << " is not a valid number. Aborting. " << std::endl;
            return false;
        }
    }
    // check sprites sizes
    if (result.count("sprites"))
    {
        if (m_spriteSizes.size() != 2)
        {
            std::cerr << "Sprite size format must be \"W,H\", e.g. \"--sprites=32,16\". Aborting. " << std::endl;
            return false;
        }
        m_spriteWidth = m_spriteSizes.at(0);
        if (m_spriteWidth < 8 || m_spriteWidth > 64 || m_spriteWidth % 8 != 0)
        {
            std::cerr << "Sprite width must be in [8,64] and a multiple of 8. Aborting." << std::endl;
            return false;
        }
        m_spriteHeight = m_spriteSizes.at(1);
        if (m_spriteHeight < 8 || m_spriteHeight > 64 || m_spriteHeight % 8 != 0)
        {
            std::cerr << "Sprite height must be in [8,64] and a multiple of 8. Aborting." << std::endl;
            return false;
        }
        m_asSprites = true;
    }
    return true;
}

void printUsage()
{
    std::cout << "Converts an compresses a video file to a .c and .h file to compile it into a" << std::endl;
    std::cout << "GBA executable." << std::endl;
    std::cout << "Usage: vid2h COLORS [CONVERSION] [COMPRESSION] INFILE OUTNAME" << std::endl;
    std::cout << "COLORS options (mutually exclusive):" << std::endl;
    std::cout << "--binary=N: Convert to binary image with intensity threshold at N. N will be" << std::endl;
    std::cout << "  clamped to [1, 255]." << std::endl;
    std::cout << "--palette=N: Convert to paletted image with N colors using dithering. N will be" << std::endl;
    std::cout << "  clamped to [2, 256]." << std::endl;
    std::cout << "--truecolor=N: Convert to RGB true-color with N bits per color. N will be" << std::endl;
    std::cout << "  clamped to [1, 5]." << std::endl;
    std::cout << "CONVERSION options (all optional):" << std::endl;
    std::cout << "--addcolor0=COLOR: Add COLOR at palette index #0 and increase all other" << std::endl;
    std::cout << "  color indices by 1. Only usable for palette conversion. Format \"abcd012\"" << std::endl;
    std::cout << "--shift=N: Increase image index values by N, keeping index #0 at 0." << std::endl;
    std::cout << "  N must be in [1, 255] and resulting indices will be clamped to [0, 255]." << std::endl;
    std::cout << "  Only usable for palette conversion." << std::endl;
    std::cout << "--tiles: Cut data into 8x8 tiles and store data tile-wise. The image needs to be" << std::endl;
    std::cout << "  converted to palette and its width and height must be a multiple of 8 pixels." << std::endl;
    std::cout << "--sprites=W,H: Cut data into sprites of size W x H and store data sprite-" << std::endl;
    std::cout << "  and 8x8-tile-wise. The image needs to be converted to palette and its width" << std::endl;
    std::cout << "  and height must be a multiple of W and H and also a multiple of 8 pixels." << std::endl;
    std::cout << "  Sprite data is stored in \"1D mapping\" order and can be read with memcpy." << std::endl;
    std::cout << "COMPRESSION options (mutually exclusive):" << std::endl;
    std::cout << "--lz10: Use LZ compression variant 10 (default: no compression)." << std::endl;
    std::cout << "--lz11: Use LZ compression variant 11 (default: no compression)." << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: Input video file to convert, e.g. \"foo.avi\"" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "EXECUTION ORDER: input, color conversion, addcolor0, shift, sprites, tiles," << std::endl;
    std::cout << "lz10 / lz11, output" << std::endl;
}

int main(int argc, const char *argv[])
{
    // check arguments
    if (argc < 3 || !readArguments(argc, argv))
    {
        printUsage();
        return 2;
    }
    // try finding gbalzss if needed
    auto [lzssAvailable, gbalzssPath] = Lzss::findGbalzss();
    if ((m_lz10Compression || m_lz11Compression) && !lzssAvailable)
    {
        std::cerr << "Necessary gbalzss executable not found. Aborting. " << std::endl;
        return 1;
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
    // check sprites option
    if (m_asSprites && m_spriteWidth % 8 != 0 && m_spriteHeight % 8 != 0)
    {
        std::cerr << "Sprite width and height must be a multiple of 8. Aborting." << std::endl;
        return 1;
    }
    if (m_asSprites && (m_spriteWidth != 8 && m_spriteWidth != 16 && m_spriteWidth != 32 && m_spriteWidth != 64))
    {
        std::cerr << "Warning: Sprite width not in [8, 16, 32, 64]!" << std::endl;
    }
    if (m_asSprites && (m_spriteHeight != 8 && m_spriteHeight != 16 && m_spriteHeight != 32 && m_spriteHeight != 64))
    {
        std::cerr << "Warning: Sprite height not in [8, 16, 32, 64]!" << std::endl;
    }
    // fire up ImageMagick and build GBA reference color map
    InitializeMagick(*argv);
    const auto ColorMapRGB555 = buildColorMapRGB555();
    // fire up ffmpeg reader and open video file
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
            image.quantizeColors(m_paletteColors);
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
    return 0;
}