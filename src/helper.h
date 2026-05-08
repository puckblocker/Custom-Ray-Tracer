// Helper Functions
#pragma once

#include <random>
#include "intersection.h"
#include "rayData.h"

struct HitInfo;

namespace Help
{
    constexpr float pi = 3.14159265359f;

    // Random Number Generator
    float RandFloat();

    // Fresnel Reflectance
    glm::vec4 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec4 F0);

    // Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
    float DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh);

    // Geometry Function (Approx relative surface area where micro surface details overshadow each other, causing light rays to be covered (from opengl))
    float GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi);

    // PBR BRDF CALCULATIONS
    glm::vec4 BxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 &wi, float &pdf);        // indirect
    glm::vec4 DirectBxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi);               // direct
    glm::vec4 LayeredBxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 &wi, float &pdf); // layered

    // RGB TO SPECTRUM
    glm::vec4 RandLambda();                                 // generate random wavelength values within spectrum
    float ReflectanceCurve(glm::vec3 rgbVal, float lambda); // generate curve (wave) from rgb values / convert from rgb to spectrum

    // CIE XYZ Table (CIE Table gotten from https://cie.co.at/datatable/cie-1931-colour-matching-functions-2-degree-observer)
    struct CIE_Table
    {
        float x, y, z;
    };

    extern std::vector<CIE_Table> cieTable;

    // CIE Weights
    CIE_Table getWeights(float lambda);
}