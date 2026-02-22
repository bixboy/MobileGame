#pragma once
#include <cmath>

namespace MMO::Math 
{

    template<typename T>
    struct Vector2 
    {
        T x{0};
        T y{0};

        constexpr Vector2() = default;
        constexpr Vector2(T x, T y) : x(x), y(y) {}

        constexpr Vector2 operator + (const Vector2& rhs) const { return {x + rhs.x, y + rhs.y}; }
        constexpr Vector2 operator - (const Vector2& rhs) const { return {x - rhs.x, y - rhs.y}; }
        constexpr Vector2 operator * (T scalar) const { return {x * scalar, y * scalar}; }

        Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }

        [[nodiscard]] float Magnitude() const 
        {
            return std::sqrt(static_cast<float>(x * x + y * y));
        }

        [[nodiscard]] static float Distance(const Vector2& a, const Vector2& b) 
        {
            return (a - b).Magnitude();
        }
    };

    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int>;
}