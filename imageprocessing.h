#pragma once

#include "exception.h"
#include "datahelpers.h"

#include <variant>
#include <cstdint>
#include <vector>
#include <Magick++.h>

class ImageProcessing
{
public:
    /// @brief Stores data for an image
    struct Data
    {
        std::string fileName;
        Magick::ImageType type;
        Magick::Geometry size;
        uint32_t bitsPerPixel;
        std::vector<uint8_t> data;
        std::vector<Magick::Color> colorMap;
    };

    /// @brief Type of processing to be done
    enum class Type
    {
        ConvertTiles,      // Convert data to 8 x 8 pixel tiles
        ConvertSprites,    // Convert data to w x h pixel sprites
        AddColor0,         // Add a color at index #0
        MoveColor0,        // Move a color to index #0
        ReorderColors,     // Reorder colors to be perceptually closer to each other
        ShiftIndices,      // Shift indices by N
        PruneIndices,      // Convert index data to 4-bit
        ConvertDelta8,     // Convert image data to 8-bit deltas
        ConvertDelta16,    // Convert image data to 16-bit deltas
        CompressLz10,      // Compress image data using LZ77 variant 10
        CompressLz11,      // Compress image data using LZ77 variant 11
        PadImageData,      // Fill up image data with 0s to a multiple of N
        PadColorMap,       // Fill up color map with 0s to a multiple of N
        EqualizeColorMaps, // Fill up all color maps with 0s to the size of the biggest color map
        ImageDiff          // Calculate signed pixel difference between successive images
    };

    /// @brief Variable parameters for processing step
    using Parameter = std::variant<bool, int32_t, uint32_t, float, Magick::Color>;

    /// @brief Add a processing step without parameters
    void addStep(Type type);

    /// @brief Add a processing step and its parameter
    void addStep(Type type, const Parameter &parameter);

    /// @brief Add a processing step and its parameters
    void addStep(Type type, const std::vector<Parameter> &parameters);

    /// @brief Get current # of steps in processing pipeline
    std::size_t size() const;

    /// @brief Remove all processing steps
    void clear();

    /// @brief Get human-readable description for processing steps in pipeline
    std::string getProcessingDescription(const std::string &seperator = ", ");

    /// @brief Run processing steps in pipepline on data
    std::vector<Data> process(const std::vector<Data> &images);

    // --- individual conversion functions ------------------------------------

    /// @brief Cut data to 8 x 8 pixel wide tiles and store per tile instead of per scanline.
    /// Width and height of image MUST be a multiple of 8!
    static Data toTiles(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Cut data to w x h pixel wide sprties and store per sprite instead of per scanline.
    /// Width and height of image MUST be a multiple of 8 and of sprit width.
    /// @param parameters Sprite width as uint32_t
    static Data toSprites(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Add color at palette index #0, shifting all other color indices +1
    /// @param parameters Color to add as Magick::Color
    static Data addColor0(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Move specific color to palette index #0, shifting all other colors accordingly
    /// @param parameters Color to move as Magick::Color
    static Data moveColor0(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Reorder color palette indices in image, so that similar colors are closer together.
    /// Uses a [simple metric](https://www.compuphase.com/cmetric.htm) to compute color distance with highly subjective results.
    static Data reorderColors(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Increate image palette indices by a value
    /// @param parameters Shift value to add to index as uint32_t
    static Data shiftIndices(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image index data to 4-bit values
    static Data pruneIndices(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image data to 8-bit deltas
    static Data toDelta8(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image data to 16-bit deltas
    static Data toDelta16(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Compress image data using LZ77 variant 10
    /// @param parameters Flag for VRAM-compatible compression as bool. Pass true to turn on
    static Data compressLZ10(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Compress image data using LZ77 variant 11
    /// @param parameters Flag for VRAM-compatible compression as bool. Pass true to turn on
    static Data compressLZ11(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Fill up image data with 0s to a multiple of N
    /// @param parameters "Modulo value" as uint32_t. The data will be padded to a multiple of this
    static Data padImageData(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Fill up color map with 0s to a multiple of N
    /// @param parameters "Modulo value" as uint32_t. The data will be padded to a multiple of this
    static Data padColorMap(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Fill up all color maps with 0s to the size of the biggest color map
    static std::vector<Data> equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters);

    /// @brief Combine image data of all images and return the data and the start indices into that data.
    /// Indices are return in DATA_TYPE units
    template <typename DATA_TYPE>
    static std::pair<std::vector<DATA_TYPE>, std::vector<uint32_t>> combineImageData(const std::vector<Data> &images, bool interleaveData = false)
    {
        std::vector<std::vector<uint8_t>> temp8;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp8), [](const auto &img)
                       { return img.data; });
        if (interleaveData)
        {
            const auto allDataSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().data.size()](const auto &img)
                                                          { return img.data.size() == refSize; }) == images.cend();
            REQUIRE(allDataSameSize, std::runtime_error, "The image data size of all images must be the same for interleaving");
            return {convertTo<DATA_TYPE>(interleave(temp8, images.front().bitsPerPixel)), std::vector<uint32_t>()};
        }
        else
        {
            return {combineTo<DATA_TYPE>(temp8), divideBy<uint32_t>(getStartIndices(temp8), sizeof(DATA_TYPE) / sizeof(uint8_t))};
        }
    }

    /// @brief Combine color maps of all images using conversion function and return the data and the start indices into that data.
    /// Indices are return in DATA_TYPE units
    template <typename DATA_TYPE>
    static std::pair<std::vector<DATA_TYPE>, std::vector<uint32_t>> combineColorMaps(const std::vector<Data> &images, const std::function<std::vector<DATA_TYPE>(const std::vector<Magick::Color> &)> &converter)
    {
        std::vector<std::vector<DATA_TYPE>> temp;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp), [converter](const auto &img)
                       { return converter(img.colorMap); });
        return {combineTo<DATA_TYPE>(temp), getStartIndices(temp)};
    }

private:
    struct ProcessingStep
    {
        Type type;
        std::vector<Parameter> parameters;
    };
    std::vector<ProcessingStep> m_steps;

    enum class OperationType
    {
        Convert,      // Converts 1 input into 1 output
        BatchConvert, // Converts N inputs into N outputs
        Combine,      // Converts 2 inputs into 1 output
        Reduce        // Converts N inputs into 1 output
    };

    using ConvertType = std::function<Data(const Data &, const std::vector<Parameter> &)>;
    using BatchConvertType = std::function<std::vector<Data>(const std::vector<Data> &, const std::vector<Parameter> &)>;
    using CombineType = std::function<Data(const Data &, const Data &, const std::vector<Parameter> &)>;
    using ReduceType = std::function<Data(const std::vector<Data> &, const std::vector<Parameter> &)>;
    using FunctionType = std::variant<ConvertType, BatchConvertType, CombineType, ReduceType>;

    struct ProcessingFunc
    {
        std::string description;
        OperationType type;
        FunctionType func;
    };
    static const std::map<Type, ProcessingFunc> ProcessingFunctions;
};
