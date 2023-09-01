#pragma once
#include <SDL.h>
#include <vector>
#include "glm/glm.hpp"
#include "fragment.h"

glm::vec3 light = normalize(glm::vec3 (0.5,0.5,1));

glm::vec3 barycentricCoordinates(const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    float w = ((B.y - C.y)*(P.x - C.x) + (C.x - B.x)*(P.y - C.y)) /
              ((B.y - C.y)*(A.x - C.x) + (C.x - B.x)*(A.y - C.y));

    float v = ((C.y - A.y)*(P.x - C.x) + (A.x - C.x)*(P.y - C.y)) /
              ((B.y - C.y)*(A.x - C.x) + (C.x - B.x)*(A.y - C.y));

    float u = 1.0f - w - v;

    return glm::vec3(w, v, u);
}


std::vector<Fragment> triangle(Vertex a, Vertex b, Vertex c) {
    glm::vec3 A = a.position;
    glm::vec3 B = b.position;
    glm::vec3 C = c.position;

    std::vector<Fragment> triangleFragments;

    float minX = std::min(std::min(A.x, B.x), C.x);
    float minY = std::min(std::min(A.y, B.y), C.y);
    float maxX = std::max(std::max(A.x, B.x), C.x);
    float maxY = std::max(std::max(A.y, B.y), C.y);

    //Castear a int
    minX = std::floor(minX);
    minY = std::floor(minY);
    maxX = std::ceil(maxX);
    maxY = std::ceil(maxY);

    glm::vec3 N = glm::normalize(glm::cross(B -A, C - A));

    for (float y = minY; y <= maxY; y++) {
        for (float x = minX; x <= maxX; x++) {
            glm::vec3 P = glm::vec3(x, y, 0);

            glm::vec3 bar = barycentricCoordinates(P, A, B, C);

            if (
                    bar.x <= 1 && bar.x >= 0 &&
                    bar.y <= 1 && bar.y >= 0 &&
                    bar.z <= 1 && bar.z >= 0
                    ) {

                P.z = a.position.z * bar.x + b.position.z * bar.y + c.position.z * bar.z;

                float intensity = glm::dot(N, light) * 10;
                Color color = Color (255 * intensity, 255 * intensity, 255 * intensity);
                triangleFragments.push_back(
                        Fragment{P, color}
                );
            }
        }
    }

    return triangleFragments;
}
