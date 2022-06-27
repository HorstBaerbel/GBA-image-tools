#include "codec_lzss.h"

#include "colorhelpers.h"
#include "exception.h"

#define CMD_CODE_10 0x10 // LZSS magic number

#define LZS_SHIFT 1   // bits to shift
#define LZS_MASK 0x80 // bits to check:
                      // ((((1 << LZS_SHIFT) - 1) << (8 - LZS_SHIFT)

#define LZS_THRESHOLD 2 // max number of bytes to not encode
#define LZS_N 0x1000    // max offset (1 << 12)
#define LZS_F 0x12      // max coded ((1 << 4) + LZS_THRESHOLD)
#define LZS_NIL LZS_N   // index for root of binary search trees

unsigned char ring[LZS_N + LZS_F - 1];
int dad[LZS_N + 1], lson[LZS_N + 1], rson[LZS_N + 1 + 256];
int pos_ring, len_ring;

auto LZSS::encodeLZSS(const std::vector<uint8_t> &data, bool vramCompatible) -> std::vector<uint8_t>
{
    unsigned char *flg;
    unsigned int pak_len, len, pos, len_best, pos_best;
    unsigned int len_next, pos_next, len_post, pos_post;
    unsigned char mask;
    const int lzs_vram = vramCompatible ? 1 : 0;

#define SEARCH(l, p)                                                \
    {                                                               \
        l = LZS_THRESHOLD;                                          \
        pos = raw - raw_buffer >= LZS_N ? LZS_N : raw - raw_buffer; \
        for (; pos > lzs_vram; pos--)                               \
        {                                                           \
            for (len = 0; len < LZS_F; len++)                       \
            {                                                       \
                if (raw + len == raw_end)                           \
                    break;                                          \
                if (*(raw + len) != *(raw + len - pos))             \
                    break;                                          \
            }                                                       \
            if (len > l)                                            \
            {                                                       \
                p = pos;                                            \
                if ((l = len) == LZS_F)                             \
                    break;                                          \
            }                                                       \
        }                                                           \
    }

    pak_len = 4 + data.size() + ((data.size() + 7) / 8);
    std::vector<uint8_t> result(pak_len);
    auto pak_buffer = result.data();

    *(unsigned int *)pak_buffer = CMD_CODE_10 | (data.size() << 8);

    auto pak = pak_buffer + 4;
    auto raw = data.data();
    auto raw_buffer = data.data();
    auto raw_end = raw_buffer + data.size();

    mask = 0;

    while (raw < raw_end)
    {
        if (!(mask >>= LZS_SHIFT))
        {
            *(flg = pak++) = 0;
            mask = LZS_MASK;
        }

        SEARCH(len_best, pos_best);

        // LZ-CUE optimization start
        if (len_best > LZS_THRESHOLD)
        {
            if (raw + len_best < raw_end)
            {
                raw += len_best;
                SEARCH(len_next, pos_next);
                raw -= len_best - 1;
                SEARCH(len_post, pos_post);
                raw--;

                if (len_next <= LZS_THRESHOLD)
                    len_next = 1;
                if (len_post <= LZS_THRESHOLD)
                    len_post = 1;

                if (len_best + len_next <= 1 + len_post)
                    len_best = 1;
            }
        }
        // LZ-CUE optimization end

        if (len_best > LZS_THRESHOLD)
        {
            raw += len_best;
            *flg |= mask;
            *pak++ = ((len_best - (LZS_THRESHOLD + 1)) << 4) | ((pos_best - 1) >> 8);
            *pak++ = (pos_best - 1) & 0xFF;
        }
        else
        {
            *pak++ = *raw++;
        }
    }
    result.resize(pak - pak_buffer);
    return result;
}