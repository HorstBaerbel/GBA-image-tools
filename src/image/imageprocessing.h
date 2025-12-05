#pragma once

#include "color/colorformat.h"
#include "color/xrgb8888.h"
#include "exception.h"
#include "imagedatahelpers.h"
#include "imagestructs.h"
#include "processing/datahelpers.h"
#include "processingtype.h"
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
        using Parameter = std::variant<bool, int32_t, uint32_t, double, Color::Format, Quantization::Method, Color::XRGB8888, std::vector<Color::XRGB8888>, Frame, std::string>;

        /// @brief Add a processing step and its parameters
        /// @param type Processing type
        /// @param parameters Parameters to pass to processing
        /// @param decodeRelevant If true the processing type is recorded for a call to getDecodingSteps().
        /// @param addStatistics The step should output statistics to the statistics container
        void addStep(ProcessingType type, const std::vector<Parameter> &parameters, bool decodeRelevant = false, bool addStatistics = false);

        /// @brief Get current # of steps in processing pipeline
        std::size_t nrOfSteps() const;

        /// @brief Remove all processing steps. Will also reset()
        void clearSteps();

        /// @brief Clear the internal state of all processing steps. Call to reset state if you want to run the processing pipeline multiple times
        void reset();

        /// @brief Get human-readable description for processing steps in pipeline
        std::string getProcessingDescription(const std::string &seperator = ", ");

        /// @brief Run processing steps in pipeline on data. Used for processing a batch of images
        /// @param data Input data and file names
        /// @note Currently no statistics are collected here
        std::vector<Frame> processBatch(const std::vector<Frame> &data);

        /// @brief Run processing steps in pipeline on single image. Used for processing a stream of images / video frames
        /// @param data Input data and file name
        /// @note Will silently ignore OperationType::BatchConvert and ::Reduce operations
        Frame processStream(const Frame &data, Statistics::Container::SPtr statistics = nullptr);

        /// @brief Get the processing needed to decode the data (steps reversed). These might not be all steps added with addStep()
        /// @return Processing steps needed to decode the data
        std::vector<ProcessingType> getDecodingSteps() const;

        // --- image conversion functions ------------------------------------

        /// @brief Binarize image using threshold. Everything < threshold will be black everything > threshold white
        /// @param parameters Binarization threshold as double. Must be in [0.0, 1.0]
        /// @return Returns data as Paletted8
        static Frame toBlackWhite(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert input image to paletted image by:
        /// - Mapping colors to colorSpaceMap (ImageMagicks -remap option)
        /// - Dithering to nrOfColors (ImageMagicks -colors option)
        /// @param parameters Image containing all colors of the target color space, e.g. RGB555 and
        ///                   Target number of colors in palette as uint32_t. This is an upper bound, the palette may be smaller.
        /// @return Returns data as Paletted8
        static Frame toPaletted(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert all input images to paletted images by:
        /// - Mapping colors to colorSpaceMap (ImageMagicks -remap option)
        /// - Finding a common palette of all images with nrOfColors
        /// - Dithering to nrOfColors (ImageMagicks -colors option)
        /// @param parameters Image containing all colors of the target color space, e.g. RGB555 and
        ///                   Target number of colors in palette as uint32_t. This is an upper bound, the palette may be smaller.
        /// @return Returns data as Paletted8
        static std::vector<Frame> toCommonPalette(const std::vector<Frame> &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert input image to RGB555, RGB565 or RGB888
        /// @param parameters Truecolor format to convert image to as Color::Format
        /// @return Returns data as XRGB1555, RGB565 or XRGB8888
        static Frame toTruecolor(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        // --- data conversion functions ------------------------------------

        /// @brief Store optimized tile and screen map. Only max. 1024 unique tiles allowed!
        /// Width and height of image MUST be a multiple of 8!
        /// Will detect horizontally, vertically and horizontally+vertically flipped tiles and will set the map index flip flags accordingly (if parameter set)
        /// @param parameters Pass true to detect flip tiles and set flip flags
        static Frame toUniqueTileMap(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Cut data to 8 x 8 pixel wide tiles and store per tile instead of per scanline.
        /// Width and height of image MUST be a multiple of 8!
        /// @param parameters Unused
        static Frame toTiles(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Cut data to w x h pixel wide sprites and store per sprite instead of per scanline.
        /// Width and height of image MUST be a multiple of 8 and of sprit width.
        /// @param parameters Sprite width as uint32_t
        static Frame toSprites(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Add color at palette index #0, shifting all other color indices +1
        /// @param parameters Color to add as Color::XRGB8888
        static Frame addColor0(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Move specific color to palette index #0, shifting all other colors accordingly
        /// @param parameters Color to move as Color::XRGB8888
        static Frame moveColor0(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Reorder color palette indices in image, so that similar colors are closer together.
        /// Uses a [simple metric](https://www.compuphase.com/cmetric.htm) to compute color distance with highly subjective results.
        /// @param parameters Unused
        static Frame reorderColors(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Increate image palette indices by a value
        /// @param parameters Shift value to add to index as uint32_t
        static Frame shiftIndices(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert image index data to 1-,2- or 4-bit values
        /// @param parameters Unused
        static Frame pruneIndices(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert image data to 8-bit deltas
        /// @param parameters Unused
        static Frame toDelta8(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert image data to 16-bit deltas
        /// @param parameters Unused
        static Frame toDelta16(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        // --- compression functions -------------------------------------------------------------

        /// @brief Compress image data using LZ77 variant 10h
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        static Frame compressLZ10(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Compress image data using rANS variant 40h
        /// @param parameters: none
        static Frame compressRANS40(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Compress image data using RLE
        /// @param parameters:
        /// - Flag for VRAM-compatible compression as bool. Pass true to turn on
        static Frame compressRLE(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Encode a truecolor RGB888 or RGB555 image as DXT1-ish image with RGB555 pixels
        /// @param parameters: Unused
        static Frame compressDXT(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Encode a truecolor RGB888 or RGB555 image as DXT1-ish image with RGB555 pixels
        /// Has additional intra- and inter-frame compression in comparison to DXT
        /// @param parameters:
        /// - Color format XRGB1555 or XBGR1555
        /// - Maximum error for I-frame (keyframes) and P-frame references (inter-frames)
        /// @param state Previous image as Data
        static Frame compressDXTV(const Frame &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics);

        /// @brief Encode a truecolor RGB888 image with YCgCoR block-based method
        /// @param parameters:
        /// - Allowed error for inter-frame block references as float in [0,1]. 0 means no error allowed
        /// - Key frame rate n as int in [1,20] meaning a key frame is stored every n frames
        /// @param state Previous image as Data
        static Frame compressGVID(const Frame &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics);

        // --- misc conversion functions ------------------------------------------------------------------------

        /// @brief Convert pixel color format and convert image data to raw data
        /// @param parameters Truecolor format to convert pixels to as Color::Format
        /// @note Will do nothing if the pixel data is already in raw format
        static Frame convertPixelsToRaw(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Fill up pixel data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The pixel data will be padded to a multiple of this
        static Frame padPixelData(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Convert color map color format and convert color map to raw data
        /// @param parameters Truecolor format to convert color map to as Color::Format
        /// @note Will do nothing if the color map data is already in raw format
        static Frame convertColorMapToRaw(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Fill up map data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The map data will be padded to a multiple of this
        static Frame padMapData(const Frame &data, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Fill up color map with 0s to a multiple of N colors
        /// @param parameters "Modulo value" as uint32_t. The color map will be padded to a multiple of this
        static Frame padColorMap(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Fill up color map raw data with 0s to a multiple of N bytes
        /// @param parameters "Modulo value" as uint32_t. The raw color map data will be padded to a multiple of this
        static Frame padColorMapData(const Frame &image, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Fill up all color maps with 0s to the size of the biggest color map
        static std::vector<Frame> equalizeColorMaps(const std::vector<Frame> &images, const std::vector<Parameter> &parameters, Statistics::Frame::SPtr statistics);

        /// @brief Calcuate pixel-difference to previous image
        /// @param parameters Unused
        /// @param state Previous image as Data
        static Frame pixelDiff(const Frame &image, const std::vector<Parameter> &parameters, std::vector<uint8_t> &state, Statistics::Frame::SPtr statistics);

    private:
        struct ProcessingStep
        {
            ProcessingType type;                // Type of processing operation applied
            std::vector<Parameter> parameters;  // Input parameters for operation
            bool decodeRelevant = false;        // If processing information is needed for decoding
            bool addStatistics = false;         // If operation statistics should be written to
            std::vector<uint8_t> state;         // The input / output state for stateful operations
        };
        std::vector<ProcessingStep> m_steps;

        enum class OperationType
        {
            Convert,      // Converts 1 data input into 1 data output
            ConvertState, // Converts 1 data input + state into 1 data output
            BatchConvert, // Converts N data inputs into N data outputs
            Reduce        // Converts N data inputs into 1 data output
        };

        using ConvertFunc = std::function<Frame(const Frame &, const std::vector<Parameter> &, Statistics::Frame::SPtr statistics)>;
        using ConvertStateFunc = std::function<Frame(const Frame &, const std::vector<Parameter> &, std::vector<uint8_t> &, Statistics::Frame::SPtr statistics)>;
        using BatchConvertFunc = std::function<std::vector<Frame>(const std::vector<Frame> &, const std::vector<Parameter> &, Statistics::Frame::SPtr statistics)>;
        using ReduceFunc = std::function<Frame(const std::vector<Frame> &, const std::vector<Parameter> &, Statistics::Frame::SPtr statistics)>;
        using FunctionType = std::variant<ConvertFunc, ConvertStateFunc, BatchConvertFunc, ReduceFunc>;

        struct ProcessingFunc
        {
            std::string description; // Processing operation description
            OperationType type;      // Of what type the operation is
            FunctionType func;       // Actual processing function
        };
        static const std::map<ProcessingType, ProcessingFunc> ProcessingFunctions;
    };

}
