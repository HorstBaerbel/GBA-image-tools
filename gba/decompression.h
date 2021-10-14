#pragma once


// Decompression Read/Write Variants
// ReadNormal:      Fast (src must be memory mapped)
// Write8bitUnits:  Fast (dest must support 8bit writes, e.g. must use WRAM, not VRAM)
// Write16bitUnits: Slow (dest must be halfword-aligned) (ok to use VRAM)
// See also: http://problemkaputt.de/gbatek.htm#biosfunctions

namespace Decompression
{

void LZ77UnCompReadNormalWrite8bit(const void *source, void *dest);
void LZ77UnCompReadNormalWrite16bit(const void *source, void *dest);
void HuffUnCompReadNormal(const void *source, void *dest);
void RLUnCompReadNormalWrite8bit(const void *source, void *dest);
void RLUnCompReadNormalWrite16bit(const void *source, void *dest);
void Diff8bitUnFilterWrite8bit(const void *source, void *dest);
void Diff8bitUnFilterWrite16bit(const void *source, void *dest);
void Diff16bitUnFilter(const void *source, void *dest);

}
