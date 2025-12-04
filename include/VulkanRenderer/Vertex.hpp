#pragma once

#include "glm.hpp"

struct VertexPosNormColorUV {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
    glm::vec2 uv;
};

struct VertexAnimatedPosNormColorUV {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
    glm::vec2 uv;
    glm::ivec4 bone_ids;
    glm::vec4 weights;
};

