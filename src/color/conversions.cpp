#include "conversions.h"

#include "lchf.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "rgb565.h"
#include "xrgb8888.h"
#include "ycgcorf.h"

#include <cmath>

namespace Color
{

    // D65 white point
    constexpr float WHITEPOINT_X = 0.950456F;
    constexpr float WHITEPOINT_Y = 1.0F;
    constexpr float WHITEPOINT_Z = 1.088754F;

    // ----- XRGB1555 --------------------------------------------------------------

    template <>
    auto convertTo(const uint16_t &color) -> XRGB1555
    {
        return XRGB1555(color);
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> XRGB1555
    {
        // bring into range
        auto R = static_cast<uint32_t>(color.R()) >> 3;
        auto G = static_cast<uint32_t>(color.G()) >> 3;
        auto B = static_cast<uint32_t>(color.B()) >> 3;
        // clamp to [0,31]
        R = R > 31 ? 31 : R;
        G = G > 31 ? 31 : G;
        B = B > 31 ? 31 : B;
        return XRGB1555(static_cast<uint16_t>(R) << 10) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const RGB565 &color) -> XRGB1555
    {
        // bring into range
        auto R = static_cast<uint32_t>(color.R());
        auto G = static_cast<uint32_t>(color.G()) >> 1;
        auto B = static_cast<uint32_t>(color.B());
        // clamp to [0,31]
        G = G > 31 ? 31 : G;
        return XRGB1555(static_cast<uint16_t>(R) << 10) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const RGBf &color) -> XRGB1555
    {
        // bring into range
        auto R = color.R() * 31.0F;
        auto G = color.G() * 31.0F;
        auto B = color.B() * 31.0F;
        // clamp to [0,31]
        R = R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R);
        G = G < 0.0F ? 0.0F : (G > 31.0F ? 31.0F : G);
        B = B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B);
        return XRGB1555(static_cast<uint16_t>(R) << 10) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> XRGB1555
    {
        // convert to float RGB in [0,1]
        float tmp = color.Y() - color.Cg() / 2.0F;
        float G = color.Cg() + tmp;
        float B = tmp - color.Co() / 2.0F;
        float R = B + color.Co();
        return convertTo<XRGB1555>(RGBf(R, G, B));
    }

    // ----- RGB565 --------------------------------------------------------------

