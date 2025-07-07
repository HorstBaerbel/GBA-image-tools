#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Audio
{

    /// @brief Raw / compressed audio sample data
    using SampleData = std::variant<std::vector<int8_t>, std::vector<uint8_t>, std::vector<int16_t>, std::vector<uint16_t>, std::vector<float>>;

    /// @brief Get raw size of sample data
    /// @return Raw size of sample data
    auto rawSampleDataSize(const SampleData &samples) -> uint32_t;

    /// @brief Audio channel format identifier
    enum class ChannelFormat : uint8_t
    {
        Unknown = 0, // Bad format
        Mono = 1,    // Single channel
        Stereo = 2   // Two (planar) channels
    };

    /// @brief Channel format information
    struct ChannelFormatInfo
    {
        ChannelFormat format = ChannelFormat::Unknown;
        std::string description;   // Channel format decription as string
        std::string id;            // Short name identifier, e.g. "mono"
        uint32_t nrOfChannels = 0; // Number of channels for format
    };

    /// @brief Return channel format information
    auto formatInfo(ChannelFormat format) -> const ChannelFormatInfo &;

    /// @brief Find channel format from identifier
    auto findChannelFormat(const std::string &id) -> ChannelFormat;

    /// @brief Audio sample format identifier
    enum class SampleFormat : uint8_t
    {
        Unknown = 0,    // Bad, raw or compressed data
        Signed8 = 1,    // Signed 8-bit data
        Unsigned8 = 2,  // Unsigned 8-bit data
        Signed16 = 3,   // Signed 16-bit data
        Unsigned16 = 4, // Unsigned 16-bit data
        Float32 = 5     // 32-bit float data
    };

    /// @brief Sample format information
    struct SampleFormatInfo
    {
        SampleFormat format = SampleFormat::Unknown;
        std::string description;    // Sample format description as string
        std::string id;             // Short name identifier, e.g. "s8"
        uint32_t bitsPerSample = 0; // Bits per sample for format
        bool isSigned = false;      // True if the values are signed data types
    };

    /// @brief Return sample format information
    auto formatInfo(SampleFormat format) -> const SampleFormatInfo &;

    /// @brief Find sample format from identifier
    auto findSampleFormat(const std::string &id) -> SampleFormat;

    /// @brief Find a sample format based on the input info
    /// @param bitsPerSample Bits per sample for audio data
    /// @param isSigned If true the sample format has signed data, else unsigned data
    /// @return Most probable sample format or SampleFormat::Unknown
    auto findSampleFormat(uint32_t bitsPerSample, bool isSigned) -> SampleFormat;
}
