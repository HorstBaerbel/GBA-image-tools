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
    // Color format info
    enum class ColorFormat
    {
        Unknown,
        Paletted4, // 4bit paletted format
        Paletted8, // 8bit paletted format
        RGB555,    // RGB555 GBA format
        RGB565,    // RGB565 format for DXT
        RGB888     // RGB888 straight truecolor format
    };

    /// @brief Return bits per pixel for input color format
    /// @note Note that RGB555 and RGB565 both return 16!
    static uint32_t bitsPerPixelForFormat(ColorFormat format);

    /// @brief Stores data for an image
    struct Data
    {
        std::string fileName;                // input file name
        Magick::ImageType type;              // input image type
        Magick::Geometry size;               // image size
        ColorFormat format;                  // image data format
        std::vector<uint8_t> data;           // image data
        std::vector<Magick::Color> colorMap; // image color map if paletted
    };

    /// @brief Type of processing to be done
    enum class Type
    {
        InputBinary,       // Input image and convert to 2-color paletted image
        InputPaletted,     // Input image and convert to paletted image
        InputTruecolor,    // Input image and convert to RGB888 truecolor
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
        CompressRLE,       // Compress image data using run-length-encoding
        CompressDXT1,      // Compress image data using DXT1
        PadImageData,      // Fill up image data with 0s to a multiple of N
        PadColorMap,       // Fill up color map with 0s to a multiple of N
        EqualizeColorMaps, // Fill up all color maps with 0s to the size of the biggest color map
        DeltaImage         // Calculate signed pixel difference between successive images
    };

    /// @brief Variable parameters for processing step
    using Parameter = std::variant<bool, int32_t, uint32_t, float, Magick::Color, Magick::Image, Data, std::string>;

    /// @brief Add a processing step without parameters
    void addStep(Type type);

    /// @brief Add a processing step and its parameters
    void addStep(Type type, const std::vector<Parameter> &parameters);

    /// @brief Get current # of steps in processing pipeline
    std::size_t size() const;

    /// @brief Remove all processing steps
    void clear();

    /// @brief Get human-readable description for processing steps in pipeline
    std::string getProcessingDescription(const std::string &seperator = ", ");

    /// @brief Run processing steps in pipeline on data. Used for processing a batch of images
    /// @param images Input data
    /// @param clearState Clear internal state for all operations
    /// @note Will silently ignore OperationType::Input operations
    std::vector<Data> processBatch(const std::vector<Data> &images, bool clearState = true);

    /// @brief Run processing steps in pipeline on single image. Used for processing a stream of images
    /// @param image Input image
    /// @param clearState Clear internal state for all operations
    /// @note Will silently ignore OperationType::BatchConvert and ::Reduce operations
    Data processStream(const Magick::Image &image, bool clearState = true);

    // --- image conversion functions ------------------------------------

    /// @brief Binarize image using threshold. Everything < threshold will be black everything > threshold white
    /// @param parameters Binarization threshold as float. Must be in [0.0, 1.0]
    static Data toBinary(const Magick::Image &image, const std::vector<Parameter> &parameters);

    /// @brief Convert input image to paletted image by:
    /// - Mapping colors to colorSpaceMap (ImageMagicks -remap option)
    /// - Dithering to nrOfColors (ImageMagicks -colors option)
    /// @param parameters Magick::Image containing all colors of the target color space, e.g. RGB555 and
    ///                   Target number of colors in palette as uint32_t. This is an upper bound, the palette may be smaller.
    static Data toPaletted(const Magick::Image &image, const std::vector<Parameter> &parameters);

    /// @brief Convert input image to RGB55, RGB565 or RGB888
    /// @param parameters Truecolor format to convert image to as std::string
    static Data toTruecolor(const Magick::Image &image, const std::vector<Parameter> &parameters);

    // --- data conversion functions ------------------------------------

    /// @brief Cut data to 8 x 8 pixel wide tiles and store per tile instead of per scanline.
    /// Width and height of image MUST be a multiple of 8!
    /// @param parameters Unused
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
    /// @param parameters Unused
    static Data reorderColors(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Increate image palette indices by a value
    /// @param parameters Shift value to add to index as uint32_t
    static Data shiftIndices(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image index data to 4-bit values
    /// @param parameters Unused
    static Data pruneIndices(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image data to 8-bit deltas
    /// @param parameters Unused
    static Data toDelta8(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Convert image data to 16-bit deltas
    /// @param parameters Unused
    static Data toDelta16(const Data &image, const std::vector<Parameter> &parameters);

    // --- compression functions -------------------------------------------------------------

    /// @brief Compress image data using LZ77 variant 10
    /// @param parameters Flag for VRAM-compatible compression as bool. Pass true to turn on
    static Data compressLZ10(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Compress image data using LZ77 variant 11
    /// @param parameters Flag for VRAM-compatible compression as bool. Pass true to turn on
    static Data compressLZ11(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Compress image data using RLE
    /// @param parameters Flag for VRAM-compatible compression as bool. Pass true to turn on
    static Data compressRLE(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Encode a truecolor RGB888 or RGB565 image as DXT1 with RGB565 pixels
    static Data compressDXT1(const Data &image, const std::vector<Parameter> &parameters);

    // --- misc conversion functions ------------------------------------------------------------------------

    /// @brief Fill up image data with 0s to a multiple of N
    /// @param parameters "Modulo value" as uint32_t. The data will be padded to a multiple of this
    static Data padImageData(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Fill up color map with 0s to a multiple of N
    /// @param parameters "Modulo value" as uint32_t. The data will be padded to a multiple of this
    static Data padColorMap(const Data &image, const std::vector<Parameter> &parameters);

    /// @brief Fill up all color maps with 0s to the size of the biggest color map
    static std::vector<Data> equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters);

    /// @brief Calcuate pixel-difference to previous image
    /// @param parameters Unused
    /// @param state Previous image as Magick::Image
    static Data imageDiff(const Data &image, const std::vector<Parameter> &parameters, std::vector<Parameter> &state);

    /// @brief Combine image data of all images and return the data and the start indices into that data.
    /// Indices are return in DATA_TYPE units
    template <typename DATA_TYPE>
    static std::pair<std::vector<DATA_TYPE>, std::vector<uint32_t>> combineImageData(const std::vector<Data> &images, bool interleavePixels = false)
    {
        std::vector<std::vector<uint8_t>> temp8;
        std::transform(images.cbegin(), images.cend(), std::back_inserter(temp8), [](const auto &img)
                       { return img.data; });
        if (interleavePixels)
        {
            const auto allDataSameSize = std::find_if_not(images.cbegin(), images.cend(), [refSize = images.front().data.size()](const auto &img)
                                                          { return img.data.size() == refSize; }) == images.cend();
            REQUIRE(allDataSameSize, std::runtime_error, "The image data size of all images must be the same for interleaving");
            return {convertTo<DATA_TYPE>(interleave(temp8, bitsPerPixelForFormat(images.front().format))), std::vector<uint32_t>()};
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
        std::vector<Parameter> state;
    };
    std::vector<ProcessingStep> m_steps;

    enum class OperationType
    {
        Input,        // Converts image input into 1 data output
        Convert,      // Converts 1 data input into 1 data output
        ConvertState, // Converts 1 data input + state into 1 data output
        BatchConvert, // Converts N data inputs into N data outputs
        Reduce        // Converts N data inputs into 1 data output
    };

    using InputFunc = std::function<Data(const Magick::Image &, const std::vector<Parameter> &)>;
    using ConvertFunc = std::function<Data(const Data &, const std::vector<Parameter> &)>;
    using ConvertStateFunc = std::function<Data(const Data &, const std::vector<Parameter> &, std::vector<Parameter> &)>;
    using BatchConvertFunc = std::function<std::vector<Data>(const std::vector<Data> &, const std::vector<Parameter> &)>;
    using ReduceFunc = std::function<Data(const std::vector<Data> &, const std::vector<Parameter> &)>;
    using FunctionType = std::variant<InputFunc, ConvertFunc, ConvertStateFunc, BatchConvertFunc, ReduceFunc>;

    struct ProcessingFunc
    {
        std::string description;
        OperationType type;
        FunctionType func;
    };
    static const std::map<Type, ProcessingFunc> ProcessingFunctions;

    static std::vector<std::vector<uint8_t>> RGB565DistanceSqrCache;

    /// @brief Encode a pixel block as DXT1 with RGB565 pixels
    static std::vector<uint8_t> encodeBlockDXT1(const uint16_t *start, uint32_t pixelStride, const std::vector<std::vector<uint8_t>> &distanceSqrMap);
};
