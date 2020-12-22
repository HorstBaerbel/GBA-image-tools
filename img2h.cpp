// Convert an image to a .h / .c file to compile it into a program.
// You'll need the libmagick++-dev installed!

#include "colorhelpers.h"
#include "datahelpers.h"
#include "filehelpers.h"
#include "imagehelpers.h"
#include "spritehelpers.h"

#include <iostream>
#include <string>
#include <cstdio>
#if defined(__GNUC__) || defined(__clang__)
#include <experimental/filesystem>
namespace FS_NAMESPACE = std::experimental::filesystem;
#elif defined(_MSC_VER)
#include <filesystem>
namespace FS_NAMESPACE = std::tr2::sys;
#endif

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>
using namespace Magick;

std::string m_inFile;
std::string m_outFile;
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

bool readArguments(int argc, const char *argv[])
{
    cxxopts::Options options("img2h", "Convert an image to a .h / .c file to compile it into a program");
    options.allow_unrecognised_options();
    options.add_options()("h,help", "Print help")("a,addcolor0", "Optional. Add COLOR at palette index #0 and increase all other color indices by 1. Only usable for paletted images. Color format \"abcd012\"", cxxopts::value<std::string>(m_addColor0String))("i,infile", "Input file, e.g. \"foo.png\"", cxxopts::value<std::string>())("o,outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>())("t,tiles", "Optional. Cut data into 8x8 tiles and store data tile-wise. The image needs to be paletted and its width and height must be a multiple of 8 pixels", cxxopts::value<bool>(m_asTiles))("s,sprites", "Optional. Cut data into sprites of size W x H and store data sprite- and 8x8-tile-wise. The image needs to be paletted and its width and height must be a multiple of W and H and also a multiple of 8 pixels. Sprite data is stored in \"1D mapping\" order and can be read with memcpy", cxxopts::value<std::vector<uint8_t>>(m_spriteSizes))("m,movecolor", "Optional. Move COLOR to palette index #0 and move all other colors accordingly. Only usable for paletted images. Color format \"abcd012\"", cxxopts::value<std::string>(m_moveColor0String))("positional", "", cxxopts::value<std::vector<std::string>>());
    options.parse_positional({"infile", "outname", "positional"});
    auto result = options.parse(argc, argv);
    // check if help was requested
    if (result.count("h"))
    {
        return false;
    }
    // get first positional argument as input file
    if (result.count("infile"))
    {
        // make sure the input file exists
        m_inFile = result["infile"].as<std::string>();
        if (!FS_NAMESPACE::exists(m_inFile))
        {
            std::cout << "Input file \"" << m_inFile << "\" does not exist!" << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "No input file passed!" << std::endl;
        return false;
    }
    // get second positional argument as output file / name
    if (result.count("outname"))
    {
        m_outFile = result["outname"].as<std::string>();
    }
    else
    {
        std::cout << "No output name passed!" << std::endl;
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
            std::cerr << m_addColor0String << " is not a valid color. Format must be e.g. \"--addcolor0 abc012\". Aborting. " << std::endl;
            return false;
        }
    }
    // check color moving
    if (result.count("movecolor"))
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
            std::cerr << m_moveColor0String << " is not a valid color. Format must be e.g. \"--movecolor0 abc012\". Aborting. " << std::endl;
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
    std::cout << "Convert an image to a .h / .c file to compile it into a program. Will either" << std::endl;
    std::cout << "save indices and a palette or truecolor data. All colors will be converted to" << std::endl;
    std::cout << "RGB555 directly. You might want to use ImageMagicks \"convert +remap\" before." << std::endl;
    std::cout << "Usage: img2h [OPTIONS] INFILE OUTNAME" << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "--addcolor0 COLOR: Optional. Add COLOR at palette index #0 and increase all" << std::endl;
    std::cout << "other color indices by 1. Only usable for paletted images. Format \"abcd012\"" << std::endl;
    std::cout << "--movecolor0=COLOR: Optional. Move COLOR to palette index #0 and move all" << std::endl;
    std::cout << "other colors accordingly. Only usable for paletted images. Format \"abcd012\"" << std::endl;
    std::cout << "--tiles: Optional. Cut data into 8x8 tiles and store data tile-wise. The image" << std::endl;
    std::cout << "needs to be paletted and its width and height must be a multiple of 8 pixels." << std::endl;
    std::cout << "--sprites=W,H: Optional. Cut data into sprites of size W x H and store data" << std::endl;
    std::cout << "sprite- and 8x8-tile-wise. The image needs to be paletted and its width and" << std::endl;
    std::cout << "height must be a multiple of W and H and also a multiple of 8 pixels." << std::endl;
    std::cout << "Sprite data is stored in \"1D mapping\" order and can be read with memcpy." << std::endl;
    std::cout << "INFILE: Input file, e.g. \"foo.png\"." << std::endl;
    std::cout << "OUTNAME: Output file and variable name, e.g \"foo\". This will name the output" << std::endl;
    std::cout << "files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"." << std::endl;
    std::cout << "Example: img2h foo.png foo" << std::endl;
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
        return 2;
    }
    if (m_outFile.empty())
    {
        std::cerr << "No output name passed. Aborting." << std::endl;
        return 2;
    }
    // check sprites option
    if (m_asSprites && m_spriteWidth % 8 != 0 && m_spriteHeight % 8 != 0)
    {
        std::cerr << "Sprite width and height must be a multiple of 8. Aborting." << std::endl;
        return 1;
    }
    // fire up ImageMagick
    InitializeMagick(*argv);
    // open image
    std::cout << "Reading " << m_inFile;
    ImageType imgType = ImageType::UndefinedType;
    Geometry imgSize;
    Image img;
    try
    {
        img.read(m_inFile);
        imgType = img.type();
        imgSize = img.size();
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        if (img.classType() == PseudoClass && imgType == ImageType::PaletteType)
        {
            std::cout << "paletted, " << img.colorMapSize() << " colors" << std::endl;
        }
        else if (imgType == ImageType::TrueColorType)
        {
            std::cout << "true color" << std::endl;
        }
        else
        {
            std::cerr << "unsupported format. Aborting." << std::endl;
            return 1;
        }
    }
    catch (const Exception &ex)
    {
        std::cerr << ". Failed to read " << m_inFile << ": " << ex.what() << std::endl;
        return 1;
    }
    // if we want to convert to tiles or sprites make sure data is multiple of 8 pixels in width
    if ((m_asSprites || m_asTiles) && (imgType != ImageType::PaletteType || imgSize.width() % 8 != 0 || imgSize.height() % 8 != 0))
    {
        std::cerr << "Image must be paletted and width / height must be a multiple of 8. Aborting." << std::endl;
        return 1;
    }
    if (m_asSprites && (imgSize.width() % m_spriteWidth != 0 || imgSize.height() % m_spriteHeight != 0))
    {
        std::cerr << "Image width / height must be a multiple of sprite width / height. Aborting." << std::endl;
        return 1;
    }
    // read image data
    std::vector<uint8_t> imageData = getImageData(img);
    // read color map
    std::vector<Color> colorMap;
    if (img.classType() == PseudoClass && imgType == ImageType::PaletteType)
    {
        colorMap = getColorMap(img);
    }
    // add extra color #0 if wanted
    if (m_doAddColor0)
    {
        if (img.classType() != PseudoClass || imgType != ImageType::PaletteType)
        {
            std::cerr << "Adding color #0 can only be done for paletted images. Aborting." << std::endl;
            return 1;
        }
        // check if the color map has one free entry
        if (colorMap.size() > 255)
        {
            std::cerr << "No space in color map (image has " << colorMap.size() << " colors). Aborting." << std::endl;
            return 1;
        }
        // add color at front of color map and increase all color index values by 1
        colorMap = addColorAtIndex0(colorMap, m_addColor0);
        imageData = incImageIndicesBy1(imageData);
        std::cout << "Added " << m_addColor0String << " as color #0. Image now has " << colorMap.size() << " colors." << std::endl;
    }
    // move color to index #0 if wanted
    if (m_doMoveColor0)
    {
        if (img.classType() != PseudoClass || imgType != ImageType::PaletteType)
        {
            std::cerr << "Moving colors can only be done for paletted images. Aborting." << std::endl;
            return 1;
        }
        // try to find color in palette
        auto oldColorIt = std::find(colorMap.begin(), colorMap.end(), m_moveColor0);
        if (oldColorIt == colorMap.end())
        {
            std::cerr << "Color " << m_moveColor0String << " not found in image color map. Aborting." << std::endl;
            return 1;
        }
        const size_t oldIndex = std::distance(colorMap.begin(), oldColorIt);
        // check if index needs to move
        if (oldIndex == 0)
        {
            std::cout << "Color " << m_moveColor0String << " already at index #0. Skipping." << std::endl;
        }
        else
        {
            // move index in color map and image data
            std::swap(colorMap[oldIndex], colorMap[0]);
            imageData = swapIndexToIndex0(imageData, oldIndex);
        }
        std::cout << "Moved color " << m_addColor0String << " to index #0." << std::endl;
    }
    if (m_asTiles)
    {
        imageData = convertToTiles(imageData, imgSize.width(), imgSize.height(), colorMap.size() <= 16 ? 4 : 8);
        std::cout << "Converted to 8x8 tile data." << std::endl;
    }
    else if (m_asSprites)
    {
        if (imgSize.width() != m_spriteWidth)
        {
            imageData = convertToWidth(imageData, imgSize.width(), imgSize.height(), colorMap.size() <= 16 ? 4 : 8, m_spriteWidth);
            imgSize = Magick::Geometry(m_spriteWidth, (imgSize.width() * imgSize.height()) / m_spriteWidth);
            std::cout << "Converted to " << m_spriteWidth << "x" << m_spriteHeight << " sprite data." << std::endl;
        }
        imageData = convertToTiles(imageData, imgSize.width(), imgSize.height(), colorMap.size() <= 16 ? 4 : 8);
        std::cout << "Converted to 8x8 tile data." << std::endl;
    }
    fillUpToMultipleOf(imageData, 4);
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
            std::transform(varName.begin(), varName.end(), varName.begin(), [](char c) { return std::toupper(c, std::locale()); });
            // output header
            hFile << "// Converted with img2h" << std::endl;
            hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                  << std::endl;
            // output image and palette info
            uint32_t nrOfBytesPerImageOrSprite = imgSize.width() * imgSize.height();
            uint32_t nrOfImagesOrSprites = 1;
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
            nrOfBytesPerImageOrSprite = imgType == ImageType::PaletteType ? (colorMap.size() <= 16 ? (nrOfBytesPerImageOrSprite / 2) : nrOfBytesPerImageOrSprite) : (nrOfBytesPerImageOrSprite * 2);
            // convert image data to uint32_ts and palette to BGR555 uint16_ts
            auto imageData32 = convertTo<uint32_t>(imageData);
            auto paletteData16 = convertToBGR555(colorMap);
            writeImageInfoToH(hFile, varName, imageData32, imgSize.width(), imgSize.height(), nrOfBytesPerImageOrSprite, nrOfImagesOrSprites, m_asTiles || m_asSprites);
            if (imgType == ImageType::PaletteType)
            {
                writePaletteInfoToHeader(hFile, varName, paletteData16, colorMap.size(), true, m_asTiles || m_asSprites);
            }
            hFile << std::endl;
            hFile.close();
            // output image and palette data
            writeImageDataToC(cFile, varName, baseName, imageData32, std::vector<uint32_t>(), m_asTiles || m_asSprites);
            if (imgType == ImageType::PaletteType)
            {
                writePaletteDataToC(cFile, varName, paletteData16, std::vector<uint32_t>(), m_asTiles || m_asSprites);
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
    return 0;
}