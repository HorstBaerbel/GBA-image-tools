#include "videohelp.h"

#include "memory/memory.h"
#include "palette.h"
#include "print/itoa.h"
#include "sprites.h"
#include "tiles.h"

#include "./data/videohelp.h"

namespace VideoHelp
{
    struct SymbolInfo
    {
        bool enabled = true;
        uint16_t spriteIndex = 0;
        uint16_t spriteCount = 0;
    };

    EWRAM_DATA Sprites::Sprite2D *sprites = nullptr;
    EWRAM_DATA uint16_t spritesInUse = 0;
    EWRAM_DATA uint16_t spritePaletteIndex = 0;
    EWRAM_DATA SymbolInfo symbols[6]{};

    auto Setup(uint32_t spriteStartIndex, uint32_t tileStartIndex, uint16_t paletteIndex) -> void
    {
        // disable sprites
        REG_DISPCNT &= ~OBJ_ON;
        // clear all sprites
        Sprites::clearOAM();
        // build sprite color palette #0
        spritePaletteIndex = paletteIndex;
        Palette::Sprite16[spritePaletteIndex][0] = 0;
        Memory::memset16(&Palette::Sprite16[spritePaletteIndex][1], 0x7fff, 15);
        // allocate sprites
        spritesInUse = VIDEOHELP_NR_OF_TILES;
        sprites = Memory::malloc_EWRAM<Sprites::Sprite2D>(spritesInUse);
        Sprites::create(sprites, spritesInUse, spriteStartIndex, tileStartIndex, Sprites::SizeCode::Size64x32, Sprites::ColorDepth::Depth16, spritePaletteIndex);
        // play
        symbols[static_cast<uint32_t>(Symbol::Play)].spriteIndex = 0;
        symbols[static_cast<uint32_t>(Symbol::Play)].spriteCount = 2;
        sprites[0].x = 30;
        sprites[0].y = 22;
        sprites[1].x = 30 + 64;
        sprites[1].y = 22;
        // stop
        symbols[static_cast<uint32_t>(Symbol::Stop)].spriteIndex = 2;
        symbols[static_cast<uint32_t>(Symbol::Stop)].spriteCount = 1;
        sprites[2].x = 164;
        sprites[2].y = 22;
        // help
        symbols[static_cast<uint32_t>(Symbol::Help)].spriteIndex = 3;
        symbols[static_cast<uint32_t>(Symbol::Help)].spriteCount = 1;
        sprites[3].x = 32;
        sprites[3].y = 66;
        // subtitles
        symbols[static_cast<uint32_t>(Symbol::Subtitles)].spriteIndex = 4;
        symbols[static_cast<uint32_t>(Symbol::Subtitles)].spriteCount = 2;
        sprites[4].x = 102;
        sprites[4].y = 66;
        sprites[5].x = 102 + 64;
        sprites[5].y = 66;
        // previous
        symbols[static_cast<uint32_t>(Symbol::Previous)].spriteIndex = 6;
        symbols[static_cast<uint32_t>(Symbol::Previous)].spriteCount = 2;
        sprites[6].x = 42;
        sprites[6].y = 106;
        sprites[7].x = 42 + 64;
        sprites[7].y = 106;
        // next
        symbols[static_cast<uint32_t>(Symbol::Next)].spriteIndex = 8;
        symbols[static_cast<uint32_t>(Symbol::Next)].spriteCount = 1;
        sprites[8].x = 152;
        sprites[8].y = 106;
        // copy data to tile map
        auto spriteTile = Sprites::TILE_INDEX_TO_MEM<uint32_t>(tileStartIndex);
        Memory::memcpy32(spriteTile, VIDEOHELP_DATA, VIDEOHELP_DATA_SIZE);
        // enable sprites
        REG_DISPCNT |= OBJ_ON | OBJ_1D_MAP;
    }

    auto SpritesInUse() -> uint32_t
    {
        return sprites != nullptr ? spritesInUse : 0;
    }

    auto TilesInUse() -> uint32_t
    {
        if (sprites != nullptr && spritesInUse > 0)
        {
            const auto tileStart = sprites[0].tileIndex;
            const auto &sprite = sprites[spritesInUse - 1];
            return sprite.tileIndex + Tiles::TileCountForSizeCode[static_cast<uint32_t>(sprite.size)] - tileStart;
        }
        return 0;
    }

    auto SetSymbolEnabled(Symbol symbol, bool enable) -> void
    {
        symbols[static_cast<uint32_t>(symbol)].enabled = enable;
    }

    auto SetVisible(bool visible) -> void
    {
        if (visible)
        {
            // enable mosaic and brightness decrease effect for background 2
            REG_BG2CNT |= 0x0040;
            REG_BLDCNT |= 0x00C4;
            // set brightness decrease factor to 12/16
            REG_BLDY = 12;
            // set mosaic to x=4 and y=4
            REG_MOSAIC = (4 << 4) | 4;
            // show sprites
            for (uint32_t i = 0; i < (sizeof(symbols) / sizeof(SymbolInfo)); ++i)
            {
                auto &symbol = symbols[i];
                for (uint32_t j = 0; j < symbol.spriteCount; ++j)
                {
                    sprites[symbol.spriteIndex + j].visible = symbol.enabled;
                    sprites[symbol.spriteIndex + j].priority = symbol.enabled ? Sprites::Priority::Prio0 : Sprites::Priority::Prio3;
                }
            }
        }
        else
        {
            // hide sprites
            for (uint32_t i = 0; i < spritesInUse; ++i)
            {
                auto &sprite = sprites[i];
                sprite.visible = false;
                sprite.priority = Sprites::Priority::Prio3;
            }
            // disable mosaic and brightness decrease effect for background 2
            REG_BG2CNT &= ~0x0040;
            REG_BLDCNT &= ~0x00C4;
            // set brightness decrease factor to 0/16
            REG_BLDY = 0;
            // clear mosaic setting
            REG_MOSAIC &= 0xFF00;
        }
    }

    auto SetColor(color16 textColor) -> void
    {
        Memory::memset16(&Palette::Sprite16[spritePaletteIndex][1], textColor, 15);
    }

    auto Present() -> void
    {
        Sprites::copyToOAM(sprites, 0, spritesInUse);
    }

    auto Cleanup() -> void
    {
        SetVisible(false);
        REG_DISPCNT &= ~OBJ_ON;
        spritesInUse = 0;
        spritePaletteIndex = 0;
        Sprites::clearOAM();
        Memory::free(sprites);
    }
}
