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
        return m_max;
    }

    auto operator|=(const T &p) -> BoundingBox &
    {
        m_min[0] = p[0] < m_min[0] ? p[0] : m_min[0];
        m_min[1] = p[1] < m_min[1] ? p[1] : m_min[1];
        m_min[2] = p[2] < m_min[2] ? p[2] : m_min[2];
        m_max[0] = p[0] > m_max[0] ? p[0] : m_max[0];
        m_max[1] = p[1] > m_max[1] ? p[1] : m_max[1];
        m_max[2] = p[2] > m_max[2] ? p[2] : m_max[2];
        return *this;
    }

    friend auto operator|(BoundingBox b, const T &p) -> BoundingBox
    {
        b |= p;
        return b;
    }

    auto size() const -> float
    {
        return m_min == m_max ? 0.0F : T::mse(m_min, m_max);
    }

private:
    T m_min;
    T m_max;
};
