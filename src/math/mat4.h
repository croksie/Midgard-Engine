#pragma once
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math/vec3.h"
#include "math/vec4.h"


namespace midgard::math {

struct Mat4 {
    float elements[4][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    Mat4() = default;

    explicit Mat4(float diagonal) {
        elements[0][0] = diagonal; elements[0][1] = 0.0f;     elements[0][2] = 0.0f;     elements[0][3] = 0.0f;
        elements[1][0] = 0.0f;     elements[1][1] = diagonal; elements[1][2] = 0.0f;     elements[1][3] = 0.0f;
        elements[2][0] = 0.0f;     elements[2][1] = 0.0f;     elements[2][2] = diagonal; elements[2][3] = 0.0f;
        elements[3][0] = 0.0f;     elements[3][1] = 0.0f;     elements[3][2] = 0.0f;     elements[3][3] = diagonal;
    }

    Mat4(const glm::mat4& m) {
        std::memcpy(elements, glm::value_ptr(m), sizeof(elements));
    }

    const float* data() const { return &elements[0][0]; }
    float* data() { return &elements[0][0]; }

    Mat4 operator*(const Mat4& other) const {
        return Mat4(toGlm() * other.toGlm());
    }

    Vec4 operator*(const Vec4& v) const {
        glm::vec4 res = toGlm() * v.toGlm();
        return Vec4(res);
    }

    Vec4& operator[](size_t index) {
        return reinterpret_cast<Vec4&>(elements[index]);
    }

    const Vec4& operator[](size_t index) const {
        return reinterpret_cast<const Vec4&>(elements[index]);
    }

    Mat4 translate(const Vec3& v) {
        return Mat4(glm::translate(this->toGlm(), v.toGlm()));
    }

    operator glm::mat4() const { return toGlm(); }

    glm::mat4 toGlm() const {
        return glm::make_mat4(&elements[0][0]);
    }

    static Mat4 identity() {
        return Mat4(1.0f);
    }

    static Mat4 translate(const Mat4& m, const Vec3& v) {
        return Mat4(glm::translate(m.toGlm(), v.toGlm()));
    }

    static Mat4 rotate(const Mat4& m, float angleRadians, const Vec3& axis) {
        return Mat4(glm::rotate(m.toGlm(), angleRadians, axis.toGlm()));
    }

    static Mat4 scale(const Mat4& m, const Vec3& s) {
        return Mat4(glm::scale(m.toGlm(), s.toGlm()));
    }

    static Mat4 perspective(float fovRadians, float aspect, float zNear, float zFar) {
        return Mat4(glm::perspective(fovRadians, aspect, zNear, zFar));
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        return Mat4(glm::lookAt(eye.toGlm(), center.toGlm(), up.toGlm()));
    }
};

} // namespace midgard::math