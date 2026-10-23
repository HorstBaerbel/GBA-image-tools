#include "testmacros.h"

#include "processing/bitstream.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace Bitstream;

TEST_SUITE("Bitstream")

// =============================================================================
// Writer tests
// =============================================================================

TEST_CASE("Writer data empty initially")
{
    Writer writer;
    auto data = writer.data();
    CATCH_REQUIRE(data.empty());
}

TEST_CASE("Writer push single bit")
{
    Writer writer;
    writer.push(1, 1);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 1);
    CATCH_REQUIRE(data[0] == 0x01);
}

TEST_CASE("Writer push single bit zero")
{
    Writer writer;
    writer.push(0, 1);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 1);
    CATCH_REQUIRE(data[0] == 0x00);
}

TEST_CASE("Writer push multiple bits")
{
    Writer writer;
    writer.push(1, 3);
    writer.push(3, 4);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 1);
    CATCH_REQUIRE(data[0] == 0b0010011);
}

TEST_CASE("Writer multiple small writes")
{
    Writer writer;
    // Write bits: 1, 0, 1, 1, 0, 0, 1, 0 => 0b10110010 = 0xB2
    writer.push(1, 1);
    writer.push(0, 1);
    writer.push(1, 1);
    writer.push(1, 1);
    writer.push(0, 1);
    writer.push(0, 1);
    writer.push(1, 1);
    writer.push(0, 1);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 1);
    CATCH_REQUIRE(data[0] == 0xB2);
}

TEST_CASE("Writer push byte")
{
    Writer writer;
    writer.push(0xAB, 8);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 1);
    CATCH_REQUIRE(data[0] == 0xAB);
}

TEST_CASE("Writer push 32 bits")
{
    Writer writer;
    writer.push(0xDEADBEEF, 32);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 4);
    // MSB-first: first byte is the most significant
    CATCH_REQUIRE(data[0] == 0xEF);
    CATCH_REQUIRE(data[1] == 0xBE);
    CATCH_REQUIRE(data[2] == 0xAD);
    CATCH_REQUIRE(data[3] == 0xDE);
}

TEST_CASE("Writer push count throws")
{
    Writer writer;
    CATCH_CHECK_THROWS_AS(writer.push(0, 0), std::runtime_error);
    CATCH_CHECK_THROWS_AS(writer.push(0, 33), std::runtime_error);
}

TEST_CASE("Writer flush")
{
    Writer writer;
    writer.push(0xFF, 8);
    CATCH_REQUIRE(writer.data().size() == 0);
    writer.flush();
    CATCH_REQUIRE(writer.data().size() == 1);
}

TEST_CASE("Writer flush partial bytes")
{
    Writer writer;
    writer.push(0x6AA, 12);
    writer.flush();
    auto data = writer.data();
    CATCH_REQUIRE(data.size() == 2);
    CATCH_REQUIRE(data[0] == 0x6A);
    CATCH_REQUIRE(data[1] == 0x0A);
}

TEST_CASE("Writer push across 32bit boundary")
{
    Writer writer;
    writer.push(0xFFFFF, 20);
    writer.push(0x1234, 16);
    writer.flush();
    auto data = writer.data();
    // the stream contains 32 bit first (swapped), then the remaining 4 bit
    CATCH_REQUIRE(data.size() == 5);
    CATCH_REQUIRE(data == std::vector<uint8_t>({0x23, 0xF1, 0xFF, 0xFF, 0x04}));
}

TEST_CASE("Writer bits written")
{
    Writer writer;
    CATCH_REQUIRE(writer.bitsPushed() == 0);
    writer.push(0xFFFF0F, 24);
    CATCH_REQUIRE(writer.bitsPushed() == 24);
    writer.push(0x0123, 14);
    CATCH_REQUIRE(writer.bitsPushed() == 38);
    writer.flush();
    CATCH_REQUIRE(writer.bitsPushed() == 38);
}

// =============================================================================
// Reader tests
// =============================================================================

TEST_CASE("Reader empty stream throws")
{
    std::vector<uint8_t> empty;
    CATCH_CHECK_THROWS_AS(Reader(empty, 0), std::runtime_error);
}

TEST_CASE("Reader pop single bit")
{
    std::vector<uint8_t> stream = {0x80}; // 10000000
    Reader reader(stream, 8);
    CATCH_REQUIRE(reader.pop(1) == 1);
    CATCH_REQUIRE(reader.pop(1) == 0);
    CATCH_REQUIRE(reader.pop(1) == 0);
}