    template <>
    auto convertTo(const uint16_t &color) -> RGB565
    {
        return RGB565(color);
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> RGB565
    {
        // bring into range
        auto R = static_cast<uint32_t>(color.R());
        auto G = static_cast<uint32_t>(color.G()) << 1;
        auto B = static_cast<uint32_t>(color.B());
        // clamp to [0,31] resp. [0,63]
        G = G > 63 ? 63 : G;
        return RGB565(static_cast<uint16_t>(R) << 11) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> RGB565
    {
        // bring into range
        auto R = static_cast<uint32_t>(color.R()) >> 3;
        auto G = static_cast<uint32_t>(color.G()) >> 2;
        auto B = static_cast<uint32_t>(color.B()) >> 3;
        // clamp to [0,31] resp. [0,63]
        R = R > 31 ? 31 : R;
        G = G > 63 ? 63 : G;
        B = B > 31 ? 31 : B;
        return RGB565(static_cast<uint16_t>(R) << 11) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const RGBf &color) -> RGB565
    {
        // bring into range
        auto R = color.R() * 31.0F;
        auto G = color.G() * 63.0F;
        auto B = color.B() * 31.0F;
        // clamp to [0,31] resp. [0,63]
        R = R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R);
        G = G < 0.0F ? 0.0F : (G > 63.0F ? 63.0F : G);
        B = B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B);
        return RGB565(static_cast<uint16_t>(R) << 11) | (static_cast<uint16_t>(G) << 5) | static_cast<uint16_t>(B);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> RGB565
    {
        // convert to float RGB in [0,1]
        float tmp = color.Y() - color.Cg() / 2.0F;
        float G = color.Cg() + tmp;
        float B = tmp - color.Co() / 2.0F;
        float R = B + color.Co();
        return convertTo<RGB565>(RGBf(R, G, B));
    }

    // ----- XRGB8888 --------------------------------------------------------------

    template <>
    auto convertTo(const uint32_t &color) -> XRGB8888
    {
        return XRGB8888(color);
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> XRGB8888
    {
        uint32_t c = static_cast<uint16_t>(color);
        uint32_t R = (c & 0x7C00) << 9;
        uint32_t G = (c & 0x3E0) << 6;
        uint32_t B = (c & 0x1F) << 3;
        return XRGB8888(R | G | B);
    }

    template <>
    auto convertTo(const RGB565 &color) -> XRGB8888
    {
        uint32_t c = static_cast<uint16_t>(color);
        uint32_t R = (c & 0xF800) << 8;
        uint32_t G = (c & 0x7E0) << 5;
        uint32_t B = (c & 0x1F) << 3;
        return XRGB8888(R | G | B);
    }

    template <>
    auto convertTo(const RGBf &color) -> XRGB8888
    {
        float R = color.R() * 255.0F;
        float G = color.G() * 255.0F;
        float B = color.B() * 255.0F;
        // clamp to [0,255]
        uint32_t R32 = R < 0.0F ? 0.0F : (R > 255.0F ? 255.0F : R);
        uint32_t G32 = G < 0.0F ? 0.0F : (G > 255.0F ? 255.0F : G);
        uint32_t B32 = B < 0.0F ? 0.0F : (B > 255.0F ? 255.0F : B);
        return XRGB8888((static_cast<uint32_t>(R32) << 10) | (static_cast<uint32_t>(G32) << 5) | static_cast<uint32_t>(B32));
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> XRGB8888
    {
        // convert to float RGB in [0,1]
        float tmp = color.Y() - color.Cg() / 2.0F;
        float G = color.Cg() + tmp;
        float B = tmp - color.Co() / 2.0F;
        float R = B + color.Co();
        return convertTo<XRGB8888>(RGBf(R, G, B));
    }

    // ----- RGBf -----------------------------------------------------------------

    template <>
    auto convertTo(const XRGB1555 &color) -> RGBf
    {
        uint16_t c = color;
        auto R = static_cast<float>((c & 0x7C00) >> 10);
        auto G = static_cast<float>((c & 0x3E0) >> 5);
        auto B = static_cast<float>(c & 0x1F);
        R *= 31.0F;
        G *= 31.0F;
        B *= 31.0F;
        return RGBf(R, G, B);
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> RGBf
    {
        uint32_t c = color;
        auto R = static_cast<float>((c & 0xFF0000) >> 16);
        auto G = static_cast<float>((c & 0xFF00) >> 8);
        auto B = static_cast<float>(c & 0xFF);
        R *= 255.0F;
        G *= 255.0F;
        B *= 255.0F;
        return RGBf(R, G, B);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> RGBf
    {
        // convert to float RGB in [0,1]
        float tmp = color.Y() - color.Cg() / 2.0F;
        float G = color.Cg() + tmp;
        float B = tmp - color.Co() / 2.0F;
        float R = B + color.Co();
        return RGBf(R, G, B);
    }

    auto LABINVF(float v) -> float
    {
        constexpr float EPSILON = 6.0 / 29.0;
        return v >= EPSILON ? v * v * v : (108.0F / 841.0F) * (v - (4.0F / 29.0F));
    }

    // See: https://mina86.com/2021/srgb-lab-lchab-conversions/
    // and: https://getreuer.info/posts/colorspace/
    // and: https://github.com/lucasb-eyer/go-colorful/blob/master/colors.go
    template <>
    auto convertTo(const Lchf &color) -> RGBf
    {
        // convert to Lab
        float L = color.L();
        float a = color.C() * std::cos(color.H() * float(M_PI / 180.0));
        float b = color.C() * std::sin(color.H() * float(M_PI / 180.0));
        // convert to XYZ
        L = (L + 16.0F) / 116.0F;
        a = L + a / 500.0F;
        b = L - b / 200.0F;
        float X = WHITEPOINT_X * LABINVF(a);
        float Y = WHITEPOINT_Y * LABINVF(L);
        float Z = WHITEPOINT_Z * LABINVF(b);
        // convert to RGB
        float R = 3.2406F * X - 1.5372F * Y - 0.4986F * Z;
        float G = -0.9689F * X + 1.8758F * Y + 0.0415F * Z;
        float B = 0.0557F * X - 0.2040F * Y + 1.0570F * Z;
        float minC = R < G ? (R < B ? R : B) : (G < B ? G : B);
        // Force non-negative values so that gamma correction is well-defined
        if (minC < 0.0F)
        {
            R -= minC;
            G -= minC;
            B -= minC;
        }
        // clamp to range, because Lch / conversion result can have much bigger range
        R = R > 1.0F ? 1.0F : R;
        G = G > 1.0F ? 1.0F : G;
        B = B > 1.0F ? 1.0F : B;
        return RGBf(R, G, B);
    }

    // ----- YCgCoRf --------------------------------------------------------------

    template <>
    auto convertTo(const XRGB1555 &color) -> YCgCoRf
    {
        // convert to float RGB in [0,1]
        uint16_t c = color;
        auto R = static_cast<float>((c & 0x7C00) >> 10);
        auto G = static_cast<float>((c & 0x3E0) >> 5);
        auto B = static_cast<float>(c & 0x1F);
        R *= 31.0F;
        G *= 31.0F;
        B *= 31.0F;
        return convertTo<YCgCoRf>(RGBf(R, G, B));
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> YCgCoRf
    {
        // convert to float RGB in [0,1]
        uint32_t c = color;
        auto R = static_cast<float>((c & 0xFF0000) >> 16);
        auto G = static_cast<float>((c & 0xFF00) >> 8);
        auto B = static_cast<float>(c & 0xFF);
        R *= 255.0F;
        G *= 255.0F;
        B *= 255.0F;
        return convertTo<YCgCoRf>(RGBf(R, G, B));
    }

    template <>
    auto convertTo(const RGBf &color) -> YCgCoRf
    {
        float Co = color.R() - color.B();
        float tmp = color.B() + Co / 2.0F;
        float Cg = color.G() - tmp;
        float Y = tmp + Cg / 2.0F;
        return YCgCoRf(Y, Cg, Co);
    }

    // ----- LCHf --------------------------------------------------------------

    auto LABF(float v) -> float
    {
        constexpr float THIRD = 1.0 / 3.0;
        constexpr float EPSILON = 216.0 / 24389.0;
        return v >= EPSILON ? std::pow(v, THIRD) : (841.0F / 108.0F) * v + (4.0F / 29.0F);
    }

    // See: https://mina86.com/2021/srgb-lab-lchab-conversions/
    // and: https://getreuer.info/posts/colorspace/
    // and: https://github.com/lucasb-eyer/go-colorful/blob/master/colors.go
    template <>
    auto convertTo(const RGBf &color) -> Lchf
    {
        // convert to XYZ
        auto X = color.R() * 0.4124108464885388 + color.G() * 0.3575845678529519 + color.B() * 0.18045380393360833;
        auto Y = color.R() * 0.21264934272065283 + color.G() * 0.7151691357059038 + color.B() * 0.07218152157344333;
        auto Z = color.R() * 0.019331758429150258 + color.G() * 0.11919485595098397 + color.B() * 0.9503900340503373;
        // convert to Lab
        X /= WHITEPOINT_X;
        Y /= WHITEPOINT_Y;
        Z /= WHITEPOINT_Z;
        auto fx = LABF(X / 0.9504492182750991F);
        auto fy = LABF(Y);
        auto fz = LABF(Z / 1.0889166484304715F);
        auto L = 116.0F * fy - 16.0F;
        auto a = 500.0F * (fx - fy);
        auto b = 200.0F * (fy - fz);
        // convert to Lch
        auto C = std::sqrt(a * a + b * b);
        auto H = std::atan2(b, a) * float(180.0 / M_PI);
        if (H < 0.0F)
        {
            H += 360.0F;
        }
        return Lchf(L, C, H);
    }

}
