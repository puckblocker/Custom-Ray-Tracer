#include "lighting.h"
#include "intersection.h"
#include <glm/glm.hpp>

// Point Light (No Geometry To Hit)
glm::vec3 Light::pointLight(pLight light, HitInfo hitInfo, glm::vec3 &wi, float &dist)
{
    // PDF is 1
    // VARIABLES
    glm::vec3 point = hitInfo.point;

    // MOST IMPORTANT > radiance = radiant flux / area * angle // radiant flux = energy / time // irradiance = energy / time * area
    // power leaving surface = power surface emits + (incoming power - absorbed power)
    // Shading is from > Radiance(color) seen in specific outgoing direction w sampled at point p on a surface (L0(p, w0) = Le(p,w0) + Lr(p, w0)) (Lr = reflected light afetr absorbtion in our direction)
    // BRDF (fr)  fr(p, wo, wi) = (d*L0(p,w0)) / (Li(p,wi)*(n*wi)d*wi)
    // fr(p, wo, wi) >= 0, BRDF never negative // fr(p, wo, wi) = fr(p,wi,w0) Helmhotz, direction of calc doesn't matter // Energy conservation, BRDF doesn't create energy

    // EMITTED RADIANCE
    dist = glm::length(light.origin - point);
    float attentuation = 1.0f / (dist * dist); // reduce lighting effect
    wi = (light.origin - point) / dist;        // wi (incoming direction)

    // Irradiance
    glm::vec3 Le = light.color * attentuation; // irradiance
    return Le;
}

// Directional Light (Infinite)
glm::vec3 Light::directionalLight(dLight light, HitInfo hitInfo, glm::vec3 &wi)
{
    // PDF is 1.0
    // Light Direction
    wi = glm::normalize(-light.direction);

    // Irradiance
    glm::vec3 Le = light.color;
    return Le;
}