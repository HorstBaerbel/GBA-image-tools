#pragma once

#include <cstdint>
#include <vector>

namespace Bitstream
{
    /// @brief Simple bit stream writer
    /// It writes bits in chunks of 32 bits MSB-first, symmetrical to Bitstream::Reader
    class Writer
    {
    public:
        /// @brief Append bits to stream
        /// @param bits Bits to write to stream. Must be in [1, 32].
        /// @param count Number of bits to write to stream
        auto push(const uint32_t bits, const std::size_t count) -> void;

        /// @brief Flush remaining bits to stream
        auto flush() -> void;

        /// @brief Return bit stream, appends 0 bits if needed
        /// @return Bit stream with 0s append up to a full byte
        /// @note You might want to flush() the stream before calling this function
        [[nodiscard]] auto data() const -> std::vector<uint8_t>;

        /// @brief Overall number of bits in stream and buffer
        /// @return Overall number of bits in stream and buffer
        [[nodiscard]] auto bitsPushed() const -> std::size_t;

    private:
        std::vector<uint8_t> m_stream; // Bit stream memory
        uint64_t m_bitBuffer = 0;      // Buffer holding bits
        int32_t m_bitsInBuffer = 0;    // Number of bits in buffer
    };

    // -------------------------------------------------------------------------------------------

    /// @brief Simple bit stream reader
    /// It read bits in chunks of 32 bits MSB-first, symmetrical to Bitstream::Writer
    class Reader
    {
    public:
        /// @brief Create bitstream reader from memory
        /// @param stream Input bit stream memory
        /// @param nrOfBits Number of bits available in stream
        Reader(const std::vector<uint8_t> &stream, std::size_t nrOfBits);

        /// @brief Pops count bits from the stream and advances the stream pointer
        /// @param count Number of bits to pop. Must be in [1, 32].
        /// @return Bits from the stream
        /// @throws std::runtime_exception if not enough bits in stream
        [[nodiscard]] auto pop(const std::size_t count) -> uint32_t;

        /// @brief Peek count bits from the stream without advancing the stream pointer
        /// @param count Number of bits to peek. Must be in [1, 32].
        /// @param allowEmpty If not enough bits in bit stream anymore, append zeros until count and do not error out
        /// @return Bits from the stream
        /// @throws std::runtime_exception if not enough bits in stream and allowEmpty == false
        [[nodiscard]] auto peek(const std::size_t count, const bool allowEmpty) -> uint32_t;

        /// @brief Overall number of bits left in stream and buffer
        /// @return Overall number of bits left in stream and buffer
        [[nodiscard]] auto bitsAvailable() const -> std::size_t;

    private:
        const std::vector<uint8_t> m_stream;  // Bit stream memory
        std::size_t m_streamBitsRead = 0;     // Bit of stream memory already read
        const std::size_t m_bitsInStream = 0; // Number of bits in stream memory
        uint64_t m_bitBuffer = 0;             // Buffer holding bits
        int32_t m_bitsInBuffer = 0;           // Number of bits in buffer
    };
}
