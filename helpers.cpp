#include "helpers.h"

/// @brief Write values as a comma-separated array of hex numbers.
template <typename T>
void writeValues(std::ofstream &outFile, const std::vector<T> &data, bool asHex = false)
{
    auto flags = outFile.flags();
    size_t loopCount = 0;
    for (auto current : data)
    {
        if (asHex)
        {
            outFile << "0x" << std::hex << std::noshowbase << std::setw(sizeof(T) * 2) << std::setfill('0') << current;
        }
        else
        {
            outFile << std::dec << current;
        }
        if (loopCount < data.size() - 1)
        {
            outFile << ", ";
        }
        if (++loopCount % 10 == 0)
        {
            outFile << std::endl;
        }
    }
    outFile.flags(flags);
}

void writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages, bool asTiles)
{
    hFile << "#pragma once" << std::endl;
    hFile << "#include <stdint.h>" << std::endl
          << std::endl;
    if (asTiles)
    {
        hFile << "#define " << varName << "_WIDTH " << width << " // width of sprites/tiles in pixels" << std::endl;
        hFile << "#define " << varName << "_HEIGHT " << height << " // height of sprites/tiles in pixels" << std::endl;
        hFile << "#define " << varName << "_BYTES_PER_TILE " << bytesPerImage << " // bytes for one complete sprite/tile" << std::endl;
        hFile << "#define " << varName << "_DATA_SIZE " << data.size() << " // size of sprite/tile data in DWORDs" << std::endl;
    }
    else
    {
        hFile << "#define " << varName << "_WIDTH " << width << " // width of image in pixels" << std::endl;
        hFile << "#define " << varName << "_HEIGHT " << height << " // height of image in pixels" << std::endl;
        hFile << "#define " << varName << "_BYTES_PER_IMAGE " << bytesPerImage << " // bytes for one complete image" << std::endl;
        hFile << "#define " << varName << "_DATA_SIZE " << data.size() << " // size of image data in DWORDs" << std::endl;
    }
    if (nrOfImages > 1)
    {
        if (asTiles)
        {
            hFile << "#define " << varName << "_NR_OF_TILES " << nrOfImages << " // # of sprites/tiles in data" << std::endl;
        }
        else
        {
            hFile << "#define " << varName << "_NR_OF_IMAGES " << nrOfImages << " // # of images in data" << std::endl;
            hFile << "extern const uint32_t " << varName << "_DATA_START[" << varName << "_NR_OF_IMAGES]; // index where the data for an image starts" << std::endl;
        }
    }
    hFile << "extern const uint32_t " << varName << "_DATA[" << varName << "_DATA_SIZE];" << std::endl;
}

void writePaletteInfoToHeader(std::ofstream &hFile, const std::string &varName, const std::vector<uint16_t> &data, uint32_t nrOfColors, bool singleColorMap, bool asTiles)
{
    hFile << "#define " << varName << "_PALETTE_LENGTH " << nrOfColors << " // # of palette entries per palette" << std::endl;
    hFile << "#define " << varName << "_PALETTE_SIZE " << data.size() << " // size of palette data in WORDs" << std::endl;
    if (!singleColorMap)
    {
        hFile << "extern const uint32_t " << varName << "_PALETTE_START[" << varName << (asTiles ? "_NR_OF_TILES]; // index where a palette for a sprite/tile starts" : "_NR_OF_IMAGES]; // index where a palette for an image starts") << std::endl;
    }
    hFile << "extern const uint16_t " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE];" << std::endl;
}

void writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &startIndices, bool asTiles)
{
    cFile << "#include \"" << hFileBaseName << ".h\"" << std::endl
          << std::endl;
    // write data start indices if passed
    if (!startIndices.empty())
    {
        cFile << "const uint32_t " << varName << "_DATA_START[" << varName << (asTiles ? "_NR_OF_TILES] = { " : "_NR_OF_IMAGES] = { ") << std::endl;
        writeValues(cFile, startIndices);
        cFile << "};" << std::endl
              << std::endl;
    }
    // write image data
    cFile << "const _Alignas(4) uint32_t " << varName << "_DATA[" << varName << "_DATA_SIZE] = { " << std::endl;
    writeValues(cFile, data, true);
    cFile << "};" << std::endl
          << std::endl;
}

void writePaletteDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<uint16_t> &data, const std::vector<uint32_t> &startIndices, bool asTiles)
{
    // write palette start indices if more than one palette
    if (!startIndices.empty())
    {
        cFile << "const uint32_t " << varName << "_PALETTE_START[" << varName << (asTiles ? "_NR_OF_TILES] = { " : "_NR_OF_IMAGES] = { ") << std::endl;
        writeValues(cFile, startIndices);
        cFile << "};" << std::endl
              << std::endl;
    }
    // write palette data
    cFile << "const _Alignas(4) uint16_t " << varName << "_PALETTE[" << varName << "_PALETTE_SIZE] = { " << std::endl;
    writeValues(cFile, data, true);
    cFile << "};" << std::endl
          << std::endl;
}

std::string getBaseNameFromFilePath(const std::string &filePath)
{
    std::string baseName = filePath;
    baseName = baseName.substr(baseName.find_last_of("/\\") + 1);
    baseName = baseName.substr(0, baseName.find_first_of("."));
    return baseName;
}

std::pair<bool, uint32_t> stringToUint(const std::string &s)
{
    std::pair<bool, uint32_t> result;
    try
    {
        result.second = std::stoul(s);
        result.first = true;
    }
    catch (const std::invalid_argument &ex)
    {
        result.first = false;
    }
    catch (const std::out_of_range &ex)
    {
        result.first = false;
    }
    return result;
}