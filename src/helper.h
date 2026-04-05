// Helper Functions
#pragma once

#include <random>
#include "intersection.h"

namespace Help
{
    // Random Number Generator
    float RandFloat();

    // Fresnel Reflectance
    glm::vec3 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0);

    // Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
    float DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh);

    // Geometry Function (Approx relative surface area where micro surface details overshadow each other, causing light rays to be covered (from opengl))
    float GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi);

    // PBR BRDF CALCULATIONS (DIRECT LIGHTING)
    glm::vec3 BRDF(glm::vec3 R, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi);
}