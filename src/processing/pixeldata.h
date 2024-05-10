#pragma once

#include "color/colorformat.h"
#include "color/conversions.h"
#include "color/grayf.h"
#include "color/lchf.h"
#include "color/rgb565.h"
#include "color/rgbf.h"
#include "color/xrgb1555.h"
#include "color/xrgb8888.h"
#include "color/ycgcorf.h"
#include "exception.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace Image
{

    /// @brief Stores indexed, true color or raw / compressed pixels / color map data
    /// Continuous array of pixels / colors, no stride etc.
    class PixelData
    {
    public:
        using storage_type = std::variant<std::monostate, std::vector<uint8_t>, std::vector<Color::XRGB1555>, std::vector<Color::RGB565>, std::vector<Color::XRGB8888>, std::vector<Color::RGBf>, std::vector<Color::LChf>, std::vector<Color::YCgCoRf>, std::vector<Color::Grayf>>;

        PixelData() = default;

        template <typename PIXEL_TYPE>
        PixelData(const std::vector<PIXEL_TYPE> &data, Color::Format dataFormat)
            : m_data(data), m_dataFormat(dataFormat)
        {
            if constexpr (std::is_same<PIXEL_TYPE, uint8_t>::value)
            {
                REQUIRE(m_dataFormat == Color::Format::Unknown || m_dataFormat == Color::Format::Paletted1 || m_dataFormat == Color::Format::Paletted2 || m_dataFormat == Color::Format::Paletted4 || m_dataFormat == Color::Format::Paletted8, std::runtime_error, "Color format must be paletted or raw");
            }
            else
            {
                REQUIRE(m_dataFormat == Color::Format::XRGB1555 || m_dataFormat == Color::Format::RGB565 ||
                            m_dataFormat == Color::Format::XRGB8888 || m_dataFormat == Color::Format::RGBf ||
                            m_dataFormat == Color::Format::LChf || m_dataFormat == Color::Format::YCgCoRf ||
                            m_dataFormat == Color::Format::Grayf,
                        std::runtime_error, "Color format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf, YCgCoRf, Grayf");
            }
        }

        template <typename PIXEL_TYPE>
        PixelData(const std::vector<PIXEL_TYPE> &&data, Color::Format dataFormat)
            : m_dataFormat(dataFormat)
        {
            if constexpr (std::is_same<PIXEL_TYPE, uint8_t>::value)
            {
                REQUIRE(m_dataFormat == Color::Format::Unknown || m_dataFormat == Color::Format::Paletted1 || m_dataFormat == Color::Format::Paletted2 || m_dataFormat == Color::Format::Paletted4 || m_dataFormat == Color::Format::Paletted8, std::runtime_error, "Color format must be paletted or raw");
            }
            else
            {
                REQUIRE(m_dataFormat == Color::Format::XRGB1555 || m_dataFormat == Color::Format::RGB565 ||
                            m_dataFormat == Color::Format::XRGB8888 || m_dataFormat == Color::Format::RGBf ||
                            m_dataFormat == Color::Format::LChf || m_dataFormat == Color::Format::YCgCoRf ||
                            m_dataFormat == Color::Format::Grayf,
                        std::runtime_error, "Color format must be XRGB1555, RGB565, XRGB8888, RGBf, LChf, YCgCoRf, Grayf");
            }
            m_data = std::move(data);
        }

        template <typename T>
        auto data() const -> const std::vector<T> &
        {
            REQUIRE(std::holds_alternative<std::vector<T>>(m_data), std::runtime_error, "Can't get data in different format");
            return std::get<std::vector<T>>(m_data);
        }

        template <typename T>
        auto data() -> std::vector<T> &
        {
            REQUIRE(std::holds_alternative<std::vector<T>>(m_data), std::runtime_error, "Can't get data in different format");
            return std::get<std::vector<T>>(m_data);
        }

        auto storage() const -> const storage_type &
        {
            return m_data;
        }

        friend bool operator==(const PixelData &lhs, const PixelData &rhs);

        friend bool operator!=(const PixelData &lhs, const PixelData &rhs);

        template <typename T>
        auto convertData() const -> std::vector<T>
        {
            if (std::holds_alternative<std::vector<T>>(m_data))
            {
                return std::get<std::vector<T>>(m_data);
            }
            else if constexpr (!std::is_same<T, uint8_t>())
            {
                if (std::holds_alternative<std::vector<Color::XRGB1555>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::XRGB1555>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::RGB565>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::RGB565>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::XRGB8888>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::XRGB8888>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::RGBf>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::RGBf>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::LChf>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::LChf>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::YCgCoRf>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::YCgCoRf>>(m_data));
                }
                else if (std::holds_alternative<std::vector<Color::Grayf>>(m_data))
                {
                    return Color::convertTo<T>(std::get<std::vector<Color::Grayf>>(m_data));
                }
            }
            THROW(std::runtime_error, "Unsupported pixel format");
        }

        auto convertDataToRaw() const -> std::vector<uint8_t>
        {
            if (std::holds_alternative<std::vector<uint8_t>>(m_data))
            {
                return std::get<std::vector<uint8_t>>(m_data);
            }
            else if (std::holds_alternative<std::vector<Color::XRGB1555>>(m_data))
            {
                return getAsRaw<Color::XRGB1555>();
            }
            else if (std::holds_alternative<std::vector<Color::RGB565>>(m_data))
            {
                return getAsRaw<Color::RGB565>();
            }
            else if (std::holds_alternative<std::vector<Color::XRGB8888>>(m_data))
            {
                return getAsRaw<Color::XRGB8888>();
            }
            else if (std::holds_alternative<std::vector<Color::LChf>>(m_data))
            {
                return getAsRaw<Color::LChf>();
            }
            else if (std::holds_alternative<std::vector<Color::YCgCoRf>>(m_data))
            {
                return getAsRaw<Color::YCgCoRf>();
            }
            else if (std::holds_alternative<std::vector<Color::Grayf>>(m_data))
            {
                return getAsRaw<Color::Grayf>();
            }
            THROW(std::runtime_error, "Unsupported pixel format");
        }

        auto empty() const -> bool
        {
            return std::visit([](auto arg)
                              { if constexpr(std::is_same<decltype(arg), std::monostate>()) {
                                 return true;
                               }
                               else { return arg.empty(); } },
                              m_data);
        }

        auto size() const -> std::size_t
        {
            return std::visit([](auto arg)
                              { if constexpr(std::is_same<decltype(arg), std::monostate>()) {
                                 return std::size_t(0);
                               }
                               else { return arg.size(); } },
                              m_data);
        }

        auto rawSize() const -> std::size_t
        {
            return std::visit([](auto arg)
                              { if constexpr(std::is_same<decltype(arg), std::monostate>()) {
                                 return std::size_t(0);
                               }
                               else { return arg.size() * sizeof(typename decltype(arg)::value_type); } },
                              m_data);
        }

        auto format() const -> Color::Format
        {
            return m_dataFormat;
        }

        auto isGrayscale() const -> bool
        {
            return !std::holds_alternative<std::monostate>(m_data) && Color::formatInfo(m_dataFormat).channels == 1 && std::holds_alternative<std::vector<Color::Grayf>>(m_data);
        }

        auto isIndexed() const -> bool
        {
            return !std::holds_alternative<std::monostate>(m_data) && Color::formatInfo(m_dataFormat).isIndexed && std::holds_alternative<std::vector<uint8_t>>(m_data);
        }

        auto isTruecolor() const -> bool
        {
            return !std::holds_alternative<std::monostate>(m_data) && Color::formatInfo(m_dataFormat).isTruecolor && (!std::holds_alternative<std::vector<uint8_t>>(m_data));
        }

        auto isRaw() const -> bool
        {
            return !std::holds_alternative<std::monostate>(m_data) && m_dataFormat == Color::Format::Unknown && std::holds_alternative<std::vector<uint8_t>>(m_data);
        }

    private:
        template <typename T>
        auto getAsRaw() const -> std::vector<uint8_t>
        {
            const auto &src = std::get<std::vector<T>>(m_data);
            auto srcPtr = src.data();
            std::vector<uint8_t> raw(src.size() * sizeof(T));
            auto rawPtr = reinterpret_cast<T *>(raw.data());
            std::memcpy(rawPtr, srcPtr, raw.size());
            return raw;
        }

        Color::Format m_dataFormat = Color::Format::Unknown;
        storage_type m_data;
    };

    inline bool operator==(const PixelData &lhs, const PixelData &rhs)
    {
        return lhs.m_dataFormat == rhs.m_dataFormat &&
               lhs.m_data == rhs.m_data;
    }

    inline bool operator!=(const PixelData &lhs, const PixelData &rhs)
    {
        return !(lhs == rhs);
    }

}
