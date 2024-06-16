#pragma once

#include "color/colorformat.h"
#include "color/xrgb8888.h"
#include "datahelpers.h"
#include "exception.h"
#include "imagestructs.h"
#include "processinghelpers.h"
#include "processingtypes.h"
#include "quantizationmethod.h"
#include "statistics/statistics.h"

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace Image
{

    class Processing
    {
    public:
        /// @brief Variable parameters for processing step
        using Parameter = std::variant<bool, int32_t, uint32_t, double, Color::Format, Quantization::Method, Color::XRGB8888, std::vector<Color::XRGB8888>, Data, std::string>;

        /// @brief Set object to receive statistics from processing pipeline
        void setStatisticsContainer(Statistics::Container::SPtr c);

        /// @brief Add a processing step and its parameters
        /// @param type Processing type
        /// @param parameters Parameters to pass to processing
        /// @param prependProcessingInfo If true the input data size and processing type will be prepended to the result
        /// @param addStatistics The step should output statistics to the container set with setStatisticsContainer()
        void addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool prependProcessingInfo = false, bool addStatistics = false);

        /// @brief Get current # of steps in processing pipeline
        std::size_t size() const;

        /// @brief Remove all processing steps
        void clear();

        /// @brief Clear the internal state of all processing steps. Call to reset state if you want to run the processing pipeline multiple times
        void clearState();

        /// @brief Get human-readable description for processing steps in pipeline
        std::string getProcessingDescription(const std::string &seperator = ", ");

        /// @brief Run processing steps in pipeline on data. Used for processing a batch of images
        /// @param data Input data and file names
        std::vector<Data> processBatch(const std::vector<Data> &data);

        /// @brief Run processing steps in pipeline on single image. Used for processing a stream of images / video frames
        /// @param data Input data and file name
        /// @note Will silently ignore OperationType::BatchConvert and ::Reduce operations
        Data processStream(const Data &data);

        // --- image conversion functions ------------------------------------

        /// @brief Binarize image using threshold. Everything < threshold will be black everything > threshold white
        /// @param parameters Binarization threshold as double. Must be in [0.0, 1.0]
        /// @return Returns data as Paletted8
        static Data toBlackWhite(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert input image to paletted image by:
        /// - Mapping colors to colorSpaceMap (ImageMagicks -remap option)
        /// - Dithering to nrOfColors (ImageMagicks -colors option)
        /// @param parameters Image containing all colors of the target color space, e.g. RGB555 and
        ///                   Target number of colors in palette as uint32_t. This is an upper bound, the palette may be smaller.
        /// @return Returns data as Paletted8
        static Data toPaletted(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert all input images to paletted images by:
        /// - Mapping colors to colorSpaceMap (ImageMagicks -remap option)
        /// - Finding a common palette of all images with nrOfColors
        /// - Dithering to nrOfColors (ImageMagicks -colors option)
        /// @param parameters Image containing all colors of the target color space, e.g. RGB555 and
        ///                   Target number of colors in palette as uint32_t. This is an upper bound, the palette may be smaller.
        /// @return Returns data as Paletted8
        static std::vector<Data> toCommonPalette(const std::vector<Data> &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert input image to RGB555, RGB565 or RGB888
        /// @param parameters Truecolor format to convert image to as Color::Format
        /// @return Returns data as XRGB1555, RGB565 or XRGB8888
        static Data toTruecolor(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        // --- data conversion functions ------------------------------------

        /// @brief Store optimized tile and screen map. Only max. 1024 unique tiles allowed!
        /// Width and height of image MUST be a multiple of 8!
        /// Will detect horizontally, vertically and horizontally+vertically flipped tiles and will set the map index flip flags accordingly (if parameter set)
        /// @param parameters Pass true to detect flip tiles and set flip flags
        static Data toUniqueTileMap(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Cut data to 8 x 8 pixel wide tiles and store per tile instead of per scanline.
        /// Width and height of image MUST be a multiple of 8!
        /// @param parameters Unused
        static Data toTiles(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Cut data to w x h pixel wide sprites and store per sprite instead of per scanline.
        /// Width and height of image MUST be a multiple of 8 and of sprit width.
        /// @param parameters Sprite width as uint32_t
        static Data toSprites(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Add color at palette index #0, shifting all other color indices +1
        /// @param parameters Color to add as Color::XRGB8888
        static Data addColor0(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Move specific color to palette index #0, shifting all other colors accordingly
        /// @param parameters Color to move as Color::XRGB8888
        static Data moveColor0(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Reorder color palette indices in image, so that similar colors are closer together.
        /// Uses a [simple metric](https://www.compuphase.com/cmetric.htm) to compute color distance with highly subjective results.
        /// @param parameters Unused
        static Data reorderColors(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Increate image palette indices by a value
        /// @param parameters Shift value to add to index as uint32_t
        static Data shiftIndices(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert image index data to 1-,2- or 4-bit values
        /// @param parameters Unused
        static Data pruneIndices(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert image data to 8-bit deltas
        /// @param parameters Unused
        static Data toDelta8(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert image data to 16-bit deltas
        /// @param parameters Unused
        static Data toDelta16(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        // --- compression functions -------------------------------------------------------------

        /// @brief Compress image data using LZ77 variant 10
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        static Data compressLZ10(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Compress image data using RLE
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        static Data compressRLE(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Encode a truecolor RGB888 or RGB555 image as DXT1-ish image with RGB555 pixels
        /// @param parameters: Unused
        static Data compressDXT(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Encode a truecolor RGB888 or RGB555 image as DXT1-ish image with RGB555 pixels
        /// Has additional intra- and inter-frame compression in comparison to DTXG
        /// @param parameters:
        /// - Key frame interval n as int in [1,20] meaning a key frame is stored every n frames
        /// - Maximum error for B-frame references (keyframes)
        /// - Maximum error for P-frame references (inter-frames)
        /// @param state Previous image as Data
        static Data compressDXTV(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics);

        /// @brief Encode a truecolor RGB888 image with YCgCoR block-based method
        /// @param parameters:
        /// - Allowed error for inter-frame block references as float in [0,1]. 0 means no error allowed
        /// - Key frame rate n as int in [1,20] meaning a key frame is stored every n frames
        /// @param state Previous image as Data
        static Data compressGVID(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics);

        // --- misc conversion functions ------------------------------------------------------------------------

        /// @brief Convert pixels to format.
        /// @param parameters Truecolor format to convert pixels to as Color::Format
        /// @param Will do nothing if the pixel data raw
        static Data convertPixelsToRaw(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Fill up pixel data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The pixel data will be padded to a multiple of this
        static Data padPixelData(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Convert color map to format.
        /// @param parameters Truecolor format to convert color map to as Color::Format
        /// @param Will do nothing if the pixel data raw
        static Data convertColorMapToRaw(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Fill up map data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The map data will be padded to a multiple of this
        static Data padMapData(const Data &data, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Fill up color map with 0s to a multiple of N colors
        /// @param parameters "Modulo value" as uint32_t. The color map will be padded to a multiple of this
        static Data padColorMap(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Fill up color map raw data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The raw color map data will be padded to a multiple of this
        static Data padColorMapData(const Data &image, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Fill up all color maps with 0s to the size of the biggest color map
        static std::vector<Data> equalizeColorMaps(const std::vector<Data> &images, const std::vector<Parameter> &parameters, Statistics::Container::SPtr statistics);

        /// @brief Calcuate pixel-difference to previous image
        /// @param parameters Unused
        /// @param state Previous image as Data
        static Data pixelDiff(const Data &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Container::SPtr statistics);

    private:
        struct ProcessingStep
        {
            ProcessingType type;
            std::vector<Parameter> parameters;
            bool prependProcessingInfo = false;
            bool addStatistics = false;
            std::vector<uint8_t> state;
        };
        std::vector<ProcessingStep> m_steps;
        Statistics::Container::SPtr m_statistics;

        enum class OperationType
        {
            Convert,      // Converts 1 data input into 1 data output
            ConvertState, // Converts 1 data input + state into 1 data output
            BatchConvert, // Converts N data inputs into N data outputs
            Reduce        // Converts N data inputs into 1 data output
        };

        using ConvertFunc = std::function<Data(const Data &, const std::vector<Parameter> &, Statistics::Container::SPtr statistics)>;
        using ConvertStateFunc = std::function<Data(const Data &, const std::vector<Parameter> &, std::vector<uint8_t> &, Statistics::Container::SPtr statistics)>;
        using BatchConvertFunc = std::function<std::vector<Data>(const std::vector<Data> &, const std::vector<Parameter> &, Statistics::Container::SPtr statistics)>;
        using ReduceFunc = std::function<Data(const std::vector<Data> &, const std::vector<Parameter> &, Statistics::Container::SPtr statistics)>;
        using FunctionType = std::variant<ConvertFunc, ConvertStateFunc, BatchConvertFunc, ReduceFunc>;

        struct ProcessingFunc
        {
            std::string description;
            OperationType type;
            FunctionType func;
        };
        static const std::map<ProcessingType, ProcessingFunc> ProcessingFunctions;
    };

}
