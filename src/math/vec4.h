#pragma once
#include <cmath>

#include <glm/glm.hpp>

#include "math/vec3.h"

namespace midgard::math {

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vec4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}
    constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    constexpr Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    Vec4(const glm::vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

    // Operators
    Vec4 operator+(const Vec4& other) const { return {x + other.x, y + other.y, z + other.z, w + other.w}; }
    Vec4 operator-(const Vec4& other) const { return {x - other.x, y - other.y, z - other.z, w - other.w}; }
    Vec4 operator-() const { return {-x, -y, -z, -w}; }
    Vec4 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
    Vec4 operator*(const Vec4& other) const { return {x * other.x, y * other.y, z * other.z, w * other.w}; }
    Vec4 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar, w / scalar}; }
    Vec4 operator/(const Vec4& other) const { return {x / other.x, y / other.y, z / other.z, w / other.w}; }

    Vec4& operator+=(const Vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    Vec4& operator-=(const Vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
    Vec4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    Vec4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

    bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    bool operator!=(const Vec4& other) const { return !(*this == other); }

    float& operator[](size_t index) { return (&x)[index]; }
    const float& operator[](size_t index) const { return (&x)[index]; }

    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    float lengthSquared() const { return x * x + y * y + z * z + w * w; }

    Vec4 normalized() const {
        float len = length();
        return len > 0.0f ? *this / len : Vec4();
    }

    static float dot(const Vec4& a, const Vec4& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    // GLM cast
    operator glm::vec4() const { return glm::vec4(x, y, z, w); }
    glm::vec4 toGlm() const { return glm::vec4(x, y, z, w); }
};

inline Vec4 operator*(float scalar, const Vec4& v) {
    return v * scalar;
}

} // namespace midgard::math