#pragma once

#include <cstdint>

namespace Image
{

    /// @brief Type of processing to be done
    enum class ProcessingType : uint8_t
    {
        Uncompressed = 0,        // Verbatim data copy
        InputBlackWhite = 10,    // Input image and convert to 2-color paletted image
        InputPaletted = 11,      // Input image and convert to paletted image
        InputTruecolor = 12,     // Input image and convert to RGB888 truecolor
        InputCommonPalette = 13, // Input images and remap colors of all images to a common palette
        ConvertTiles = 20,       // Convert data to 8 x 8 pixel tiles
        ConvertSprites = 21,     // Convert data to w x h pixel sprites
        BuildTileMap = 22,       // Convert data to 8 x 8 pixel tiles and build optimized screen and tile map
        AddColor0 = 30,          // Add a color at index #0
        MoveColor0 = 31,         // Move a color to index #0
        ReorderColors = 32,      // Reorder colors to be perceptually closer to each other
        ShiftIndices = 40,       // Shift indices by N
        PruneIndices = 41,       // Convert index data to 1-bit, 2-bit or 4-bit
        ConvertDelta8 = 50,      // Convert image data to 8-bit deltas
        ConvertDelta16 = 51,     // Convert image data to 16-bit deltas
        DeltaImage = 55,         // Calculate signed pixel difference between successive images
        CompressLz10 = 60,       // Compress image data using LZ77 variant 10
        CompressLz11 = 61,       // Compress image data using LZ77 variant 11
        CompressRLE = 65,        // Compress image data using run-length-encoding
        CompressDXTG = 70,       // Compress image data using DXTG
        CompressDXTV = 71,       // Compress image data using DXTV
        CompressGVID = 72,       // Compress image data using GVID
        PadImageData = 80,       // Fill up image data with 0s to a multiple of N bytes
        PadColorMap = 90,        // Fill up color map with 0s to a multiple of N colors
        ConvertColorMap = 91,    // Convert input color map to raw data
        PadColorMapData = 92,    // Fill up raw color map data with 0s to a multiple of N bytes
        EqualizeColorMaps = 93   // Fill up all color maps with 0s to the size of the biggest color map
    };

    static constexpr uint8_t ProcessingTypeFinal = 128; // Marks the final processing step in an encoding sequence. Is ORed with Processing::Type

}
