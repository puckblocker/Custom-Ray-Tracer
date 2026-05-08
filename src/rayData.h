#pragma once // Ensures header file only include once throughout program

#include <glm/glm.hpp>

class Ray
{
public:
    glm::vec3 origin;
    glm::vec3 direction;
    double time;
    glm::vec4 lambda;   // wavelengths (4)
    glm::vec4 radiance; // radiance / power
};