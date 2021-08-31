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
#include "imageprocessing.h"
#include "processingoptions.h"
#include "lzsshelpers.h"
#include "spritehelpers.h"
#include "exception.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>

#include "cxxopts/include/cxxopts.hpp"
#include <Magick++.h>

std::vector<std::string> m_inFile;
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
    cxxopts::Options opts("img2h", "Convert and compress a list images to a .h / .c file to compile it into a program");
    opts.allow_unrecognised_options();
    opts.add_option("", {"h,help", "Print help"});
    opts.add_option("", {"i,infile", "Input file(s), e.g. \"foo.png\"", cxxopts::value<std::vector<std::string>>()});
    opts.add_option("", {"o,outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
    opts.add_option("", options.reorderColors.cxxOption);
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
    opts.add_option("", options.vram.cxxOption);
    opts.add_option("", options.interleavePixels.cxxOption);
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
    return options.addColor0.parse(result) && options.moveColor0.parse(result) &&
           options.shiftIndices.parse(result) && options.sprites.parse(result);
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
    std::cout << options.reorderColors.helpString() << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << options.interleavePixels.helpString() << std::endl;
    std::cout << "COMPRESSION options (all optional):" << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << options.lz11.helpString() << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "Valid combinations are e.g. \"--diff8 --lz10\" or \"--lz11 --vram\"." << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: can be a file list and/or can have * as a wildcard. Multiple input " << std::endl;
    std::cout << "images MUST have the same type (palette / true color) and resolution!" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "EXECUTION ORDER: input, reordercolors, addcolor0, movecolor0, shift, prune," << std::endl;
    std::cout << "sprites, tiles, diff8 / diff16, lz10 / lz11, interleavepixels, output" << std::endl;
}

std::tuple<ImageType, Geometry, std::vector<ImageProcessing::Data>> readImages(const std::vector<std::string> &fileNames, const ProcessingOptions &options)
{
    ImageType imgType = Magick::ImageType::UndefinedType;
    Geometry imgSize;
    std::vector<ImageProcessing::Data> images;
    // open first image and store type
    auto ifIt = fileNames.cbegin();
    while (ifIt != fileNames.cend())
    {
        std::cout << "Reading " << *ifIt;
        Image img;
        try
        {
            img.read(*ifIt);
        }
        catch (const Exception &ex)
        {
            THROW(std::runtime_error, "Failed to read image: " << ex.what());
        }
        imgSize = img.size();
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        imgType = img.type();
        const bool isPaletted = img.classType() == ClassType::PseudoClass && imgType == Magick::ImageType::PaletteType;
        if (isPaletted)
        {
            std::cout << "paletted, " << img.colorMapSize() << " colors" << std::endl;
        }
        else if (imgType == Magick::ImageType::TrueColorType)
        {
            std::cout << "true color" << std::endl;
        }
        else
        {
            THROW(std::runtime_error, "Unsupported image format");
        }
        // compare size and type to first image to make sure all images have the same format
        if (images.size() > 0)
        {
            // check type and size
            REQUIRE(images.front().type == imgType, std::runtime_error, "Image types do not match");
            REQUIRE(images.front().size == imgSize, std::runtime_error, "Image sizes do not match");
        }
        // if we want to convert to tiles or sprites make sure data is multiple of 8 pixels in width and height
        if ((options.sprites || options.tiles) && (!isPaletted || imgSize.width() % 8 != 0 || imgSize.height() % 8 != 0))
        {
            THROW(std::runtime_error, "Image must be paletted and width / height must be a multiple of 8");
        }
        if (options.sprites && (imgSize.width() % options.sprites.value.front() != 0 || imgSize.height() % options.sprites.value.back() != 0))
        {
            THROW(std::runtime_error, "Image width / height must be a multiple of sprite width / height");
        }
        ImageProcessing::Data entry{*ifIt, imgType, imgSize, (isPaletted ? 8U : 16U), getImageData(img), (isPaletted ? getColorMap(img) : std::vector<Magick::Color>())};
        images.push_back(entry);
        ifIt++;
    }
    return {imgType, imgSize, images};
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
        // fire up ImageMagick
        InitializeMagick(*argv);
        // read image(s) from disk
        auto [imgType, imgSize, images] = readImages(m_inFile, options);
        // build processing pipeline
        ImageProcessing processing;
        if (options.reorderColors)
        {
            processing.addStep(ImageProcessing::Type::ReorderColors);
        }
        if (options.addColor0)
        {
            processing.addStep(ImageProcessing::Type::AddColor0, ImageProcessing::Parameter(options.addColor0.value));
        }
        if (options.moveColor0)
        {
            processing.addStep(ImageProcessing::Type::MoveColor0, ImageProcessing::Parameter(options.moveColor0.value));
        }
        if (options.shiftIndices)
        {
            processing.addStep(ImageProcessing::Type::ShiftIndices, ImageProcessing::Parameter(options.shiftIndices.value));
        }
        if (imgType == Magick::ImageType::PaletteType)
        {
            processing.addStep(ImageProcessing::Type::EqualizeColorMaps);
        }
        if (options.pruneIndices)
        {
            processing.addStep(ImageProcessing::Type::PruneIndices);
        }
        if (options.sprites)
        {
            processing.addStep(ImageProcessing::Type::ConvertSprites, ImageProcessing::Parameter(options.sprites.value.front()));
        }
        if (options.tiles)
        {
            processing.addStep(ImageProcessing::Type::ConvertTiles);
        }
        if (options.delta8)
        {
            processing.addStep(ImageProcessing::Type::ConvertDelta8);
        }
        if (options.delta16)
        {
            processing.addStep(ImageProcessing::Type::ConvertDelta16);
        }
        if (options.lz10)
        {
            processing.addStep(ImageProcessing::Type::CompressLz10, ImageProcessing::Parameter(options.vram.isSet));
        }
        if (options.lz11)
        {
            processing.addStep(ImageProcessing::Type::CompressLz11, ImageProcessing::Parameter(options.vram.isSet));
        }
        processing.addStep(ImageProcessing::Type::PadImageData, ImageProcessing::Parameter(uint32_t(4)));
        // apply image processing pipeline
        const auto processingDescription = processing.getProcessingDescription();
        std::cout << "Applying processing: " << processingDescription << (options.interleavePixels ? ", interleave pixels" : "") << std::endl;
        images = processing.process(images);
        // check if all color maps are the same
        bool allColorMapsSame = true;
        uint32_t maxColorMapColors = 0;
        if (imgType == Magick::ImageType::PaletteType)
        {
            if (images.size() == 1)
            {
                maxColorMapColors = images.front().colorMap.size();
            }
            else
            {
                allColorMapsSame = std::find_if_not(images.cbegin(), images.cend(), [&refColorMap = images.front().colorMap](const auto &img)
                                                    { return img.colorMap == refColorMap; }) == images.cend();
                maxColorMapColors = std::max_element(images.cbegin(), images.cend(), [](const auto &imgA, const auto &imgB)
                                                     { return imgA.colorMap.size() < imgB.colorMap.size(); })
                                        ->colorMap.size();
            }
            std::cout << "Saving " << (allColorMapsSame ? 1 : images.size()) << " color map(s) with " << maxColorMapColors << " colors" << std::endl;
        }
        // open output files
        std::ofstream hFile(m_outFile + ".h", std::ios::out);
        std::ofstream cFile(m_outFile + ".c", std::ios::out);
        if (hFile.is_open() && cFile.is_open())
        {
            std::cout << "Writing output files " << m_outFile << ".h, " << m_outFile << ".c" << std::endl;
            try
            {
                // build output file / variable name
                std::string baseName = getBaseNameFromFilePath(m_outFile);
                std::string varName = baseName;
                std::transform(varName.begin(), varName.end(), varName.begin(), [](char c)
                               { return std::toupper(c, std::locale()); });
                // output header
                hFile << "// Converted with img2h " << getCommandLine(argc, argv) << std::endl;
                hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                      << std::endl;
                // output image and palette info
                const bool storeTileOrSpriteWise = (images.size() == 1) && (options.tiles || options.sprites);
                uint32_t nrOfBytesPerImageOrSprite = imgSize.width() * imgSize.height();
                uint32_t nrOfImagesOrSprites = images.size();
                if (nrOfImagesOrSprites == 1)
                {
                    // if we have a single input image, store data per tile or sprite
                    if (options.sprites)
                    {
                        // calculate number of w*h sprites
                        auto spriteWidth = options.sprites.value.front();
                        auto spriteHeight = options.sprites.value.back();
                        nrOfImagesOrSprites = (imgSize.width() * imgSize.height()) / (spriteWidth * spriteHeight);
                        nrOfBytesPerImageOrSprite = spriteWidth * spriteHeight;
                        imgSize = Magick::Geometry(spriteWidth, spriteHeight);
                    }
                    else if (options.tiles)
                    {
                        // calculate number of 8*8 pixel tiles
                        nrOfImagesOrSprites = (imgSize.width() * imgSize.height()) / 64;
                        nrOfBytesPerImageOrSprite = 64;
                        imgSize = Magick::Geometry(8, 8);
                    }
                }
                nrOfBytesPerImageOrSprite = imgType == Magick::ImageType::PaletteType ? (maxColorMapColors <= 16 ? (nrOfBytesPerImageOrSprite / 2) : nrOfBytesPerImageOrSprite) : (nrOfBytesPerImageOrSprite * 2);
                // convert image data to uint32_ts and palette to BGR555 uint16_ts
                auto [imageData32, imageOrSpriteStartIndices] = ImageProcessing::combineImageData<uint32_t>(images, options.interleavePixels);
                // make sure we have the correct number of images. sprites and tiles will have no start indices, thus we need to use nrOfImagesOrSprites
                nrOfImagesOrSprites = imageOrSpriteStartIndices.size() > 1 ? imageOrSpriteStartIndices.size() : nrOfImagesOrSprites;
                // output image and palette data
                writeImageInfoToH(hFile, varName, imageData32, imgSize.width(), imgSize.height(), nrOfBytesPerImageOrSprite, nrOfImagesOrSprites, storeTileOrSpriteWise);
                writeImageDataToC(cFile, varName, baseName, imageData32, imageOrSpriteStartIndices, storeTileOrSpriteWise);
                if (imgType == Magick::ImageType::PaletteType)
                {
                    auto [paletteData16, colorMapsStartIndices] = (allColorMapsSame ? std::make_pair(convertToBGR555(images.front().colorMap), std::vector<uint32_t>()) : ImageProcessing::combineColorMaps<uint16_t>(images, [](auto cm)
                                                                                                                                                                                                                      { return convertToBGR555(cm); }));
                    writePaletteInfoToHeader(hFile, varName, paletteData16, maxColorMapColors, allColorMapsSame || colorMapsStartIndices.size() <= 1, storeTileOrSpriteWise);
                    writePaletteDataToC(cFile, varName, paletteData16, colorMapsStartIndices, storeTileOrSpriteWise);
                }
                hFile << std::endl;
                hFile.close();
                cFile.close();
            }
            catch (const std::runtime_error &e)
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