TEST_CASE("Reader pop byte")
{
    std::vector<uint8_t> stream = {0xAB};
    Reader reader(stream, 8);
    CATCH_REQUIRE(reader.pop(8) == 0xAB);
}

TEST_CASE("Reader pop 32 bits")
{
    std::vector<uint8_t> stream = {0xDE, 0xAD, 0xBE, 0xEF};
    Reader reader(stream, 32);
    CATCH_REQUIRE(reader.pop(32) == 0xEFBEADDE);
}

TEST_CASE("Reader pop count throws")
{
    std::vector<uint8_t> stream = {0xFF};
    Reader reader(stream, 8);
    CATCH_CHECK_THROWS_AS(reader.pop(0), std::runtime_error);
    CATCH_CHECK_THROWS_AS(reader.pop(33), std::runtime_error);
}

TEST_CASE("Reader pop not enough bits throws")
{
    std::vector<uint8_t> stream = {0xFF};
    Reader reader(stream, 0);
    CATCH_CHECK_THROWS_AS(reader.pop(9), std::runtime_error);
}

TEST_CASE("Reader peek does not advance")
{
    std::vector<uint8_t> stream = {0xAB, 0xCD};
    Reader reader(stream, 16);
    // Peek 8 bits - should see 0xAB
    CATCH_REQUIRE(reader.peek(8, false) == 0xAB);
    // Peek again - should still see 0xAB (not advanced)
    CATCH_REQUIRE(reader.peek(8, false) == 0xAB);
    // Now pop - should get 0xAB and advance
    CATCH_REQUIRE(reader.pop(8) == 0xAB);
    // Next peek should see 0xCD
    CATCH_REQUIRE(reader.peek(8, false) == 0xCD);
}

TEST_CASE("Reader peek allow empty pads with zeros")
{
    std::vector<uint8_t> stream = {0x80}; // 10000000 - only 8 bits
    Reader reader(stream, 8);
    // Peek 16 bits with allowEmpty - should pad with zeros
    CATCH_REQUIRE(reader.peek(16, true) == 0x0080);
}

TEST_CASE("Reader peek not enough bits no allow empty throws")
{
    std::vector<uint8_t> stream = {0xFF}; // only 8 bits
    Reader reader(stream, 8);
    CATCH_CHECK_THROWS_AS(reader.peek(16, false), std::runtime_error);
}

TEST_CASE("Reader peek count throws")
{
    std::vector<uint8_t> stream = {0xFF};
    Reader reader(stream, 8);
    CATCH_CHECK_THROWS_AS(reader.peek(0, false), std::runtime_error);
    CATCH_CHECK_THROWS_AS(reader.peek(33, false), std::runtime_error);
}

TEST_CASE("Reader bitsAvailable initial")
{
    std::vector<uint8_t> stream = {0xFF, 0xFF, 0xFF};
    Reader reader(stream, 24);
    CATCH_REQUIRE(reader.bitsAvailable() == 24);
}

TEST_CASE("Reader bitsAvailable decreases after pop")
{
    std::vector<uint8_t> stream = {0xFF, 0xFF, 0xFF};
    Reader reader(stream, 24);
    CATCH_REQUIRE(reader.bitsAvailable() == 24);
    CATCH_REQUIRE(reader.pop(8) == 0xFF);
    CATCH_REQUIRE(reader.bitsAvailable() == 16);
    CATCH_REQUIRE(reader.pop(8) == 0xFF);
    CATCH_REQUIRE(reader.bitsAvailable() == 8);
    CATCH_REQUIRE(reader.pop(8) == 0xFF);
    CATCH_REQUIRE(reader.bitsAvailable() == 0);
}

TEST_CASE("Reader pop multiple bytes")
{
    std::vector<uint8_t> stream = {0x12, 0x34, 0x56, 0x78};
    Reader reader(stream, 32);
    CATCH_REQUIRE(reader.pop(8) == 0x78);
    CATCH_REQUIRE(reader.pop(8) == 0x56);
    CATCH_REQUIRE(reader.pop(8) == 0x34);
    CATCH_REQUIRE(reader.pop(8) == 0x12);
}

TEST_CASE("Reader pop crossing byte boundary")
{
    // Stream: 0xAB 0xCD => bits: 10101011 11001101
    // Pop 12 bits: 101010111100 = 0xABC
    std::vector<uint8_t> stream = {0xAB, 0xCD};
    Reader reader(stream, 16);
    CATCH_REQUIRE(reader.pop(12) == 0xABC);
    // Remaining 4 bits: 1101 = 0xD
    CATCH_REQUIRE(reader.pop(4) == 0xD);
}

