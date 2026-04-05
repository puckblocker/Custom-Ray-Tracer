#include "helper.h"

// Fresnel Reflectance
glm::vec3 Help::FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0)
{
    glm::vec3 Fr = F0 + (1.0f - F0) * glm::pow((1.0f - glm::dot(normal, direction)), 5.0f);
    return Fr;
}

// ROUGH SPECULAR (Trowbridge-Reitz)
// Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
float Help::DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh)
{
    float pi = 3.14159265359;
    float D = alphaSqr / (pi * ((glm::dot(normal, wh) * glm::dot(normal, wh)) * (alphaSqr - 1) + 1));
    return D;
}

// Geometry Function (Approx relative surface area where micro surface details overshadow each other, causing light rays to be covered (from opengl))
float Help::GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi)
{
    float w0HalfVec = glm::dot(normal, w0) / (glm::dot(normal, w0) * (1 - k) + k);
    float wiHalfVec = glm::dot(normal, wi) / (glm::dot(normal, wi) * (1 - k) + k);
    float G = glm::max(w0HalfVec, 0.0f) * glm::max(wiHalfVec, 0.0f); // clamped to prevent negative light (light from behind object)
    return G;
}

// PBR BRDF CALCULATIONS (DIRECT LIGHTING)
glm::vec3 Help::BRDF(glm::vec3 R, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi)
{
    float pi = 3.14159265359;

    // Ideal Diffuse
    glm::vec3 idealDiffuse = R / pi;

    // IDEAL SPECULAR
    // Fresnel Schlick Approx (Calculate Reflection Based On Incident Angle)
    glm::vec3 F0 = glm::vec3(0.04f);
    if (hitInfo.mat.metallic)
    {
        F0 = glm::mix(F0, hitInfo.mat.albedo, hitInfo.mat.metallic); // mix the fresnel reflection with the base color based on how metallic object is
    }

    // ROUGH SPECULAR (Direction Diffuse)

    // Trowbridge Reitz (Normal Distr Function)
    glm::vec3 wh = glm::normalize(0.5f * (w0 + wi));      // half way vector
    float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
    float alphaSqr = alpha * alpha;
    float k = (alphaSqr + 1.0f) / 8.0f;
    float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
    float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

    // Rough Specular BRDF
    glm::vec3 roughSpec = (D * G * FrsRflct(hitInfo.normal, wh, w0)) / (4.0f * glm::dot(hitInfo.normal, w0) * glm::dot(hitInfo.normal, wi));

    // PBR BRDF RESULT (Direct Lighting)
    glm::vec3 directLighting = idealDiffuse + roughSpec;

    return directLighting;
}