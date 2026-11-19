#include "testmacros.h"

#include "exception.h"
#include "processing/bitstream.h"
#include "processing/motionvectors.h"

#include <cstdint>
#include <vector>

using namespace MotionVectors;
using namespace Bitstream;

TEST_SUITE("MotionVectors")

// =============================================================================
// DeltaVectors table tests
// =============================================================================

TEST_CASE("DeltaVectors has correct number of entries")
{
    CATCH_REQUIRE(sizeof(DeltaVectors) / sizeof(DeltaVector) == (1 << MotionVectors::DeltaLongCodeLength));
}

TEST_CASE("DeltaVectors index 0 is zero vector")
{
    CATCH_REQUIRE(DeltaVectors[0].dx == 0);
    CATCH_REQUIRE(DeltaVectors[0].dy == 0);
}

TEST_CASE("DeltaVectors known values")
{
    // Index 1: {0, -1}
    CATCH_REQUIRE(DeltaVectors[1].dx == 0);
    CATCH_REQUIRE(DeltaVectors[1].dy == -1);
    // Index 2: {-1, 0}
    CATCH_REQUIRE(DeltaVectors[2].dx == -1);
    CATCH_REQUIRE(DeltaVectors[2].dy == 0);
    // Index 3: {1, 0}
    CATCH_REQUIRE(DeltaVectors[3].dx == 1);
    CATCH_REQUIRE(DeltaVectors[3].dy == 0);
    // Index 4: {0, 1}
    CATCH_REQUIRE(DeltaVectors[4].dx == 0);
    CATCH_REQUIRE(DeltaVectors[4].dy == 1);
    // Index 5: {-1, -1}
    CATCH_REQUIRE(DeltaVectors[5].dx == -1);
    CATCH_REQUIRE(DeltaVectors[5].dy == -1);
}

TEST_CASE("DeltaVectors all values within range")
{
    for (int i = 0; i < (1 << MotionVectors::DeltaLongCodeLength); ++i)
    {
        CATCH_REQUIRE(DeltaVectors[i].dx >= DeltaMinComponent);
        CATCH_REQUIRE(DeltaVectors[i].dx <= DeltaMaxComponent);
        CATCH_REQUIRE(DeltaVectors[i].dy >= DeltaMinComponent);
        CATCH_REQUIRE(DeltaVectors[i].dy <= DeltaMaxComponent);
    }
}

// =============================================================================
// writeMotionDelta tests
// =============================================================================

TEST_CASE("writeMotionDelta short code writes 6 bits")
{
    Writer writer;
    writeMotionDelta(writer, 0);
    CATCH_REQUIRE(writer.bitsPushed() == 6);
    writeMotionDelta(writer, (1 << MotionVectors::DeltaShortCodeLength) - 1);
    CATCH_REQUIRE(writer.bitsPushed() == 12);
}

TEST_CASE("writeMotionDelta short code writes correct bits")
{
    Writer writer0;
    writeMotionDelta(writer0, 0);
    writer0.flush();
    auto data0 = writer0.data();
    CATCH_REQUIRE(data0.size() == 1);
    CATCH_REQUIRE(data0[0] == 0x00);
    Writer writer1;
    writeMotionDelta(writer1, (1 << MotionVectors::DeltaShortCodeLength) - 1);
    writer1.flush();
    auto data1 = writer1.data();
    CATCH_REQUIRE(data1.size() == 1);
    CATCH_REQUIRE(data1[0] == (1 << MotionVectors::DeltaShortCodeLength) - 1);
}

TEST_CASE("writeMotionDelta long code writes 10 bits")
{
    Writer writer;
    writeMotionDelta(writer, (1 << MotionVectors::DeltaShortCodeLength));
    CATCH_REQUIRE(writer.bitsPushed() == 10);
    writeMotionDelta(writer, (1 << MotionVectors::DeltaLongCodeLength) - 1);
    CATCH_REQUIRE(writer.bitsPushed() == 20);
}

TEST_CASE("writeMotionDelta long code writes correct bits")
{
    Writer writer0;
    writeMotionDelta(writer0, (1 << MotionVectors::DeltaShortCodeLength));
    writer0.flush();
    auto data0 = writer0.data();
    // 10 bits: 32 | 512 = 544 = 0b1000100000
    // MSB-first: first byte = 10001000 = 0x88, remaining 2 bits = 00
    CATCH_REQUIRE(data0.size() == 2);
    CATCH_REQUIRE(data0[0] == 0x88);
    CATCH_REQUIRE(data0[1] == 0x00);
    Writer writer1;
    writeMotionDelta(writer1, (1 << MotionVectors::DeltaLongCodeLength) - 1);
    writer1.flush();
    auto data1 = writer1.data();
    // 10 bits: 511 | 512 = 1023 = 0b1111111111
    // First byte = 11111111 = 0xFF, remaining 2 bits = 11 right-aligned → 0x03
    CATCH_REQUIRE(data1.size() == 2);
    CATCH_REQUIRE(data1[0] == 0xFF);
    CATCH_REQUIRE(data1[1] == 0x03);
}

