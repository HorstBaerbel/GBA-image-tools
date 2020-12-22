#include "helpers.h"

#include <cstring>

std::vector<uint8_t> getImageData(const Image &img)
{
    std::vector<uint8_t> data;
    if (img.type() == PaletteType)
    {
        const auto nrOfColors = img.colorMapSize();
        const auto nrOfIndices = img.columns() * img.rows();
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows()); // we need to call this first for getIndices to work...
        auto indices = img.getConstIndexes();
        if (nrOfColors <= 16)
        {
            // size must be a multiple of 2
            if (nrOfIndices & 1)
            {
                throw std::runtime_error("Number of indices must be even!");
            }
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 2)
            {
                uint8_t v = (((indices[i + 1]) & 0x0F) << 4);
                v |= ((indices[i]) & 0x0F);
                data.push_back(v);
            }
        }
        else if (nrOfColors <= 256)
        {
            for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; ++i)
            {
                data.push_back(indices[i]);
            }
        }
        else
        {
            throw std::runtime_error("Only up to 256 colors supported in color map!");
        }
    }
    else if (img.type() == TrueColorType)
    {
        // get pixel colors and rescale to RGB555
        const auto nrOfPixels = img.columns() * img.rows();
        auto pixels = img.getConstPixels(0, 0, img.columns(), img.rows());
        for (std::remove_const<decltype(nrOfPixels)>::type i = 0; i < nrOfPixels; ++i)
        {
            auto pixel = pixels[i];
            data.push_back((31 * pixel.red) / QuantumRange);
            data.push_back((31 * pixel.green) / QuantumRange);
            data.push_back((31 * pixel.blue) / QuantumRange);
        }
    }
    else
    {
        throw std::runtime_error("Unsupported image type!");
    }
    /*std::ofstream of("dump.hex", std::ios::binary | std::ios::out);
    of.write(reinterpret_cast<const char *>(data.data()), data.size());
    of.close();*/
    return data;
}

std::vector<uint8_t> incImageIndicesBy1(const std::vector<uint8_t> &imageData)
{
    auto tempData = imageData;
    std::for_each(tempData.begin(), tempData.end(), [](auto &index) { index++; });
    return tempData;
}

std::vector<uint8_t> convertToWidth(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth)
{
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTileLine = bitsPerPixel * (tileWidth / 8);
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    uint8_t *dstData = dst.data();
    for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
    {
        const uint8_t *srcLine = src.data() + (blockX * bitsPerPixel) / 8;
        for (uint32_t tileY = 0; tileY < height; ++tileY)
        {
            std::memcpy(dstData, srcLine, bytesPerTileLine);
            dstData += bytesPerTileLine;
            srcLine += bytesPerSrcLine;
        }
    }
    return dst;
}

std::vector<uint8_t> convertToTiles(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t tileWidth, uint32_t tileHeight)
{
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTileLine = bitsPerPixel * (tileWidth / 8);
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    uint8_t *dstData = dst.data();
    for (uint32_t blockY = 0; blockY < height; blockY += tileHeight)
    {
        const uint8_t *srcBlock = src.data() + blockY * bytesPerSrcLine;
        for (uint32_t blockX = 0; blockX < width; blockX += tileWidth)
        {
            const uint8_t *srcLine = srcBlock + (blockX * bitsPerPixel) / 8;
            for (uint32_t tileY = 0; tileY < tileHeight; ++tileY)
            {
                std::memcpy(dstData, srcLine, bytesPerTileLine);
                dstData += bytesPerTileLine;
                srcLine += bytesPerSrcLine;
            }
        }
    }
    return dst;
}

std::vector<uint8_t> convertToSprites(const std::vector<uint8_t> &src, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint32_t spriteWidth, uint32_t spriteHeight)
{
    // convert to tiles first
    auto tileData = convertToTiles(src, width, height, bitsPerPixel);
    // now convert to sprites
    std::vector<uint8_t> dst(src.size());
    const uint32_t bytesPerTile = bitsPerPixel * 8;
    const uint32_t bytesPerSrcLine = (width * bitsPerPixel) / 8;
    const uint32_t spritesHorizontal = width / spriteWidth;
    const uint32_t spritesVertical = height / spriteHeight;
    const uint32_t spriteTileWidth = spriteWidth / 8;
    const uint32_t spriteTileHeight = spriteHeight / 8;
    const uint32_t bytesPerSpriteLine = spriteTileWidth * bytesPerTile;
    uint8_t *dstData = dst.data();
    for (uint32_t spriteY = 0; spriteY < spritesVertical; ++spriteY)
    {
        const uint8_t *srcBlock = src.data() + spriteY * spriteHeight * bytesPerSrcLine;
        for (uint32_t spriteX = 0; spriteX < spritesHorizontal; ++spriteX)
        {
            const uint8_t *srcTile = srcBlock + spriteX * bytesPerSpriteLine;
            for (uint32_t tileY = 0; tileY < spriteTileHeight; ++tileY)
            {
                std::memcpy(dstData, srcTile, bytesPerSpriteLine);
                dstData += bytesPerSpriteLine;
                srcTile += bytesPerSrcLine * 8;
            }
        }
    }
    return dst;
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