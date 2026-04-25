#include "lighting.h"
#include "intersection.h"
#include <glm/glm.hpp>

glm::vec3 Light::pointLight(pLight light, HitInfo hitInfo, glm::vec3 &wi, float &dist)
{
    // VARIABLES
    glm::vec3 point = hitInfo.point;

    // MOST IMPORTANT > radiance = radiant flux / area * angle // radiant flux = energy / time // irradiance = energy / time * area
    // power leaving surface = power surface emits + (incoming power - absorbed power)
    // Shading is from > Radiance(color) seen in specific outgoing direction w sampled at point p on a surface (L0(p, w0) = Le(p,w0) + Lr(p, w0)) (Lr = reflected light afetr absorbtion in our direction)
    // BRDF (fr)  fr(p, wo, wi) = (d*L0(p,w0)) / (Li(p,wi)*(n*wi)d*wi)
    // fr(p, wo, wi) >= 0, BRDF never negative // fr(p, wo, wi) = fr(p,wi,w0) Helmhotz, direction of calc doesn't matter // Energy conservation, BRDF doesn't create energy

    // EMITTED RADIANCE
    dist = glm::length(light.origin - point);
    float attentuation = 1.0f / (dist * dist);                       // reduce lighting effect
    wi = (point - light.origin) / glm::length(point - light.origin); // wi (incoming direction)
    glm::vec3 Le = light.color * attentuation;                       // irradiance

    return Le;
}

glm::vec3 Light::directionalLight(dLight light, HitInfo hitInfo, glm::vec3 viewDir)
{
    // Light Direction
    glm::vec3 lightDir = glm::normalize(-light.direction);

    // Cosine Term (How Much Light Is Hitting)
    float lightAmnt = glm::max(glm::dot(-light.direction, hitInfo.normal), 0.0f);

    glm::vec3 exitRad = light.color * lightAmnt;

    return exitRad;
}