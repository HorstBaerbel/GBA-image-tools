// Converts and (optionally) compresses image files using
// a GBA-compatible LZSS/LZ77/LZ10 one by one.
// The type and size of the first file MUST match all following files.
// Only paletted and true color images are allowed. The alpha channel is ignored.
// Then all compressed files are converted to arrays in a .c file and
// a proper .h is generated. The palette is also extracted
// from the images in the sequence and added to the .h and .c
// files as an array. If all palettes in the sequence are the same
// only one palette will be stored.
// You can compile / link the resulting files to your program like
// normal .c files. All images are stored as 32-bit hex strings
// and padded to a multiple of 4 bytes as needed.
// Note that this only works for small video sequences, otherwise
// your compiler might barf at you.
// You'll need libmagick++-dev installed!

#include "colorhelpers.h"
#include "datahelpers.h"
#include "filehelpers.h"
#include "imagehelpers.h"
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

bool m_interleaveData = false;
bool m_vramCompatible = false;
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
bool m_doMoveColor0 = false;
std::string m_moveColor0String;
Color m_moveColor0;
bool m_reorderColors = false;
std::vector<std::string> m_inFile;
std::string m_outFile;
std::string m_gbalzssPath;

bool mustCompress()
{
    return m_vramCompatible || m_lz10Compression || m_lz11Compression;
}

