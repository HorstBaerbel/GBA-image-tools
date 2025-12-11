#include "lz4.h"
#include "lz4_constants.h"

namespace Compression
{
    IWRAM_FUNC void LZ4UnCompWrite8bit(const uint32_t *data, uint32_t *dst)
    {
        // read data header and skipt to data
        const uint32_t header = *data++;
        if ((header & 0xFF) != Lz4Constants::TYPE_MARKER)
        {
            return;
        }
        int32_t uncompressedSize = (header >> 8);
        if (uncompressedSize == 0)
        {
            return;
        }
        // decompress data
        auto data8 = reinterpret_cast<const uint8_t *>(data);
        auto dst8 = reinterpret_cast<uint8_t *>(dst);
        while (uncompressedSize > 0)
        {
            // read token
            uint8_t token = *data8++;
            // copy literal if any
            uint32_t literalLength = (token & Lz4Constants::LITERAL_LENGTH_MASK) >> 4;
            if (literalLength > 0)
            {
                // read extra literal length
                if (literalLength == 15)
                {
                    uint8_t extraLength = 0;
                    do
                    {
                        extraLength = *data8++;
                        literalLength += extraLength;
                    } while (extraLength == 255);
                }
                // copy literals following the length
                for (uint32_t i = 0; i < literalLength; ++i)
                {
                    dst8[i] = data8[i];
                }
                data8 += literalLength;
                dst8 += literalLength;
                uncompressedSize -= literalLength;
            }
            // copy match if any
            uint32_t matchLength = token & Lz4Constants::MATCH_LENGTH_MASK;
            if (matchLength > 0)
            {
                // read match offset
                int32_t matchOffset = (uint32_t(*data8++) << 8);
                matchOffset |= uint32_t(*data8++);
                // read extra match length
                if (matchLength == 15)
                {
                    uint8_t extraLength = 0;
                    do
                    {
                        extraLength = *data8++;
                        matchLength += extraLength;
                    } while (extraLength == 255);
                }
                matchLength += (Lz4Constants::MIN_MATCH_LENGTH - 1);
                // copy match from current byte - matchOffset until current byte - matchOffset + matchLength
                auto match8 = const_cast<const uint8_t *>(dst8) - matchOffset;
                // note that we have to allow overlapping copies
                for (uint32_t i = 0; i < matchLength; ++i)
                {
                    dst8[i] = match8[i];
                }
                dst8 += matchLength;
                uncompressedSize -= matchLength;
            }
        }
    }
}
