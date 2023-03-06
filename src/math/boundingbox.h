#pragma once

template <typename T>
class BoundingBox
{
public:
    BoundingBox() = default;
    constexpr BoundingBox(const T &p) : m_min(p), m_max(p) {}
    constexpr BoundingBox(const T &min, const T &max) : m_min(min), m_max(max) {}

    auto min() const -> const T &
    {
        return m_min;
    }

    auto max() const -> const T &
    {
        m_max;
    }

    auto operator|=(const T &p) -> BoundingBox &
    {
        m_min.x() = p.x() < m_min.x() ? p.x() : m_min.x();
        m_min.y() = p.y() < m_min.y() ? p.y() : m_min.y();
        m_min.z() = p.z() < m_min.z() ? p.z() : m_min.z();
        m_max.x() = p.x() > m_max.x() ? p.x() : m_max.x();
        m_max.y() = p.y() > m_max.y() ? p.y() : m_max.y();
        m_max.z() = p.z() > m_max.z() ? p.z() : m_max.z();
        return *this;
    }

    friend auto operator|(BoundingBox b, const T &p) -> BoundingBox
    {
        b |= p;
        return b;
    }

    auto size() const -> float
    {
        return m_min == m_max ? 0.0F : T::distance(m_min, m_max);
    }

private:
    T m_min;
    T m_max;
};