TEST_CASE("writeMotionDelta index 512 throws")
{
    Writer writer;
    CATCH_CHECK_THROWS_AS(writeMotionDelta(writer, (1 << MotionVectors::DeltaLongCodeLength)), std::runtime_error);
}

// =============================================================================
// readMotionDelta tests
// =============================================================================

TEST_CASE("readMotionDelta short code index")
{
    // Encode: prefix 0 + 5-bit index 0 = 0b000000
    std::vector<uint8_t> stream0 = {0x00};
    Reader reader0(stream0, 6);
    auto vec0 = readMotionDelta(reader0);
    CATCH_REQUIRE(vec0.dx == DeltaVectors[0].dx);
    CATCH_REQUIRE(vec0.dy == DeltaVectors[0].dy);
    // Encode: prefix 0 + 5-bit index 31 = 0b011111
    std::vector<uint8_t> stream1 = {(1 << MotionVectors::DeltaShortCodeLength) - 1};
    Reader reader1(stream1, 6);
    auto vec1 = readMotionDelta(reader1);
    CATCH_REQUIRE(vec1.dx == DeltaVectors[31].dx);
    CATCH_REQUIRE(vec1.dy == DeltaVectors[31].dy);
}

TEST_CASE("readMotionDelta long code index")
{
    // Encode: 32 | 512 = 544 = 0b1000100000
    // 10 bits MSB-first: 1,0,0,0,1,0,0,0,0,0
    std::vector<uint8_t> stream0 = {0x88, 0x00};
    Reader reader0(stream0, 10);
    auto vec0 = readMotionDelta(reader0);
    CATCH_REQUIRE(vec0.dx == DeltaVectors[32].dx);
    CATCH_REQUIRE(vec0.dy == DeltaVectors[32].dy);
    // Encode: 511 | 512 = 1023 = 0b1111111111
    // 10 bits MSB-first: 1,1,1,1,1,1,1,1,1,1
    // First byte: 0xFF (8 bits), second byte: 0x03 (2 bits in LSB)
    std::vector<uint8_t> stream1 = {0xFF, 0x03};
    Reader reader1(stream1, 10);
    auto vec1 = readMotionDelta(reader1);
    CATCH_REQUIRE(vec1.dx == DeltaVectors[511].dx);
    CATCH_REQUIRE(vec1.dy == DeltaVectors[511].dy);
}

// =============================================================================
// Round-trip tests
// =============================================================================

TEST_CASE("RoundTrip short code all indices 0 to DeltaShortCodeLength - 1")
{
    Writer writer;
    for (uint32_t i = 0; i < (1 << MotionVectors::DeltaShortCodeLength) - 1; ++i)
    {
        writeMotionDelta(writer, i);
    }
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    for (uint32_t i = 0; i < (1 << MotionVectors::DeltaShortCodeLength) - 1; ++i)
    {
        auto vec = readMotionDelta(reader);
        CATCH_REQUIRE(vec.dx == DeltaVectors[i].dx);
        CATCH_REQUIRE(vec.dy == DeltaVectors[i].dy);
    }
}

TEST_CASE("RoundTrip long code all indices DeltaShortCodeLength to DeltaLongCodeLength - 1")
{
    Writer writer;
    for (uint32_t i = (1 << MotionVectors::DeltaShortCodeLength); i < (1 << MotionVectors::DeltaLongCodeLength); ++i)
    {
        writeMotionDelta(writer, i);
    }
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    for (uint32_t i = (1 << MotionVectors::DeltaShortCodeLength); i < (1 << MotionVectors::DeltaLongCodeLength); ++i)
    {
        auto vec = readMotionDelta(reader);
        CATCH_REQUIRE(vec.dx == DeltaVectors[i].dx);
        CATCH_REQUIRE(vec.dy == DeltaVectors[i].dy);
    }
}