TEST_CASE("Reader peek crossing byte boundary")
{
    // Stream: 0xAB 0xCD => bits: 10101011 11001101
    // Peek 12 bits: 101010111100 = 0xAC
    std::vector<uint8_t> stream = {0xAB, 0xCD};
    Reader reader(stream, 16);
    CATCH_REQUIRE(reader.peek(12, false) == 0xABC);
    // Should not have advanced
    CATCH_REQUIRE(reader.bitsAvailable() == 16);
    CATCH_REQUIRE(reader.pop(12) == 0xABC);
    // Remaining 4 bits: 1101 = 0xD
    CATCH_REQUIRE(reader.peek(4, false) == 0xD);
    CATCH_REQUIRE(reader.bitsAvailable() == 4);
}

// =============================================================================
// Round-trip tests (Writer -> Reader)
// =============================================================================

TEST_CASE("RoundTrip single byte")
{
    Writer writer;
    writer.push(0xDE, 8);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(8) == 0xDE);
}

TEST_CASE("RoundTrip multiple bytes")
{
    Writer writer;
    writer.push(0x12, 8);
    writer.push(0x34, 8);
    writer.push(0x56, 8);
    writer.push(0x78, 8);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(8) == 0x12);
    CATCH_REQUIRE(reader.pop(8) == 0x34);
    CATCH_REQUIRE(reader.pop(8) == 0x56);
    CATCH_REQUIRE(reader.pop(8) == 0x78);
}

TEST_CASE("RoundTrip partial bits")
{
    Writer writer;
    writer.push(0b101101010110, 12);
    writer.push(0b0111010, 7);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(12) == 0b101101010110);
    CATCH_REQUIRE(reader.pop(7) == 0b0111010);
}

TEST_CASE("RoundTrip 32 bit value")
{
    Writer writer;
    writer.push(0xDEADBEEF, 32);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(32) == 0xDEADBEEF);
}

TEST_CASE("RoundTrip mixed bit counts")
{
    Writer writer;
    writer.push(1, 1);       // 1 bit
    writer.push(0xAB, 8);    // 8 bits
    writer.push(0x5, 3);     // 3 bits
    writer.push(0x1234, 16); // 16 bits
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(1) == 1);
    CATCH_REQUIRE(reader.pop(8) == 0xAB);
    CATCH_REQUIRE(reader.pop(3) == 0x5);
    CATCH_REQUIRE(reader.pop(16) == 0x1234);
}

TEST_CASE("RoundTrip crossing 32 bit boundary")
{
    // Write 20 + 16 = 36 bits (crosses 32-bit boundary)
    Writer writer;
    writer.push(0xFFFFF, 20);
    writer.push(0x1234, 16);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(20) == 0xFFFFF);
    CATCH_REQUIRE(reader.pop(16) == 0x1234);
}

TEST_CASE("RoundTrip random data")
{
    // Larger round-trip test with more data
    std::mt19937 rng(123);
    Writer writer;
    const int numChunks = 200;
    std::vector<std::pair<uint32_t, std::size_t>> written;
    for (int i = 0; i < numChunks; ++i)
    {
        auto count = static_cast<std::size_t>(rng() % 32) + 1; // 1 to 32
        auto bits = rng() & ((1ULL << count) - 1);
        writer.push(bits, count);
        written.push_back({bits, count});
    }
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    for (auto &[bits, count] : written)
    {
        CATCH_REQUIRE(reader.pop(count) == bits);
    }
}

TEST_CASE("RoundTrip all zero bits")
{
    Writer writer;
    writer.push(0, 8);
    writer.push(0, 16);
    writer.push(0, 32);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(8) == 0);
    CATCH_REQUIRE(reader.pop(16) == 0);
    CATCH_REQUIRE(reader.pop(32) == 0);
}

TEST_CASE("RoundTrip all one bits")
{
    Writer writer;
    writer.push(0xFF, 8);
    writer.push(0xFFFF, 16);
    writer.push(0xFFFFFFFF, 32);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    CATCH_REQUIRE(reader.pop(8) == 0xFF);
    CATCH_REQUIRE(reader.pop(16) == 0xFFFF);
    CATCH_REQUIRE(reader.pop(32) == 0xFFFFFFFF);
}
