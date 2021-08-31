// some utility functions used by the tools
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

#if defined(__GNUC__) || defined(__clang__)
#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#elif defined(_MSC_VER)
#include <filesystem>
namespace stdfs = std::tr2::sys;
#endif

/// @brief Write image information to a .h file.
void writeImageInfoToH(std::ofstream &hFile, const std::string &varName, const std::vector<uint32_t> &data, uint32_t width, uint32_t height, uint32_t bytesPerImage, uint32_t nrOfImages = 1, bool asTiles = false);
/// @brief Write additional palette information to a .h file. Use after write writeImageInfoToH.
void writePaletteInfoToHeader(std::ofstream &hFile, const std::string &varName, const std::vector<uint16_t> &data, uint32_t nrOfColors, bool singleColorMap = true, bool asTiles = false);
/// @brief Write image data to a .c file.
void writeImageDataToC(std::ofstream &cFile, const std::string &varName, const std::string &hFileBaseName, const std::vector<uint32_t> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), bool asTiles = false);
/// @brief Write palette data to a .c file. Use after write writeImageDataToC.
void writePaletteDataToC(std::ofstream &cFile, const std::string &varName, const std::vector<uint16_t> &data, const std::vector<uint32_t> &startIndices = std::vector<uint32_t>(), bool asTiles = false);

/// @brief Get base name from file path.
std::string getBaseNameFromFilePath(const std::string &filePath);

/// @brief Tries to resolve wildcards and check if all files exists.
/// @return [true, resolved_absolute_paths_to_files] if all files are found, [false, something] if not.
/// @todo On Windows we might need to resolve wildcards in input files.
std::pair<bool, std::vector<std::string>> resolveFilePaths(const std::vector<std::string> &files);

/// @brief Get environment variable from system.
std::string getEnv(const std::string &var);
