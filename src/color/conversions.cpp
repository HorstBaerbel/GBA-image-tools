#include "conversions.h"

#include "grayf.h"
#include "cielabf.h"
#include "rgb565.h"
#include "rgb888.h"
#include "rgbf.h"
#include "xrgb1555.h"
#include "xrgb8888.h"
#include "ycgcorf.h"

#include <cmath>

namespace Color
{

    // D65 white point (noon daylight: television, sRGB color space)
    constexpr float WHITEPOINT_D65_X = 0.950489;
    constexpr float WHITEPOINT_D65_Y = 1.0;
    constexpr float WHITEPOINT_D65_Z = 1.088840;

    // D50 white point (horizon light, ICC profile PCS)
    constexpr float WHITEPOINT_D50_X = 0.964112;
    constexpr float WHITEPOINT_D50_Y = 1.0;
    constexpr float WHITEPOINT_D50_Z = 0.825188;

    // ----- RGBf -----------------------------------------------------------------

    template <>
    auto convertTo(const Grayf &color) -> RGBf
    {
        return RGBf(color.I(), color.I(), color.I());
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> RGBf
    {
        auto R = static_cast<float>(color.R()) / 31.0F;
        auto G = static_cast<float>(color.G()) / 31.0F;
        auto B = static_cast<float>(color.B()) / 31.0F;
        return RGBf(R, G, B);
    }

    template <>
    auto convertTo(const RGB565 &color) -> RGBf
    {
        auto R = static_cast<float>(color.R()) / 31.0F;
        auto G = static_cast<float>(color.G()) / 63.0F;
        auto B = static_cast<float>(color.B()) / 31.0F;
        return RGBf(R, G, B);
    }

    template <>
    auto convertTo(const RGB888 &color) -> RGBf
    {
        auto R = static_cast<float>(color.R()) / 255.0F;
        auto G = static_cast<float>(color.G()) / 255.0F;
        auto B = static_cast<float>(color.B()) / 255.0F;
        return RGBf(R, G, B);
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> RGBf
    {
        auto R = static_cast<float>(color.R()) / 255.0F;
        auto G = static_cast<float>(color.G()) / 255.0F;
        auto B = static_cast<float>(color.B()) / 255.0F;
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
        constexpr float THIRD = 1.0 / 3.0;
        constexpr float CBRT_EPSILON = 6.0 / 29.0; // = 216 / 24389 ^ (1/3)
        constexpr float KAPPA = 24389.0 / 27.0;
        return v > CBRT_EPSILON ? v * v * v : (v * 116.0F - 16.0F) / KAPPA;
    }

    // See: https://mina86.com/2021/srgb-lab-lchab-conversions/
    // and: https://getreuer.info/posts/colorspace/
    // and: https://github.com/lucasb-eyer/go-colorful/blob/master/colors.go
    // and: http://www.brucelindbloom.com/index.html -> Math
    // Note that we need linearized RGB values here!
    template <>
    auto convertTo(const CIELabf &color) -> RGBf
    {
        constexpr float KAPPA = 24389.0 / 27.0;
        float L = color.L();
        float a = color.a();
        float b = color.b();
        // convert Lab to XYZ
        float fy = (L + 16.0F) / 116.0F;
        float fx = fy + a / 500.0F;
        float fz = fy - b / 200.0F;
        float X = LABINVF(fx);
        float Y = L > 8.0F ? fy * fy * fy : L / KAPPA;
        float Z = LABINVF(fz);
        // apply white reference
        X *= WHITEPOINT_D65_X;
        Y *= WHITEPOINT_D65_Y;
        Z *= WHITEPOINT_D65_Z;
        // convert to RGB
        float R = X * 3.240812398895283F - Y * 1.5373084456298136F - Z * 0.4985865229069666F;
        float G = X * -0.9692430170086407F + Y * 1.8759663029085742F + Z * 0.04155503085668564F;
        float B = X * 0.055638398436112804F - Y * 0.20400746093241362F + Z * 1.0571295702861434F;
        float minC = R < G ? (R < B ? R : B) : (G < B ? G : B);
        // Force non-negative values so that gamma correction is well-defined
        if (minC < 0.0F)
        {
            R -= minC;
            G -= minC;
            B -= minC;
        }
        // clamp to range, because Lab / conversion result can have a much bigger range
        R = R > 1.0F ? 1.0F : R;
        G = G > 1.0F ? 1.0F : G;
        B = B > 1.0F ? 1.0F : B;
        return RGBf(R, G, B);
    }

    // ----- XRGB1555 --------------------------------------------------------------

    template <>
    auto convertTo(const Grayf &color) -> XRGB1555
    {
        // bring into range and round
        auto R = color.I() * 31.0F + 0.5F;
        // clamp to [0,31]
        auto R16 = static_cast<uint16_t>(R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R));
        return XRGB1555((R16 << 10) | (R16 << 5) | R16);
    }

    template <>
    auto convertTo(const uint16_t &color) -> XRGB1555
    {
        return XRGB1555(color);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/9ab8887d2cb48685
    template <>
    auto convertTo(const RGB888 &color) -> XRGB1555
    {
        // bring into range and round
        auto R = (static_cast<uint16_t>(color.R()) * 249 + 1014) >> 11;
        auto G = (static_cast<uint16_t>(color.G()) * 249 + 1014) >> 11;
        auto B = (static_cast<uint16_t>(color.B()) * 249 + 1014) >> 11;
        return XRGB1555((R << 10) | (G << 5) | B);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/9ab8887d2cb48685
    template <>
    auto convertTo(const XRGB8888 &color) -> XRGB1555
    {
        // bring into range and round
        auto R = (static_cast<uint16_t>(color.R()) * 249 + 1014) >> 11;
        auto G = (static_cast<uint16_t>(color.G()) * 249 + 1014) >> 11;
        auto B = (static_cast<uint16_t>(color.B()) * 249 + 1014) >> 11;
        return XRGB1555((R << 10) | (G << 5) | B);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/90829ec6fc2f95c3
    template <>
    auto convertTo(const RGB565 &color) -> XRGB1555
    {
        // bring into range and round
        auto R = static_cast<uint16_t>(color.R());
        auto G = static_cast<uint16_t>(color.G() * 31 + 31) >> 6;
        auto B = static_cast<uint16_t>(color.B());
        return XRGB1555((R << 10) | (G << 5) | B);
    }

    template <>
    auto convertTo(const RGBf &color) -> XRGB1555
    {
        // bring into range and round
        auto R = color.R() * 31.0F + 0.5F;
        auto G = color.G() * 31.0F + 0.5F;
        auto B = color.B() * 31.0F + 0.5F;
        // clamp to [0,31]
        auto R16 = static_cast<uint16_t>(R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R));
        auto G16 = static_cast<uint16_t>(G < 0.0F ? 0.0F : (G > 31.0F ? 31.0F : G));
        auto B16 = static_cast<uint16_t>(B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B));
        return XRGB1555((R16 << 10) | (G16 << 5) | B16);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> XRGB1555
    {
        return convertTo<XRGB1555>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> XRGB1555
    {
        return convertTo<XRGB1555>(convertTo<RGBf>(color));
    }

    // ----- RGB565 --------------------------------------------------------------

    template <>
    auto convertTo(const Grayf &color) -> RGB565
    {
        // bring into range and round
        auto R = color.I() * 31.0F + 0.5F;
        auto G = color.I() * 63.0F + 0.5F;
        // clamp to [0,31] resp. [0,63]
        auto R16 = static_cast<uint16_t>(R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R));
        auto G16 = static_cast<uint16_t>(G < 0.0F ? 0.0F : (G > 63.0F ? 63.0F : G));
        return RGB565((R16 << 11) | (G16 << 5) | R16);
    }

    template <>
    auto convertTo(const uint16_t &color) -> RGB565
    {
        return RGB565(color);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/90829ec6fc2f95c3
    template <>
    auto convertTo(const XRGB1555 &color) -> RGB565
    {
        // bring into range and round
        auto R = static_cast<uint16_t>(color.R());
        auto G = static_cast<uint16_t>(color.G() * 130 + 33) >> 6;
        auto B = static_cast<uint16_t>(color.B());
        return RGB565((R << 11) | (G << 5) | B);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/9ab8887d2cb48685
    template <>
    auto convertTo(const RGB888 &color) -> RGB565
    {
        // bring into range and round
        auto R = (static_cast<uint16_t>(color.R()) * 249 + 1014) >> 11;
        auto G = (static_cast<uint16_t>(color.G()) * 253 + 505) >> 10;
        auto B = (static_cast<uint16_t>(color.B()) * 249 + 1014) >> 11;
        return RGB565((R << 11) | (G << 5) | B);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/9ab8887d2cb48685
    template <>
    auto convertTo(const XRGB8888 &color) -> RGB565
    {
        // bring into range and round
        auto R = (static_cast<uint16_t>(color.R()) * 249 + 1014) >> 11;
        auto G = (static_cast<uint16_t>(color.G()) * 253 + 505) >> 10;
        auto B = (static_cast<uint16_t>(color.B()) * 249 + 1014) >> 11;
        return RGB565((R << 11) | (G << 5) | B);
    }

    template <>
    auto convertTo(const RGBf &color) -> RGB565
    {
        // bring into range and round
        auto R = color.R() * 31.0F + 0.5F;
        auto G = color.G() * 63.0F + 0.5F;
        auto B = color.B() * 31.0F + 0.5F;
        // clamp to [0,31] resp. [0,63]
        auto R16 = static_cast<uint16_t>(R < 0.0F ? 0.0F : (R > 31.0F ? 31.0F : R));
        auto G16 = static_cast<uint16_t>(G < 0.0F ? 0.0F : (G > 63.0F ? 63.0F : G));
        auto B16 = static_cast<uint16_t>(B < 0.0F ? 0.0F : (B > 31.0F ? 31.0F : B));
        return RGB565((R16 << 11) | (G16 << 5) | B16);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> RGB565
    {
        return convertTo<RGB565>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> RGB565
    {
        return convertTo<RGB565>(convertTo<RGBf>(color));
    }

    // ----- XRGB8888 --------------------------------------------------------------

    template <>
    auto convertTo(const Grayf &color) -> XRGB8888
    {
        // bring into range and round
        float R = color.I() * 255.0F + 0.5F;
        float G = color.I() * 255.0F + 0.5F;
        // clamp to [0,255]
        auto R32 = static_cast<uint32_t>(R < 0.0F ? 0.0F : (R > 255.0F ? 255.0F : R));
        auto G32 = static_cast<uint32_t>(G < 0.0F ? 0.0F : (G > 255.0F ? 255.0F : G));
        return XRGB8888((R32 << 16) | (G32 << 8) | R32);
    }

    template <>
    auto convertTo(const uint32_t &color) -> XRGB8888
    {
        return XRGB8888(color);
    }

    template <>
    auto convertTo(const RGB888 &color) -> XRGB8888
    {
        return XRGB8888(uint32_t(color));
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/90829ec6fc2f95c3
    template <>
    auto convertTo(const XRGB1555 &color) -> XRGB8888
    {
        // bring into range and round
        uint32_t c = static_cast<uint16_t>(color);
        uint32_t R = (((c & 0x7C00) >> 10) * 527 + 23) >> 6;
        uint32_t G = (((c & 0x3E0) >> 5) * 527 + 23) >> 6;
        uint32_t B = ((c & 0x1F) * 527 + 23) >> 6;
        return XRGB8888((R << 16) | (G << 8) | B);
    }

    // See: https://stackoverflow.com/a/9069480/1121150
    // Test: https://coliru.stacked-crooked.com/a/90829ec6fc2f95c3
    template <>
    auto convertTo(const RGB565 &color) -> XRGB8888
    {
        // bring into range and round
        uint32_t c = static_cast<uint16_t>(color);
        uint32_t R = (((c & 0xF800) >> 11) * 527 + 23) >> 6;
        uint32_t G = (((c & 0x7E0) >> 5) * 259 + 33) >> 6;
        uint32_t B = ((c & 0x1F) * 527 + 23) >> 6;
        return XRGB8888((R << 16) | (G << 8) | B);
    }

    template <>
    auto convertTo(const RGBf &color) -> XRGB8888
    {
        // bring into range and round
        float R = color.R() * 255.0F + 0.5F;
        float G = color.G() * 255.0F + 0.5F;
        float B = color.B() * 255.0F + 0.5F;
        // clamp to [0,255]
        auto R32 = static_cast<uint32_t>(R < 0.0F ? 0.0F : (R > 255.0F ? 255.0F : R));
        auto G32 = static_cast<uint32_t>(G < 0.0F ? 0.0F : (G > 255.0F ? 255.0F : G));
        auto B32 = static_cast<uint32_t>(B < 0.0F ? 0.0F : (B > 255.0F ? 255.0F : B));
        return XRGB8888((R32 << 16) | (G32 << 8) | B32);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> XRGB8888
    {
        return convertTo<XRGB8888>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> XRGB8888
    {
        return convertTo<XRGB8888>(convertTo<RGBf>(color));
    }

    // ----- RGB888 ----------------------------------------------------------------

    template <>
    auto convertTo(const XRGB8888 &color) -> RGB888
    {
        return RGB888((uint32_t)color);
    }

    template <>
    auto convertTo(const Grayf &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const uint32_t &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const RGB565 &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const RGBf &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> RGB888
    {
        return convertTo<RGB888>(convertTo<XRGB8888>(color));
    }

    // ----- YCgCoRf --------------------------------------------------------------

    template <>
    auto convertTo(const RGBf &color) -> YCgCoRf
    {
        float Co = color.R() - color.B();
        float tmp = color.B() + Co / 2.0F;
        float Cg = color.G() - tmp;
        float Y = tmp + Cg / 2.0F;
        return YCgCoRf(Y, Cg, Co);
    }

    template <>
    auto convertTo(const Grayf &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const RGB565 &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const RGB888 &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> YCgCoRf
    {
        return convertTo<YCgCoRf>(convertTo<RGBf>(color));
    }

    // ----- CIELabf --------------------------------------------------------------

    auto LABF(float v) -> float
    {
        constexpr float THIRD = 1.0 / 3.0;
        constexpr float EPSILON = 216.0 / 24389.0;
        constexpr float KAPPA = 24389.0 / 27.0;
        return v > EPSILON ? std::pow(v, THIRD) : (KAPPA * v + 16.0F) / 116.0F;
    }

    // See: https://mina86.com/2021/srgb-lab-lchab-conversions/
    // and: https://getreuer.info/posts/colorspace/
    // and: https://github.com/lucasb-eyer/go-colorful/blob/master/colors.go
    // and: http://www.brucelindbloom.com/index.html -> Math
    // Note that this returns linearized RGB values!
    template <>
    auto convertTo(const RGBf &color) -> CIELabf
    {
        // convert RGB to XYZ
        float X = color.R() * 0.4124108464885388F + color.G() * 0.3575845678529519F + color.B() * 0.18045380393360833F;
        float Y = color.R() * 0.21264934272065283F + color.G() * 0.7151691357059038F + color.B() * 0.07218152157344333F;
        float Z = color.R() * 0.019331758429150258F + color.G() * 0.11919485595098397F + color.B() * 0.9503900340503373F;
        // apply white reference
        X /= WHITEPOINT_D65_X;
        Y /= WHITEPOINT_D65_Y;
        Z /= WHITEPOINT_D65_Z;
        // convert XYZ to Lab
        float fx = LABF(X);
        float fy = LABF(Y);
        float fz = LABF(Z);
        float L = 116.0F * fy - 16.0F;
        float a = 500.0F * (fx - fy);
        float b = 200.0F * (fy - fz);
        return CIELabf(L, a, b);
    }

    template <>
    auto convertTo(const Grayf &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const RGB565 &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const RGB888 &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> CIELabf
    {
        return convertTo<CIELabf>(convertTo<RGBf>(color));
    }

    // ----- Grayf --------------------------------------------------------------

    template <>
    auto convertTo(const RGBf &color) -> Grayf
    {
        return Grayf(0.2126F * color.R() + 0.7152F * color.G() + 0.0722F * color.B());
    }

    template <>
    auto convertTo(const XRGB1555 &color) -> Grayf
    {
        return Grayf((0.2126F * static_cast<float>(color.R()) + 0.7152F * static_cast<float>(color.G()) + 0.0722F * static_cast<float>(color.B())) / 31.0F);
    }

    template <>
    auto convertTo(const RGB565 &color) -> Grayf
    {
        return Grayf((0.2126F * static_cast<float>(color.R())) / 31.0F + (0.7152F * static_cast<float>(color.G())) / 63.0F + (0.0722F * static_cast<float>(color.B())) / 31.0F);
    }

    template <>
    auto convertTo(const RGB888 &color) -> Grayf
    {
        return Grayf((0.2126F * static_cast<float>(color.R()) + 0.7152F * static_cast<float>(color.G()) + 0.0722F * static_cast<float>(color.B())) / 255.0F);
    }

    template <>
    auto convertTo(const XRGB8888 &color) -> Grayf
    {
        return Grayf((0.2126F * static_cast<float>(color.R()) + 0.7152F * static_cast<float>(color.G()) + 0.0722F * static_cast<float>(color.B())) / 255.0F);
    }

    template <>
    auto convertTo(const YCgCoRf &color) -> Grayf
    {
        return convertTo<Grayf>(convertTo<RGBf>(color));
    }

    template <>
    auto convertTo(const CIELabf &color) -> Grayf
    {
        return convertTo<Grayf>(convertTo<RGBf>(color));
    }

}