bool readArguments(int argc, const char *argv[])
{
    cxxopts::Options options("img2h", "Convert and compress a list images to a .h / .c file to compile it into a program");
    options.allow_unrecognised_options();
    options.add_options()("h,help", "Print help")("a,addcolor0", "Optional. Add COLOR at palette index #0 and increase all other color indices by 1. Only usable for paletted images. Color format \"abcd012\"", cxxopts::value<std::string>(m_addColor0String))("i,infile", "Input file(s), e.g. \"foo.png\"", cxxopts::value<std::vector<std::string>>())("o,outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>())("v,vram", "Optional: Make compression VRAM-safe", cxxopts::value<bool>(m_vramCompatible))("0,lz10", "Optional: Use LZ compression variant 10 (default: no compression)", cxxopts::value<bool>(m_lz10Compression))("1,lz11", "Optional: Use LZ compression variant 11 (default: no compression)", cxxopts::value<bool>(m_lz11Compression))("t,tiles", "Optional. Cut data into 8x8 tiles and store data tile-wise. The image needs to be paletted and its width and height must be a multiple of 8 pixels", cxxopts::value<bool>(m_asTiles))("s,sprites", "Optional. Cut data into sprites of size W x H and store data sprite- and 8x8-tile-wise. The image needs to be paletted and its width and height must be a multiple of W and H and also a multiple of 8 pixels. Sprite data is stored in \"1D mapping\" order and can be read with memcpy", cxxopts::value<std::vector<uint8_t>>(m_spriteSizes))("m,movecolor0", "Optional. Move COLOR to palette index #0 and move all other colors accordingly. Only usable for paletted images. Color format \"abcd012\"", cxxopts::value<std::string>(m_moveColor0String))("d,interleavedata", "Optional: Interleave all image data into one array", cxxopts::value<bool>(m_interleaveData))("r,reordercolors", "Optional: Reorder palette colors to minimize preceived color distance", cxxopts::value<bool>(m_reorderColors))("positional", "", cxxopts::value<std::vector<std::string>>());
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
    // get input file(s)
    if (result.count("infile"))
    {
        m_inFile = result["infile"].as<std::vector<std::string>>();
        // get last positional argument as output file / name
        if (m_outFile.empty())
        {
            m_outFile = m_inFile.back();
            m_inFile.resize(m_inFile.size() - 1);
        }
        // make sure all input files exist
        for (const auto &fileName : m_inFile)
        {
            if (!stdfs::exists(fileName))
            {
                std::cout << "Input file \"" << fileName << "\" does not exist!" << std::endl;
                return false;
            }
        }
    }
    else
    {
        std::cout << "No input file passed!" << std::endl;
        return false;
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
    // check color moving
    if (result.count("movecolor0"))
    {
        // try converting argument to a color
        std::string colorArg = m_moveColor0String;
        colorArg = std::string("#") + colorArg;
        try
        {
            m_moveColor0 = Color(colorArg);
            m_doMoveColor0 = true;
        }
        catch (const Exception &ex)
        {
            std::cerr << m_moveColor0String << " is not a valid color. Format must be e.g. \"--movecolor0=abc012\". Aborting. " << std::endl;
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
    }
    return true;
}

void printUsage()
{
    std::cout << "Convert a (list of) image files to a .c and .h file to compile them into a" << std::endl;
    std::cout << "GBA executable. Optionally compress data with GBA-compatible LZSS/LZ77." << std::endl;
    std::cout << "Will either save indices and a palette or truecolor data. All color values" << std::endl;
    std::cout << "will be converted to RGB555 directly." << std::endl;
    std::cout << "You might want to use ImageMagicks \"convert +remap\" before." << std::endl;
    std::cout << "Usage: img2h [CONVERSION] [COMPRESSION] INFILE [INFILEn...] OUTNAME" << std::endl;
    std::cout << "CONVERSION options (all optional):" << std::endl;
    std::cout << "--reordercolors: Reorder palette colors to minimize preceived color distance." << std::endl;
    std::cout << "  Only usable for paletted images." << std::endl;
    std::cout << "--addcolor0=COLOR: Add COLOR at palette index #0 and increase all other" << std::endl;
    std::cout << "  color indices by 1. Only usable for paletted images. Format \"abcd012\"" << std::endl;
    std::cout << "--movecolor0=COLOR: Move COLOR to palette index #0 and move all other" << std::endl;
    std::cout << "  colors accordingly. Only usable for paletted images. Format \"abcd012\"" << std::endl;
    std::cout << "--tiles: Cut data into 8x8 tiles and store data tile-wise. The image needs to" << std::endl;
    std::cout << "  be paletted and its width and height must be a multiple of 8 pixels." << std::endl;
    std::cout << "--sprites=W,H: Cut data into sprites of size W x H and store data sprite-" << std::endl;
    std::cout << "  and 8x8-tile-wise. The image needs to be paletted and its width and" << std::endl;
    std::cout << "  height must be a multiple of W and H and also a multiple of 8 pixels." << std::endl;
    std::cout << "  Sprite data is stored in \"1D mapping\" order and can be read with memcpy." << std::endl;
    std::cout << "--interleavedata: Interleave image data into one big array. Interleaving is" << std::endl;
    std::cout << "  done like this (image/value): I0V0,I1V0,I2V0,I0V1,I1V1,I2V1..." << std::endl;
    std::cout << "COMPRESSION options (all optional):" << std::endl;
    std::cout << "--lz10: Use LZ compression variant 10 (default: no compression)." << std::endl;
    std::cout << "--lz11: Use LZ compression variant 11 (default: no compression)." << std::endl;
    std::cout << "--vram: Make compression GBA VRAM-safe." << std::endl;
    std::cout << "  Valid combinations are e.g. \"--lz10 --vram\" or \"--lz11 --vram\"." << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: can be a file list and/or can have * as a wildcard. Multiple input " << std::endl;
    std::cout << "images MUST have the same type (palette / true color) and resolution!" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "EXECUTION ORDER: input, reordercolors, addcolor0, movecolor0, tiles, sprites, " << std::endl;
    std::cout << "compress, interleavedata, output" << std::endl;
}

std::string getEnv(const std::string &var)
{
    const char *value = std::getenv(var.c_str());
    return value != nullptr ? value : "";
}

bool findGbalzss()
{
    std::string cmdLine;
    auto dkpPath = getEnv("DEVKITPRO");
    if (!dkpPath.empty())
    {
        // DevkitPro found. We assume the gbalzss executable is there...
#ifdef WIN32
        m_gbalzssPath = dkpPath + "\\tools\\bin\\gbalzss.exe";
        cmdLine = m_gbalzssPath;
#else
        m_gbalzssPath = dkpPath + "/tools/bin/gbalzss";
        cmdLine = m_gbalzssPath + " 2> /dev/null";
#endif
        // try to execute gbalzss and see if it works
        return WEXITSTATUS(system(cmdLine.c_str())) == 1;
    }
    else
    {
        // DevkitPro not found. See if we can call gbalzss anyway
#ifdef WIN32
        m_gbalzssPath = "gbalzss.exe";
        cmdLine = m_gbalzssPath;
#else
        m_gbalzssPath = "gbalzss";
        cmdLine = m_gbalzssPath + " 2> /dev/null";
#endif
        // try to execute gbalzss and see if it works
        return WEXITSTATUS(system(cmdLine.c_str())) == 1;
    }
}

// TODO: On Windows we might need to resolve wildcards in input files
std::pair<bool, std::vector<std::string>> resolveInputFiles(const std::vector<std::string> &files)
{
    std::pair<bool, std::vector<std::string>> result;
    result.first = true;
    for (const auto &file : files)
    {
        /*if (std::find(file.cbegin(), file.cend(), '*'))
        {
            // wildcard * found, resolve files
            std::string regexString = file;
            std::replace(regexString.begin(), regexString.end(), '*', ".*");
            try
            {
                std::regex regex(regexString, std::regex::basic);
                stdfs::
            }
            catch (const std::regex_error& ex)
            {
                std::cerr << "Bad input file " << file << ". Aborting. " << std::endl;
                return std::vector<std::string>();
            }
        }*/
        // check if file exists
        if (stdfs::exists(file))
        {
            result.second.push_back(file);
        }
        else
        {
            result.first = false;
        }
    }
    return result;
}

std::vector<uint8_t> compressLzss(const std::vector<uint8_t> &data, bool vramCompatible, bool lz11Compression)
{
    std::vector<uint8_t> result;
    // write temporary file
    const std::string tempFileName = stdfs::temp_directory_path().generic_string() + "/compress.tmp";
    std::ofstream outFile(tempFileName, std::ios::binary | std::ios::out);
    if (outFile.is_open())
    {
        outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
        outFile.close();
        // run compressor
        const std::string cmdLine = m_gbalzssPath + (vramCompatible ? " --vram" : "") + (lz11Compression ? " --lz11" : "") + " e " + tempFileName + " " + tempFileName;
        if (WEXITSTATUS(std::system(cmdLine.c_str())) == 0)
        {
            // read temporary file
            std::ifstream inFile(tempFileName, std::ios::binary | std::ios::in);
            if (inFile.is_open())
            {
                // find file size
                inFile.seekg(0, std::ios::end);
                size_t fileSize = inFile.tellg();
                inFile.seekg(0, std::ios::beg);
                // read data
                result.resize(fileSize);
                inFile.read(reinterpret_cast<char *>(result.data()), fileSize);
                inFile.close();
            }
            else
            {
                std::cout << "Failed to read temporary file." << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to run compressor." << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to write temporary file." << std::endl;
    }
    return result;
}

// Return the start index of each sub-vector in the outer vector as if all vectors were concatenated.
template <typename T>
std::vector<uint32_t> getStartIndices(const std::vector<std::vector<T>> &data)
{
    size_t currentIndex = 0;
    std::vector<uint32_t> result;
    for (const auto &current : data)
    {
        result.push_back(currentIndex);
        currentIndex += current.size();
    }
    return result;
}

// Divide every element by a certain value.
template <typename T>
std::vector<T> divideBy(const std::vector<T> &data, T divideBy = 1)
{
    std::vector<T> result;
    std::transform(data.cbegin(), data.cend(), std::back_inserter(result), [divideBy](auto t)
                   { return t / divideBy; });
    return result;
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
    if (mustCompress() && !findGbalzss())
    {
        std::cerr << "Necessary gbalzss executable not found. Aborting. " << std::endl;
        return 1;
    }
    if (mustCompress() && m_interleaveData)
    {
        std::cerr << "Compression and interleaving data does not work together. Aborting. " << std::endl;
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
    // resolve wildcards in input files and check if all input files exist
    /*auto result = resolveInputFiles(m_inFile);
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
    // fire up ImageMagick
    InitializeMagick(*argv);
    // store info about images
    auto ifIt = m_inFile.cbegin();
    ImageType imgType = ImageType::UndefinedType;
    Geometry imgSize;
    std::vector<std::vector<Color>> colorMaps;
    std::vector<std::vector<uint8_t>> imageData;
    // open first image and store type
    std::cout << "Reading " << *ifIt;
    try
    {
        Image img;
        img.read(*ifIt);
        imgType = img.type();
        imgSize = img.size();
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        if (img.classType() == PseudoClass && imgType == PaletteType)
        {
            std::cout << "paletted, " << img.colorMapSize() << " colors" << std::endl;
        }
        else if (imgType == TrueColorType)
        {
            std::cout << "true color" << std::endl;
        }
        else
        {
            std::cerr << "unsupported format. Aborting" << std::endl;
            return 1;
        }
        // if we want to convert to tiles or sprites make sure data is multiple of 8 pixels in width and height
        if ((m_asSprites || m_asTiles) && (imgType != ImageType::PaletteType || imgSize.width() % 8 != 0 || imgSize.height() % 8 != 0))
        {
            std::cerr << "Image must be paletted and width / height must be a multiple of 8. Aborting" << std::endl;
            return 1;
        }
        if (m_asSprites && (imgSize.width() % m_spriteWidth != 0 || imgSize.height() % m_spriteHeight != 0))
        {
            std::cerr << "Image width / height must be a multiple of sprite width / height. Aborting" << std::endl;
            return 1;
        }
        if (img.classType() == PseudoClass && imgType == ImageType::PaletteType)
        {
            colorMaps.push_back(getColorMap(*ifIt));
        }
        imageData.push_back(getImageData(*ifIt));
        ifIt++;
    }
    catch (const Exception &ex)
    {
        std::cerr << ". Failed to read " << *ifIt << ": " << ex.what() << std::endl;
        return 1;
    }
    // read rest of images
    while (ifIt != m_inFile.cend())
    {
        std::cout << "Reading " << *ifIt;
        Image img;
        try
        {
            img.read(*ifIt);
        }
        catch (const Exception &ex)
        {
            std::cerr << " failed: " << ex.what() << std::endl;
            return 1;
        }
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        if (img.classType() == PseudoClass && imgType == PaletteType)
        {
            std::cout << "paletted, " << img.colorMapSize() << " colors" << std::endl;
        }
        else if (imgType == TrueColorType)
        {
            std::cout << "true color" << std::endl;
        }
        else
        {
            std::cerr << "unsupported format. Aborting" << std::endl;
            return 1;
        }
        // check type and size
        if (img.type() != imgType)
        {
            std::cerr << "Image types do not match: " << *ifIt << std::endl;
            return 1;
        }
        if (img.size() != imgSize)
        {
            std::cerr << "Image sizes do not match: " << *ifIt << std::endl;
            return 1;
        }
        if (img.classType() == PseudoClass && imgType == ImageType::PaletteType)
        {
            colorMaps.push_back(getColorMap(*ifIt));
        }
        imageData.push_back(getImageData(*ifIt));
        ifIt++;
    }
    // reorder palette colors
    if (m_reorderColors)
    {
        if (imgType != ImageType::PaletteType)
        {
            std::cerr << "Reordering colors can only be done for paletted images. Aborting." << std::endl;
            return 1;
        }
        for (size_t i = 0; i < colorMaps.size(); ++i)
        {
            const auto newOrder = minimizeColorDistance(colorMaps[i]);
            colorMaps[i] = swapColors(colorMaps[i], newOrder);
            imageData[i] = swapIndices(imageData[i], newOrder);
        }
    }
    // add extra color #0 if wanted
    if (m_doAddColor0)
    {
        if (imgType != ImageType::PaletteType)
        {
            std::cerr << "Adding color #0 can only be done for paletted images. Aborting." << std::endl;
            return 1;
        }
        for (size_t i = 0; i < colorMaps.size(); ++i)
        {
            auto &cm = colorMaps[i];
            // check if the color map has one free entry
            if (cm.size() > 255)
            {
                std::cerr << "No space in color map (image has " << cm.size() << " colors). Aborting." << std::endl;
                return 1;
            }
            // add color at front of color map
            cm = addColorAtIndex0(cm, m_addColor0);
            imageData[i] = incImageIndicesBy1(imageData[i]);
        }
        std::cout << "Added " << m_addColor0String << " as color #0." << std::endl;
    }
    // move color to index #0 if wanted
    if (m_doMoveColor0)
    {
        if (imgType != ImageType::PaletteType)
        {
            std::cerr << "Moving colors can only be done for paletted images. Aborting." << std::endl;
            return 1;
        }
        for (size_t i = 0; i < colorMaps.size(); ++i)
        {
            auto &cm = colorMaps[i];
            // try to find color in palette
            auto oldColorIt = std::find(cm.begin(), cm.end(), m_moveColor0);
            if (oldColorIt == cm.end())
            {
                std::cerr << "Color " << m_moveColor0String << " not found in image color map. Aborting." << std::endl;
                return 1;
            }
            const size_t oldIndex = std::distance(cm.begin(), oldColorIt);
            // check if index needs to move
            if (oldIndex != 0)
            {
                // move index in color map and image data
                std::swap(cm[oldIndex], cm[0]);
                imageData[i] = swapIndexToIndex0(imageData[i], oldIndex);
            }
        }
        std::cout << "Moved color " << m_moveColor0String << " to index #0." << std::endl;
    }
    // check if color maps have same size
    bool allColorMapsSame = true;
    bool allColorMapsSameSize = true;
    uint32_t maxColorMapColors = 0;
    if (imgType == PaletteType && !colorMaps.empty())
    {
        const auto &refColorMap = colorMaps.front();
        maxColorMapColors = refColorMap.size();
        for (const auto &cm : colorMaps)
        {
            if (allColorMapsSameSize)
            {
                if (cm.size() != refColorMap.size())
                {
                    allColorMapsSameSize = false;
                    allColorMapsSame = false;
                }
                if (allColorMapsSame && cm != refColorMap)
                {
                    allColorMapsSame = false;
                }
            }
            if (cm.size() > maxColorMapColors)
            {
                maxColorMapColors = cm.size();
            }
        }
        // check if reasonable color count
        if (maxColorMapColors > 256)
        {
            std::cerr << "Image color map has more than 256 colors. Aborting" << std::endl;
            return 1;
        }
        // padd all color maps to max color count
        std::for_each(colorMaps.begin(), colorMaps.end(), [maxColorMapColors](auto &cm)
                      { fillUpToMultipleOf(cm, maxColorMapColors); });
        std::cerr << "Saving " << (allColorMapsSame ? 1 : colorMaps.size()) << " color map(s) with " << maxColorMapColors << " colors" << std::endl;
    }
    // we have the data. pad to multiple of 4 and compress
    std::cout << (mustCompress() ? "Compressing" : "Converting") << std::flush;
    std::vector<std::vector<uint8_t>> processedData;
    for (auto &data : imageData)
    {
        std::cout << "." << std::flush;
        if (data.empty())
        {
            std::cerr << std::endl
                      << "Empty image data" << std::endl;
            return 1;
        }
        if (m_asTiles)
        {
            data = convertToTiles(data, imgSize.width(), imgSize.height(), maxColorMapColors <= 16 ? 4 : 8);
        }
        else if (m_asSprites)
        {
            auto dataSize = imgSize;
            if (dataSize.width() != m_spriteWidth)
            {
                data = convertToWidth(data, dataSize.width(), dataSize.height(), maxColorMapColors <= 16 ? 4 : 8, m_spriteWidth);
                dataSize = Magick::Geometry(m_spriteWidth, (dataSize.width() * dataSize.height()) / m_spriteWidth);
            }
            data = convertToTiles(data, dataSize.width(), dataSize.height(), maxColorMapColors <= 16 ? 4 : 8);
        }
        if (mustCompress())
        {
            data = compressLzss(data, m_vramCompatible, m_lz11Compression);
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
    }
    std::cout << std::endl;
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
            auto nrOfBitsPerPixel = imgType == ImageType::PaletteType ? (maxColorMapColors <= 16 ? 4 : 8) : 16;
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
            hFile << "// Converted with img2h" << std::endl;
            if (mustCompress())
            {
                hFile << "// Compression type LZSS, variant " << (m_lz11Compression ? "11" : "10") << (m_vramCompatible ? ", VRAM-safe" : "") << std::endl;
            }
            hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                  << std::endl;
            // output image and palette info
            const bool storeTileOrSpriteWise = (imageData.size() == 1) && (m_asTiles || m_asSprites);
            uint32_t nrOfBytesPerImageOrSprite = imgSize.width() * imgSize.height();
            uint32_t nrOfImagesOrSprites = imageData.size();
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
            auto imageOrSpriteStartIndices = divideBy<uint32_t>(getStartIndices(processedData), 4);
            auto colorMapsStartIndices = getStartIndices(colorMaps);
            writeImageInfoToH(hFile, varName, imageData32, imgSize.width(), imgSize.height(), nrOfBytesPerImageOrSprite, imageOrSpriteStartIndices.size(), storeTileOrSpriteWise);
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