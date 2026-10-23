#include "bitstream.h"

#include "exception.h"

namespace Bitstream
{
    auto Writer::push(const uint32_t bits, const std::size_t count) -> void
    {
        REQUIRE(count > 0 && count <= 32, std::runtime_error, "count must be in [1, 32]");
        // store bits to buffer
        m_bitBuffer = (m_bitBuffer << count) | (static_cast<uint64_t>(bits) & ((1UL << count) - 1));
        m_bitsInBuffer += count;
        // if we have more than 32 bits, store buffer to stream
        while (m_bitsInBuffer >= 32)
        {
            m_stream.resize(m_stream.size() + 4);
            auto streamPtr = reinterpret_cast<uint32_t *>(m_stream.data() + m_stream.size() - 4);
            *streamPtr++ = static_cast<uint32_t>(m_bitBuffer >> (m_bitsInBuffer - 32));
            m_bitsInBuffer -= 32;
        }
    }

    auto Writer::flush() -> void
    {
        if (m_bitsInBuffer <= 0)
        {
            return;
        }
        // check how many bytes we have in buffer
        const auto bytesInBuffer = (m_bitsInBuffer + 7) / 8;
        // store bits in buffer byte wise
        while (m_bitsInBuffer > 0)
        {
            // store byte, but mask out everything else except remaining bits
            const auto shift = m_bitsInBuffer >= 8 ? m_bitsInBuffer - 8 : 0;
            m_stream.push_back(static_cast<uint8_t>((m_bitBuffer >> shift) & ((1UL << m_bitsInBuffer) - 1)));
            m_bitsInBuffer -= 8;
        }
    }

    auto Writer::data() const -> std::vector<uint8_t>
    {
        return m_stream;
    }

    auto Writer::bitsPushed() const -> std::size_t
    {
        return m_stream.size() * 8 + m_bitsInBuffer;
    }

    // -------------------------------------------------------------------------------------------

    Reader::Reader(const std::vector<uint8_t> &stream, std::size_t nrOfBits)
        : m_stream(stream), m_streamBitsRead(0), m_bitsInStream(nrOfBits)
    {
        REQUIRE(!m_stream.empty(), std::runtime_error, "Bit stream can not be empty");
        REQUIRE(m_bitsInStream <= m_stream.size() * 8, std::runtime_error, "Number of bits must be <= bit stream size");
    }

    auto Reader::pop(const std::size_t count) -> uint32_t
    {
        uint32_t bits = peek(count, false);
        // reduce available bits and mask out everything except remaining bits
        m_bitsInBuffer -= count;
        m_bitBuffer &= ((1UL << m_bitsInBuffer) - 1);
        return bits;
    }

    auto Reader::peek(const std::size_t count, const bool allowEmpty) -> uint32_t
    {
        REQUIRE(count > 0 && count <= 32, std::runtime_error, "count must be in [1, 32]");
        // do we need to read data from the stream?
        while (m_bitsInBuffer < count && m_bitsInStream > m_streamBitsRead)
        {
            if (m_bitsInStream - m_streamBitsRead >= 32)
            {
                // read 32 bits
                auto streamPtr32 = reinterpret_cast<const uint32_t *>(m_stream.data() + m_streamBitsRead / 8);
                m_bitBuffer = (m_bitBuffer << 32) | *streamPtr32;
                m_bitsInBuffer += 32;
                m_streamBitsRead += 32;
            }
            else if (m_bitsInStream - m_streamBitsRead >= 8)
            {
                // read 8 bits
                auto streamPtr8 = m_stream.data() + m_streamBitsRead / 8;
                m_bitBuffer = (m_bitBuffer << 8) | *streamPtr8;
                m_bitsInBuffer += 8;
                m_streamBitsRead += 8;
            }
            else
            {
                // read m_bitsInStream - m_streamBitsRead bits
                auto streamPtr8 = m_stream.data() + m_streamBitsRead / 8;
                const auto bitsLeft = m_bitsInStream - m_streamBitsRead;
                m_bitBuffer = (m_bitBuffer << bitsLeft) | *streamPtr8;
                m_bitsInBuffer += bitsLeft;
                m_streamBitsRead += bitsLeft;
            }
        }
        // check if we as many bits as requested
        if (m_bitsInBuffer < count)
        {
            // no. if empty bots are allowed, clear the unused MSB
            if (allowEmpty)
            {
                return m_bitBuffer & ((1UL << m_bitsInBuffer) - 1);
            }
            THROW(std::runtime_error, "Not enough bits in stream. " << count << " requested, " << m_bitsInBuffer << " available");
        }
        return (m_bitBuffer >> (m_bitsInBuffer - count)) & ((1UL << count) - 1);
    }

    auto Reader::bitsAvailable() const -> std::size_t
    {
        return (m_bitsInStream > m_streamBitsRead ? m_bitsInStream - m_streamBitsRead : 0) + m_bitsInBuffer;
    }
}
