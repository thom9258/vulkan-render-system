#pragma once

#include "glm.hpp"

struct VertexPosNormColor {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
};

struct VertexPosNormColorUV {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
    glm::vec2 uv;
};