TEST_CASE("RoundTrip total bits for short codes")
{
    // 32 short codes × 6 bits each = 192 bits
    Writer writer;
    for (uint32_t i = 0; i < (1 << MotionVectors::DeltaShortCodeLength); ++i)
    {
        writeMotionDelta(writer, i);
    }
    CATCH_REQUIRE(writer.bitsPushed() == (1 << MotionVectors::DeltaShortCodeLength) * (MotionVectors::DeltaShortCodeLength + 1));
}

TEST_CASE("RoundTrip total bits for long codes")
{
    // 480 long codes × 10 bits each = 4800 bits
    Writer writer;
    for (uint32_t i = (1 << MotionVectors::DeltaShortCodeLength); i < (1 << MotionVectors::DeltaLongCodeLength); ++i)
    {
        writeMotionDelta(writer, i);
    }
    CATCH_REQUIRE(writer.bitsPushed() == ((1 << MotionVectors::DeltaLongCodeLength) - (1 << MotionVectors::DeltaShortCodeLength)) * (MotionVectors::DeltaLongCodeLength + 1));
}

TEST_CASE("RoundTrip mixed short and long codes")
{
    // Write a mix of short and long codes
    Writer writer0;
    uint32_t indices0[] = {0, 1, 2, 5, 10, 31, 32, 100, 255, 511};
    for (auto idx : indices0)
    {
        writeMotionDelta(writer0, idx);
    }
    writer0.flush();
    Reader reader0(writer0.data(), writer0.bitsPushed());

    for (auto idx : indices0)
    {
        auto vec = readMotionDelta(reader0);
        CATCH_REQUIRE(vec.dx == DeltaVectors[idx].dx);
        CATCH_REQUIRE(vec.dy == DeltaVectors[idx].dy);
    }
    // Write a mix of short and long codes
    Writer writer1;
    uint32_t indices1[] = {39, 66, 2, 0, 31, 32, 511, 12, 13};
    for (auto idx : indices1)
    {
        writeMotionDelta(writer1, idx);
    }
    writer1.flush();
    Reader reader1(writer1.data(), writer1.bitsPushed());
    for (auto idx : indices1)
    {
        auto vec = readMotionDelta(reader1);
        CATCH_REQUIRE(vec.dx == DeltaVectors[idx].dx);
        CATCH_REQUIRE(vec.dy == DeltaVectors[idx].dy);
    }
}

// =============================================================================
// Bitstream layout verification
// =============================================================================

TEST_CASE("Short code bit layout verified with reader")
{
    // Manually construct bitstream for short code index 7
    // Expected bits: 0 (prefix) + 00111 (index 7) = 0b000111
    std::vector<uint8_t> stream = {0x07};
    Reader reader(stream, 6);
    // Pop prefix bit
    CATCH_REQUIRE(reader.pop(1) == 0);
    // Pop index bits
    CATCH_REQUIRE(reader.pop(5) == 7);
}

TEST_CASE("Long code bit layout verified with reader")
{
    // Manually construct bitstream for long code index 100
    // 100 | 512 = 612 = 0b1001100100
    // MSB-first: first byte = 10011001 = 0x99, remaining 2 bits = 00
    std::vector<uint8_t> stream = {0x99, 0x00};
    Reader reader(stream, 10);
    // Pop prefix bit
    CATCH_REQUIRE(reader.pop(1) == 1);
    // Pop index bits
    CATCH_REQUIRE(reader.pop(9) == 100);
}

TEST_CASE("Read consumes correct number of bits for short code")
{
    // Write short code index 7 (6 bits) + 6 extra bits
    Writer writer;
    writeMotionDelta(writer, 7);
    writer.push(0x3F, 6);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    auto vec = readMotionDelta(reader);
    CATCH_REQUIRE(vec.dx == DeltaVectors[7].dx);
    CATCH_REQUIRE(vec.dy == DeltaVectors[7].dy);
    // Should have consumed 6 bits, leaving 6
    CATCH_REQUIRE(reader.bitsAvailable() == 6);
}

TEST_CASE("Read consumes correct number of bits for long code")
{
    // Write long code index 100 (10 bits) + 6 extra bits
    Writer writer;
    writeMotionDelta(writer, 100);
    writer.push(0x3F, 6);
    writer.flush();
    Reader reader(writer.data(), writer.bitsPushed());
    auto vec = readMotionDelta(reader);
    CATCH_REQUIRE(vec.dx == DeltaVectors[100].dx);
    CATCH_REQUIRE(vec.dy == DeltaVectors[100].dy);
    // Should have consumed 10 bits, leaving 6
    CATCH_REQUIRE(reader.bitsAvailable() == 6);
}
