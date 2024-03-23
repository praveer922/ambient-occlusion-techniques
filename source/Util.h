#pragma once
#include <cmath>
namespace Util { 

float degreesToRadians(float degrees) {
    return degrees * M_PI / 180.0;
}

cy::Matrix4f getOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) {
        cy::Matrix4f proj = cy::Matrix4f(2.0f / (right - left), 0, 0, -(right + left) / (right - left),
        0, 2.0f / (top - bottom), 0, -(top + bottom) / (top - bottom),
        0, 0, -2.0f / (far - near),  -(far + near) / (far - near),
        0, 0, 0, 1);

        return proj;
}

};