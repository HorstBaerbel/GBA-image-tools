#include "testmacros.h"

#include "io/textio.h"

#include <fstream>
#include <sstream>

using namespace IO;

TEST_SUITE("TextIO")

TEST_CASE("writeImageInfoToH basic")
{
    std::ostringstream oss;
    std::ofstream hFile("/tmp/test_textio_image.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint32_t> data = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    Text::writeImageInfoToH(hFile, "TEST_IMG", data, 2, 2, 8, 1, false);
    hFile.close();
    // Read back and verify content
    std::ifstream file("/tmp/test_textio_image.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define TEST_IMG_WIDTH 2") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TEST_IMG_HEIGHT 2") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TEST_IMG_BYTES_PER_IMAGE 8") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TEST_IMG_DATA_SIZE 4") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint32_t TEST_IMG_DATA[TEST_IMG_DATA_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_image.h");
}

TEST_CASE("writeImageInfoToH as tiles")
{
    std::ofstream hFile("/tmp/test_textio_tiles.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint32_t> data = {0x1111, 0x2222};
    Text::writeImageInfoToH(hFile, "TILE_DATA", data, 8, 8, 32, 4, true);
    hFile.close();
    std::ifstream file("/tmp/test_textio_tiles.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define TILE_DATA_WIDTH 8") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TILE_DATA_HEIGHT 8") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TILE_DATA_BYTES_PER_TILE 32") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TILE_DATA_DATA_SIZE 2") != std::string::npos);
    CATCH_REQUIRE(content.find("#define TILE_DATA_NR_OF_TILES 4") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint32_t TILE_DATA_DATA[TILE_DATA_DATA_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_tiles.h");
}

TEST_CASE("writeMapInfoToH")
{
    std::ofstream hFile("/tmp/test_textio_map.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint16_t> mapData = {0, 1, 2, 3, 4, 5, 6, 7};
    Text::writeMapInfoToH(hFile, "SCREEN_MAP", mapData, 1);
    hFile.close();
    std::ifstream file("/tmp/test_textio_map.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define SCREEN_MAP_MAPDATA_SIZE 8") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint16_t SCREEN_MAP_MAPDATA[SCREEN_MAP_MAPDATA_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_map.h");
}

TEST_CASE("writePaletteInfoToH uint16_t")
{
    std::ofstream hFile("/tmp/test_textio_palette.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint16_t> palette = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    Text::writePaletteInfoToH(hFile, "SPRITE", palette, 4, true, false);
    hFile.close();
    std::ifstream file("/tmp/test_textio_palette.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define SPRITE_PALETTE_LENGTH 4") != std::string::npos);
    CATCH_REQUIRE(content.find("#define SPRITE_PALETTE_SIZE 4") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint16_t SPRITE_PALETTE[SPRITE_PALETTE_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_palette.h");
}

TEST_CASE("writePaletteInfoToH uint32_t")
{
    std::ofstream hFile("/tmp/test_textio_palette32.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint32_t> palette = {0x12345678, 0xDEADBEEF};
    Text::writePaletteInfoToH(hFile, "BG", palette, 2, true, false);
    hFile.close();
    std::ifstream file("/tmp/test_textio_palette32.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define BG_PALETTE_LENGTH 2") != std::string::npos);
    CATCH_REQUIRE(content.find("#define BG_PALETTE_SIZE 2") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint32_t BG_PALETTE[BG_PALETTE_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_palette32.h");
}

TEST_CASE("writePaletteInfoToH multiple palettes")
{
    std::ofstream hFile("/tmp/test_textio_multi_palette.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint16_t> palette = {0x1111, 0x2222, 0x3333, 0x4444};
    Text::writePaletteInfoToH(hFile, "MULTI", palette, 2, false, false);
    hFile.close();
    std::ifstream file("/tmp/test_textio_multi_palette.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Should include PALETTE_START declaration when singleColorMap is false
    CATCH_REQUIRE(content.find("#define MULTI_PALETTE_LENGTH 2") != std::string::npos);
    CATCH_REQUIRE(content.find("#define MULTI_PALETTE_SIZE 4") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint32_t MULTI_PALETTE_START[MULTI_NR_OF_IMAGES];") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint16_t MULTI_PALETTE[MULTI_PALETTE_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_multi_palette.h");
}

TEST_CASE("writeTableInfoToH")
{
    std::ofstream hFile("/tmp/test_textio_table.h");
    CATCH_REQUIRE(hFile.is_open());
    std::vector<uint32_t> table = {10, 20, 30, 40};
    Text::writeTableInfoToH(hFile, "LOOKUP", table, 4);
    hFile.close();
    std::ifstream file("/tmp/test_textio_table.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define LOOKUP_LENGTH 4") != std::string::npos);
    CATCH_REQUIRE(content.find("#define LOOKUP_SIZE 4") != std::string::npos);
    CATCH_REQUIRE(content.find("extern const uint32_t LOOKUP[LOOKUP_SIZE];") != std::string::npos);
    std::remove("/tmp/test_textio_table.h");
}

TEST_CASE("writeCompressionInfoToH")
{
    std::ofstream hFile("/tmp/test_textio_comp.h");
    CATCH_REQUIRE(hFile.is_open());
    Text::writeCompressionInfoToH(hFile, "COMPRESSED", 4096);
    hFile.close();
    std::ifstream file("/tmp/test_textio_comp.h");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#define COMPRESSED_DECOMPRESSION_BUFFER_SIZE 4096") != std::string::npos);
    std::remove("/tmp/test_textio_comp.h");
}

TEST_CASE("writeImageDataToC")
{
    std::ofstream cFile("/tmp/test_textio_image.c");
    CATCH_REQUIRE(cFile.is_open());
    std::vector<uint32_t> data = {0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222};
    Text::writeImageDataToC(cFile, "TEST_IMG", "test_image", data);
    cFile.close();
    std::ifstream file("/tmp/test_textio_image.c");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("#include \"test_image.h\"") != std::string::npos);
    CATCH_REQUIRE(content.find("const _Alignas(4) uint32_t TEST_IMG_DATA[TEST_IMG_DATA_SIZE] = {") != std::string::npos);
    CATCH_REQUIRE(content.find("0x12345678, 0x9abcdef0, 0x11111111, 0x22222222};") != std::string::npos);
    std::remove("/tmp/test_textio_image.c");
}

TEST_CASE("writeMapDataToC")
{
    std::ofstream cFile("/tmp/test_textio_map.c");
    CATCH_REQUIRE(cFile.is_open());
    std::vector<uint16_t> mapData = {0, 1, 2, 3, 4, 5, 6, 7};
    Text::writeMapDataToC(cFile, "SCREEN_MAP", mapData);
    cFile.close();
    std::ifstream file("/tmp/test_textio_map.c");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("const _Alignas(4) uint16_t SCREEN_MAP_MAPDATA[SCREEN_MAP_MAPDATA_SIZE] = {") != std::string::npos);
    CATCH_REQUIRE(content.find("0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007};") != std::string::npos);
    std::remove("/tmp/test_textio_map.c");
}

TEST_CASE("writePaletteDataToC uint16_t")
{
    std::ofstream cFile("/tmp/test_textio_palette.c");
    CATCH_REQUIRE(cFile.is_open());
    std::vector<uint16_t> palette = {0x1234, 0x5678, 0x9ABC};
    Text::writePaletteDataToC(cFile, "SPRITE", palette);
    cFile.close();
    std::ifstream file("/tmp/test_textio_palette.c");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("const _Alignas(4) uint16_t SPRITE_PALETTE[SPRITE_PALETTE_SIZE] = {") != std::string::npos);
    CATCH_REQUIRE(content.find("0x1234, 0x5678, 0x9abc};") != std::string::npos);
    std::remove("/tmp/test_textio_palette.c");
}

TEST_CASE("writeTableDataToC")
{
    std::ofstream cFile("/tmp/test_textio_table.c");
    CATCH_REQUIRE(cFile.is_open());
    std::vector<uint8_t> table = {0x01, 0x02, 0x03, 0x04};
    Text::writeTableDataToC(cFile, "SMALL_TABLE", table);
    cFile.close();
    std::ifstream file("/tmp/test_textio_table.c");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CATCH_REQUIRE(content.find("const _Alignas(4) uint8_t SMALL_TABLE[SMALL_TABLE_SIZE] = {") != std::string::npos);
    CATCH_REQUIRE(content.find("0x01, 0x02, 0x03, 0x04};") != std::string::npos);
    std::remove("/tmp/test_textio_table.c");
}
