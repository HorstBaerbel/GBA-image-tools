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
        m_min.R() = p.R() < m_min.R() ? p.R() : m_min.R();
        m_min.G() = p.G() < m_min.G() ? p.G() : m_min.G();
        m_min.B() = p.B() < m_min.B() ? p.B() : m_min.B();
        m_max.R() = p.R() > m_max.R() ? p.R() : m_max.R();
        m_max.G() = p.G() > m_max.G() ? p.G() : m_max.G();
        m_max.B() = p.B() > m_max.B() ? p.B() : m_max.B();
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
