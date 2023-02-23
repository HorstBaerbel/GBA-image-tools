#include "color/colorhelpers.h"
#include "compression/lzss.h"
#include "processing/datahelpers.h"
#include "exception.h"
#include "io/textio.h"
#include "io/imageio.h"
#include "processing/imagehelpers.h"
#include "processing/imageprocessing.h"
#include "processing/processingoptions.h"
#include "processing/spritehelpers.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <filesystem>

#include "cxxopts/include/cxxopts.hpp"
#include "glob/single_include/glob/glob.hpp"
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
    try
    {
        cxxopts::Options opts("img2h", "Convert and compress a list images to a .h / .c file to compile it into a program");
        opts.add_option("", {"h,help", "Print help"});
        opts.add_option("", {"infile", "Input file(s), e.g. \"foo.png\"", cxxopts::value<std::vector<std::string>>()});
        opts.add_option("", {"outname", "Output file and variable name, e.g \"foo\". This will name the output files \"foo.h\" and \"foo.c\" and variable names will start with \"FOO_\"", cxxopts::value<std::string>()});
        opts.add_option("", options.blackWhite.cxxOption);
        opts.add_option("", options.paletted.cxxOption);
        opts.add_option("", options.commonPalette.cxxOption);
        opts.add_option("", options.truecolor.cxxOption);
        opts.add_option("", options.reorderColors.cxxOption);
        opts.add_option("", options.addColor0.cxxOption);
        opts.add_option("", options.moveColor0.cxxOption);
        opts.add_option("", options.shiftIndices.cxxOption);
        opts.add_option("", options.pruneIndices.cxxOption);
        opts.add_option("", options.sprites.cxxOption);
        opts.add_option("", options.tiles.cxxOption);
        opts.add_option("", options.tilemap.cxxOption);
        opts.add_option("", options.delta8.cxxOption);
        opts.add_option("", options.delta16.cxxOption);
        // opts.add_option("", options.rle.cxxOption);
        opts.add_option("", options.lz10.cxxOption);
        opts.add_option("", options.lz11.cxxOption);
        opts.add_option("", options.vram.cxxOption);
        opts.add_option("", options.dryRun.cxxOption);
        opts.add_option("", options.dumpResults.cxxOption);
        opts.add_option("", options.interleavePixels.cxxOption);
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
            // resolve wildcards in input files
            auto filePaths = glob::glob(m_inFile);
            m_inFile.clear();
            std::transform(filePaths.cbegin(), filePaths.cend(), std::back_inserter(m_inFile), [](const auto &p)
                           { return p.string(); });
            // make sure all input files exist
            for (const auto &fileName : m_inFile)
            {
                if (!std::filesystem::exists(fileName))
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
        // check if exclusive options set
        options.blackWhite.parse(result);
        options.paletted.parse(result);
        options.commonPalette.parse(result);
        options.truecolor.parse(result);
        if ((options.blackWhite + options.paletted + options.commonPalette + options.truecolor) == 0)
        {
            std::cerr << "One format option is needed." << std::endl;
            return false;
        }
        if ((options.blackWhite + options.paletted + options.commonPalette + options.truecolor) > 1)
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
        options.tilemap.parse(result);
        // if tilemap is set, also set tiles
        if (options.tilemap)
        {
            options.tiles.isSet = true;
        }
    }
    catch (const cxxopts::exceptions::parsing &e)
    {
        std::cerr << "In command line: " << getCommandLine(argc, argv) << std::endl;
        std::cerr << "Argument error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void printUsage()
{
    // 80 chars:  --------------------------------------------------------------------------------
    std::cout << "Convert a (list of) image files to a .c and .h file to compile them into a" << std::endl;
    std::cout << "GBA executable." << std::endl;
    std::cout << "Will either save indices and a palette or truecolor data." << std::endl;
    std::cout << "Usage: img2h FORMAT [GENERAL] [CONVERT] [COMPRESS] INFILE [INFILEn...] OUTNAME" << std::endl;
    std::cout << "FORMAT options (mutually exclusive):" << std::endl;
    std::cout << options.blackWhite.helpString() << std::endl;
    std::cout << options.paletted.helpString() << std::endl;
    std::cout << options.commonPalette.helpString() << std::endl;
    std::cout << options.truecolor.helpString() << std::endl;
    std::cout << "CONVERT options (all optional):" << std::endl;
    std::cout << options.reorderColors.helpString() << std::endl;
    std::cout << options.addColor0.helpString() << std::endl;
    std::cout << options.moveColor0.helpString() << std::endl;
    std::cout << options.shiftIndices.helpString() << std::endl;
    std::cout << options.pruneIndices.helpString() << std::endl;
    std::cout << options.tiles.helpString() << std::endl;
    std::cout << options.tilemap.helpString() << std::endl;
    std::cout << options.sprites.helpString() << std::endl;
    std::cout << options.delta8.helpString() << std::endl;
    std::cout << options.delta16.helpString() << std::endl;
    std::cout << options.interleavePixels.helpString() << std::endl;
    std::cout << "COMPRESS options (mutually exclusive):" << std::endl;
    // std::cout << options.rle.helpString() << std::endl;
    std::cout << options.lz10.helpString() << std::endl;
    std::cout << options.lz11.helpString() << std::endl;
    std::cout << "COMPRESS modifiers (optional):" << std::endl;
    std::cout << options.vram.helpString() << std::endl;
    std::cout << "Valid combinations are e.g. \"--rle --lz10\" or \"--lz11 --vram\"." << std::endl;
    std::cout << "You must have DevkitPro installed or the gbalzss executable must be in PATH." << std::endl;
    std::cout << "INFILE: can be a file list and/or can have * as a wildcard. Multiple input " << std::endl;
    std::cout << "images MUST have the same type (palette / true color) and resolution!" << std::endl;
    std::cout << "OUTNAME: is determined from the first non-existant file path. It can be an " << std::endl;
    std::cout << "absolute or relative file path or a file base name. Two files OUTNAME.h and " << std::endl;
    std::cout << "OUTNAME.c will be generated. All variables will begin with the base name " << std::endl;
    std::cout << "portion of OUTNAME." << std::endl;
    std::cout << "MISC options (all optional):" << std::endl;
    std::cout << options.dryRun.helpString() << std::endl;
    std::cout << options.dumpResults.helpString() << std::endl;
    std::cout << "help: Show this help." << std::endl;
    std::cout << "ORDER: input, reordercolors, addcolor0, movecolor0, shift, prune, sprites" << std::endl;
    std::cout << "tiles, tilemap, delta8 / delta16, rle, lz10 / lz11, interleavepixels, output" << std::endl;
}

std::vector<Image::InputData> readImages(const std::vector<std::string> &fileNames, const ProcessingOptions &options)
{
    Magick::ImageType commonImgType = Magick::ImageType::UndefinedType;
    Image::DataSize commonImgSize = {0, 0};
    std::vector<Image::InputData> images;
    // open first image and store type
    auto ifIt = fileNames.cbegin();
    while (ifIt != fileNames.cend())
    {
        std::cout << "Reading " << *ifIt;
        Magick::Image img;
        try
        {
            img.read(*ifIt);
        }
        catch (const Magick::Exception &ex)
        {
            THROW(std::runtime_error, "Failed to read image: " << ex.what());
        }
        Image::DataSize imgSize = {img.size().width(), img.size().height()};
        std::cout << " -> " << imgSize.width() << "x" << imgSize.height() << ", ";
        Magick::ImageType imgType = img.type();
        const bool isPaletted = img.classType() == Magick::ClassType::PseudoClass && imgType == Magick::ImageType::PaletteType;
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
        if (images.size() == 0)
        {
            // set type and size of first image
            commonImgType = imgType;
            commonImgSize = imgSize;
        }
        else
        {
            // check type and size
            REQUIRE(commonImgType == imgType, std::runtime_error, "Image types do not match");
            REQUIRE(commonImgSize == imgSize, std::runtime_error, "Image sizes do not match");
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
        images.push_back({static_cast<uint32_t>(std::distance(fileNames.cbegin(), ifIt)), *ifIt, img});
        ifIt++;
    }
    return images;
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
            std::cerr << "No input file(s) passed. Aborting." << std::endl;
            return 1;
        }
        if (m_outFile.empty())
        {
            std::cerr << "No output file passed. Aborting." << std::endl;
            return 1;
        }
        // fire up ImageMagick
        Magick::InitializeMagick(*argv);
        // read image(s) from disk
        auto images = readImages(m_inFile, options);
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
        else if (options.commonPalette)
        {
            // add common palette conversion using GBA RGB555 reference color map
            processing.addStep(Image::ProcessingType::InputCommonPalette, {buildColorMapRGB555(), options.commonPalette.value});
        }
        else if (options.truecolor)
        {
            processing.addStep(Image::ProcessingType::InputTruecolor, {options.truecolor.value});
        }
        // build processing pipeline - conversion
        if (options.reorderColors)
        {
            processing.addStep(Image::ProcessingType::ReorderColors, {});
        }
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
        if (options.paletted || options.commonPalette)
        {
            if (images.size() > 1)
            {
                processing.addStep(Image::ProcessingType::EqualizeColorMaps, {});
            }
            processing.addStep(Image::ProcessingType::ConvertColorMap, {Color::Format::RGB555});
            processing.addStep(Image::ProcessingType::PadColorMapData, {uint32_t(4)});
        }
        if (options.pruneIndices)
        {
            processing.addStep(Image::ProcessingType::PruneIndices, {options.pruneIndices.value});
        }
        if (options.sprites)
        {
            processing.addStep(Image::ProcessingType::ConvertSprites, {options.sprites.value.front()});
        }
        if (options.tiles)
        {
            processing.addStep(Image::ProcessingType::ConvertTiles, {});
        }
        if (options.tilemap)
        {
            processing.addStep(Image::ProcessingType::BuildTileMap, {options.tilemap.value});
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
            processing.addStep(Image::ProcessingType::CompressRLE, {});
        }*/
        if (options.lz10)
        {
            processing.addStep(Image::ProcessingType::CompressLz10, {options.vram.isSet});
        }
        if (options.lz11)
        {
            processing.addStep(Image::ProcessingType::CompressLz11, {options.vram.isSet});
        }
        processing.addStep(Image::ProcessingType::PadImageData, {uint32_t(4)}, {});
        // apply image processing pipeline
        const auto processingDescription = processing.getProcessingDescription();
        std::cout << "Applying processing: " << processingDescription << (options.interleavePixels ? ", interleave pixels" : "") << std::endl;
        auto data = processing.processBatch(images);
        // get info from data
        auto dataSize = data.front().size;
        const bool dataIsPaletted = Image::isPaletted(data.front());
        // check if all color maps are the same
        bool allColorMapsSame = true;
        uint32_t maxColorMapColors = 0;
        if (dataIsPaletted)
        {
            if (data.size() == 1)
            {
                maxColorMapColors = data.front().colorMap.size();
            }
            else
            {
                allColorMapsSame = std::find_if_not(data.cbegin(), data.cend(), [&refColorMap = data.front().colorMap](const auto &img)
                                                    { return img.colorMap == refColorMap; }) == data.cend();
                maxColorMapColors = std::max_element(data.cbegin(), data.cend(), [](const auto &imgA, const auto &imgB)
                                                     { return imgA.colorMap.size() < imgB.colorMap.size(); })
                                        ->colorMap.size();
            }
            std::cout << "Saving " << (allColorMapsSame ? 1 : data.size()) << " color map(s) with " << maxColorMapColors << " colors" << std::endl;
        }
        // now dump conversion results
        if (options.dumpResults)
        {
            auto dumpPath = std::filesystem::current_path() / "result";
            IO::File::writeImages(dumpPath.c_str(), data);
        }
        // open output files
        if (!options.dryRun)
        {
            std::ofstream hFile(m_outFile + ".h", std::ios::out);
            std::ofstream cFile(m_outFile + ".c", std::ios::out);
            if (hFile.is_open() && cFile.is_open())
            {
                std::cout << "Writing output files " << m_outFile << ".h, " << m_outFile << ".c" << std::endl;
                try
                {
                    // build output file / variable name
                    std::string baseName = std::filesystem::path(m_outFile).filename().replace_extension("");
                    std::string varName = baseName;
                    std::transform(varName.begin(), varName.end(), varName.begin(), [](char c)
                                   { return std::toupper(c, std::locale()); });
                    // output header
                    hFile << "// Converted with img2h " << getCommandLine(argc, argv) << std::endl;
                    hFile << "// Note that the _Alignas specifier will need C11, as a workaround use __attribute__((aligned(4)))" << std::endl
                          << std::endl;
                    // output image and palette info
                    const bool storeTileOrSpriteWise = (data.size() == 1) && (options.tiles || options.sprites);
                    uint32_t nrOfBytesPerImageOrSprite = dataSize.width() * dataSize.height();
                    uint32_t nrOfImagesOrSprites = data.size();
                    if (nrOfImagesOrSprites == 1)
                    {
                        // if we have a single input image, store data per tile or sprite
                        if (options.sprites)
                        {
                            // calculate number of w*h sprites
                            auto spriteWidth = options.sprites.value.front();
                            auto spriteHeight = options.sprites.value.back();
                            nrOfImagesOrSprites = (dataSize.width() * dataSize.height()) / (spriteWidth * spriteHeight);
                            nrOfBytesPerImageOrSprite = spriteWidth * spriteHeight;
                            dataSize = {spriteWidth, spriteHeight};
                        }
                        else if (options.tiles)
                        {
                            // calculate number of 8*8 pixel tiles
                            nrOfImagesOrSprites = (dataSize.width() * dataSize.height()) / 64;
                            nrOfBytesPerImageOrSprite = 64;
                            dataSize = {8, 8};
                        }
                    }
                    nrOfBytesPerImageOrSprite = dataIsPaletted ? (maxColorMapColors <= 16 ? (nrOfBytesPerImageOrSprite / 2) : nrOfBytesPerImageOrSprite) : (nrOfBytesPerImageOrSprite * 2);
                    // convert image data to uint32_ts and palette to BGR555 uint16_ts
                    auto [imageData32, imageOrSpriteStartIndices] = Image::Processing::combineImageData<uint32_t>(data, options.interleavePixels);
                    // make sure we have the correct number of images. sprites and tiles will have no start indices, thus we need to use nrOfImagesOrSprites
                    nrOfImagesOrSprites = imageOrSpriteStartIndices.size() > 1 ? imageOrSpriteStartIndices.size() : nrOfImagesOrSprites;
                    // output image and palette data
                    if (options.tilemap)
                    {
                        // convert map data to uint32_ts
                        auto [mapData32, mapStartIndices] = Image::Processing::combineMapData<uint32_t>(data);
                        IO::Text::writeImageInfoToH(hFile, varName, imageData32, mapData32, dataSize.width(), dataSize.height(), nrOfBytesPerImageOrSprite, nrOfImagesOrSprites, storeTileOrSpriteWise);
                        IO::Text::writeImageDataToC(cFile, varName, baseName, imageData32, imageOrSpriteStartIndices, mapData32, storeTileOrSpriteWise);
                    }
                    else
                    {
                        IO::Text::writeImageInfoToH(hFile, varName, imageData32, {}, dataSize.width(), dataSize.height(), nrOfBytesPerImageOrSprite, nrOfImagesOrSprites, storeTileOrSpriteWise);
                        IO::Text::writeImageDataToC(cFile, varName, baseName, imageData32, imageOrSpriteStartIndices, {}, storeTileOrSpriteWise);
                    }
                    if (dataIsPaletted)
                    {
                        auto [paletteData16, colorMapsStartIndices] = (allColorMapsSame ? std::make_pair(convertToBGR555(data.front().colorMap), std::vector<uint32_t>()) : Image::Processing::combineColorMaps<uint16_t>(data, [](auto cm)
                                                                                                                                                                                                                          { return convertToBGR555(cm); }));
                        IO::Text::writePaletteInfoToHeader(hFile, varName, paletteData16, maxColorMapColors, allColorMapsSame || colorMapsStartIndices.size() <= 1, storeTileOrSpriteWise);
                        IO::Text::writePaletteDataToC(cFile, varName, paletteData16, colorMapsStartIndices, storeTileOrSpriteWise);
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