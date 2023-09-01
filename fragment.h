#pragma once
#include "color.h"

struct Vertex {
    glm::vec3 position;
    Color color;
};

struct Fragment {
    glm::vec3 position;
    Color color;
};