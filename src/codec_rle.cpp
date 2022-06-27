#include "codec_rle.h"

#include "colorhelpers.h"
#include "exception.h"

#define CMD_CODE_30 0x30 // RLE magic number

#define RLE_CHECK 1     // bits to check
#define RLE_MASK 0x80   // bits position:
                        // ((((1 << RLE_CHECK) - 1) << (8 - RLE_CHECK)
#define RLE_LENGTH 0x7F // length, (0xFF & ~RLE_MASK)

#define RLE_THRESHOLD 2 // max number of bytes to not encode
#define RLE_N 0x80      // max store, (RLE_LENGTH + 1)
#define RLE_F 0x82      // max coded, (RLE_LENGTH + RLE_THRESHOLD + 1)

auto RLE::encodeRLE(const std::vector<uint8_t> &data, bool /*vramCompatible*/) -> std::vector<uint8_t>
{
    uint8_t store[RLE_N];
    uint32_t len, count;

    uint32_t pak_len = 4 + data.size() + ((data.size() + RLE_N - 1) / RLE_N);
    std::vector<uint8_t> result(pak_len);
    auto pak_buffer = result.data();

    *(unsigned int *)pak_buffer = CMD_CODE_30 | (data.size() << 8);

    auto pak = pak_buffer + 4;
    auto raw = data.data();
    auto raw_end = data.data() + data.size();

    uint32_t store_len = 0;
    while (raw < raw_end)
    {
        for (len = 1; len < RLE_F; len++)
        {
            if (raw + len == raw_end)
                break;
            if (*(raw + len) != *raw)
                break;
        }

        if (len <= RLE_THRESHOLD)
            store[store_len++] = *raw++;

        if ((store_len == RLE_N) || (store_len && (len > RLE_THRESHOLD)))
        {
            *pak++ = store_len - 1;
            for (count = 0; count < store_len; count++)
                *pak++ = store[count];
            store_len = 0;
        }

        if (len > RLE_THRESHOLD)
        {
            *pak++ = RLE_MASK | (len - (RLE_THRESHOLD + 1));
            *pak++ = *raw;
            raw += len;
        }
    }
    if (store_len)
    {
        *pak++ = store_len - 1;
        for (count = 0; count < store_len; count++)
            *pak++ = store[count];
    }

    result.resize(pak - pak_buffer);
    return result;
}
