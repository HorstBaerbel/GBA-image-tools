#pragma once

#include <cstdint>

namespace Image
{

    /// @brief Type of processing to be done
    enum class ProcessingType : uint8_t
    {
        Uncompressed = 0,                 // Verbatim data copy
        ConvertBlackWhite = 10,           // Convert image to 2-color paletted image
        ConvertPaletted = 11,             // Convert image to paletted image
        ConvertTruecolor = 12,            // Convert image to RGB555, RGB565, RGB888 truecolor
        ConvertCommonPalette = 14,        // Remap colors of all images to a common palette
        ConvertTiles = 20,                // Convert data to 8 x 8 pixel tiles
        ConvertSprites = 21,              // Convert data to w x h pixel sprites
        BuildTileMap = 22,                // Convert data to 8 x 8 pixel tiles and build optimized screen and tile map
        AddColor0 = 30,                   // Add a color at index #0
        MoveColor0 = 31,                  // Move a color to index #0
        ReorderColors = 32,               // Reorder colors to be perceptually closer to each other
        ShiftIndices = 40,                // Shift indices by N
        PruneIndices = 41,                // Convert index data to 1-bit, 2-bit or 4-bit
        ConvertDelta8 = 50,               // Convert image data to 8-bit deltas
        ConvertDelta16 = 51,              // Convert image data to 16-bit deltas
        DeltaImage = 55,                  // Calculate signed pixel difference between successive images
        CompressLZ10 = 60,                // Compress image data using LZ77 variant 10
        CompressLZ11 = 61,                // Compress image data using LZ77 variant 11
        CompressRLE = 65,                 // Compress image data using run-length-encoding
        CompressDXT = 70,                 // Compress image data using DXT
        CompressDXTV = 71,                // Compress image data using DXTV
        CompressGVID = 72,                // Compress image data using GVID
        PadPixelData = 80,                // Fill up image and map data with 0s to a multiple of N bytes
        SwapPixelDataToBGR = 81,          // Swap image data RGB->BGR
        ConvertPixelDataToRaw = 82,       // Convert image data to raw data
        PadColorMap = 90,                 // Fill up color map with 0s to a multiple of N colors
        SwapColorMapDataToBGR = 91,       // Swap color map data RGB->BGR
        ConvertColorMap = 92,             // Convert input color map to raw data
        PadColorMapData = 93,             // Fill up raw color map data with 0s to a multiple of N bytes
        EqualizeColorMaps = 94,           // Fill up all color maps with 0s to the size of the biggest color map
        ConvertColorMapDataToRaw = 95,    // Convert color map data to raw data
        CombineImageAndColorMapData = 100 // Concat image data and color map into one array
    };

    static constexpr uint8_t ProcessingTypeFinal = 128; // Marks the final processing step in an encoding sequence. Is ORed with Processing::Type

}
