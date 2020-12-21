// Move a color in the images color map to a different index in the color map
// You'll need the libmagick++-dev installed!

#include "helpers.h"
#include <iostream>
#include <string>
#if defined(__GNUC__) || defined(__clang__)
    #include <experimental/filesystem>
    namespace FS_NAMESPACE = std::experimental::filesystem;
#elif defined(_MSC_VER)
    #include <filesystem>
    namespace FS_NAMESPACE = std::tr2::sys;
#endif

#include <Magick++.h>
using namespace Magick;

std::vector<std::string> m_inFiles;
std::string m_outFile;
Color m_color;
size_t m_newIndex;

bool readArguments(int argc, const char *argv[])
{
    bool pastColor = false;
    bool pastIndex = false;
    bool pastInput = false;
    bool pastOutput = false;
    for (int i = 1; i < argc; ++i)
    {
        // read argument from list
        const std::string argument = argv[i];
        if (!pastColor)
        {
            // try converting argument to a color
            std::string colorArg = argument;
            colorArg = std::string("#") + colorArg;
            try
            {
                m_color = Color(colorArg);
                pastColor = true;
                continue;
            }
            catch (const Exception &ex)
            {
                std::cerr << colorArg << " is not a valid color. Format must be e.g. \"#abc123\". Aborting. " << std::endl;
                return false;
            }
            pastColor = true;
        }
        if (pastColor && !pastIndex)
        {
            // try converting argument to an integer
            try
            {
                m_newIndex = std::stoul(argument);
                if (m_newIndex > 255)
                {
                    std::cerr << argument << " is out of range (0,255). Aborting." << std::endl;
                    return false;
                }
                pastIndex = true;
                continue;
            }
            catch (const std::invalid_argument &ex)
            {
                std::cerr << argument << " is not a valid index. Aborting." << std::endl;
                return false;
            }
            catch (const std::out_of_range &ex)
            {
                std::cerr << argument << " is out of range (0,255). Aborting." << std::endl;
                return false;
            }
            pastIndex = true;
        }
        // check if this is an input or output file
        if (pastColor && pastIndex && !pastInput)
        {
            // add everything except the last file name to input files
            if (i < (argc - 1))
            {
                // check if file exists
                if (FS_NAMESPACE::exists(argument))
                {
                    m_inFiles.push_back(argument);
                    continue;
                }
            }
            // we stopped adding input file. must be pas input files
            pastInput = true;
        }
        if (pastColor && pastIndex && pastInput && !pastOutput)
        {
            m_outFile = argument;
            pastOutput = true;
            return true;
        }
    }
    return false;
}

void printUsage()
{
    std::cout << "Move a color in the color map of an image to a specific index." << std::endl;
    std::cout << "Usage: movecolor COLOR INDEX INFILE OUTFILE" << std::endl;
    std::cout << "COLOR: RGB color in hex format, e.g. \"abc012\"." << std::endl;
    std::cout << "INDEX: New index in color map [0,255]." << std::endl;
    std::cout << "INFILE: Input file(s). Can have wildcards, e.g. \"foo*.png\"." << std::endl;
    std::cout << "OUTFILE: The first non-existant file is used as the output file. The file name" << std::endl;
    std::cout << "can have printf-style formatting, e.g. \"foo%02d.png\", which will append a" << std::endl;
    std::cout << "number in the range 00-99 to the file name." << std::endl;
    std::cout << "Example: movecolor ff00ff 0 foo*.png bar%03.png" << std::endl;
}

int main(int argc, const char *argv[])
{
    // check arguments
    if (argc != 5 || !readArguments(argc, argv))
    {
        printUsage();
        return -1;
    }
    // fire up ImageMagick
    InitializeMagick(*argv);
    // open image
    bool errorOccurred = false;
    for (uint32_t i = 0; i < m_inFiles.size(); ++i)
    {
        auto inFile = m_inFiles.at(i);
        std::cout << "Reading " << inFile << std::endl;
        Image img;
        try
        {
            img.read(inFile);
        }
        catch (const Exception &ex)
        {
            std::cerr << "Failed to read " << inFile << ": " << ex.what() << std::endl;
            errorOccurred = true;
            continue;
        }
        // check if paletted image
        if (img.classType() != PseudoClass || img.type() != PaletteType)
        {
            std::cerr << "Only paletted images are supported! Aborting." << std::endl;
            errorOccurred = true;
            continue;
        }
        // read palette
        auto colorMap = getColorMap(img);
        // try to find color in palette
        auto oldColorIt = std::find(colorMap.cbegin(), colorMap.cend(), m_color);
        if (oldColorIt == colorMap.cend())
        {
            std::cerr << "Color " << inFile << " not found in image color map. Aborting." << std::endl;
            errorOccurred = true;
            continue;
        }
        const size_t oldIndex = std::distance(colorMap.cbegin(), oldColorIt);
        // check if index needs to move
        if (oldIndex == m_newIndex)
        {
            std::cout << "Color " << inFile << " already at index " << oldIndex << ". Quitting." << std::endl;
            errorOccurred = true;
            continue;
        }
        // swap color in color map
        img.modifyImage();
        img.colorMap(oldIndex, colorMap[m_newIndex]);
        img.colorMap(m_newIndex, colorMap[oldIndex]);
        // swap color indicies in pixels
        const auto nrOfIndices = img.columns() * img.rows();
        auto pixels = img.getPixels(0, 0, img.columns(), img.rows()); // we need to call this first for getIndexes to work
        IndexPacket *indices = img.getIndexes();
        for (size_t i = 0; i < nrOfIndices; ++i)
        {
            if (indices[i] == oldIndex)
            {
                indices[i] = m_newIndex;
            }
            else if (indices[i] == m_newIndex)
            {
                indices[i] = oldIndex;
            }
        }
        // sync and write modified image
        img.syncPixels();
        // generate file name
        std::string outFile = stringSprintf(m_outFile, i);
        std::cout << "Writing " << outFile << std::endl;
        try
        {
            img.write(outFile);
        }
        catch (const Exception &ex)
        {
            std::cerr << "Failed to write " << outFile << ": " << ex.what() << std::endl;
            errorOccurred = true;
            continue;
        }
    }
    return errorOccurred ? -2 : 0;
}