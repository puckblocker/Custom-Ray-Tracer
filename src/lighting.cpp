#include "lighting.h"
#include "intersection.h"
#include <glm/glm.hpp>

using namespace Help;

// Point Light (No Geometry To Hit)
glm::vec4 Light::pointLight(pLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi, float &dist)
{
    // PDF is 1
    // VARIABLES
    glm::vec3 point = hitInfo.point;

    // Radiance
    light.specLe.x = ReflectanceCurve(light.color, ray.lambda.x);
    light.specLe.y = ReflectanceCurve(light.color, ray.lambda.y);
    light.specLe.z = ReflectanceCurve(light.color, ray.lambda.z);
    light.specLe.w = ReflectanceCurve(light.color, ray.lambda.w);

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
    glm::vec4 Le = light.specLe * attentuation; // irradiance
    return Le;
}

// Directional Light (Infinite)
glm::vec4 Light::directionalLight(dLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi)
{
    // Radiance
    light.specLe.x = ReflectanceCurve(light.color, ray.lambda.x);
    light.specLe.y = ReflectanceCurve(light.color, ray.lambda.y);
    light.specLe.z = ReflectanceCurve(light.color, ray.lambda.z);
    light.specLe.w = ReflectanceCurve(light.color, ray.lambda.w);

    // PDF is 1.0
    // Light Direction
    wi = glm::normalize(-light.direction);

    // Irradiance
    glm::vec4 Le = light.specLe;
    return Le;
}

// Area Light
glm::vec4 Light::areaLight(aLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi, float &dist)
{
    // Radiance
    light.specLe.x = ReflectanceCurve(light.color, ray.lambda.x);
    light.specLe.y = ReflectanceCurve(light.color, ray.lambda.y);
    light.specLe.z = ReflectanceCurve(light.color, ray.lambda.z);
    light.specLe.w = ReflectanceCurve(light.color, ray.lambda.w);

    // Random Values
    float X0 = RandFloat();
    float Xi = RandFloat();

    // SAMPLING
    glm::vec3 randPoint = light.origin + (X0 * light.u) + (Xi * light.v);

    // Calculations
    float A = glm::length(glm::cross(light.u, light.v)); // surface area
    dist = glm::length(randPoint - hitInfo.point);
    wi = (randPoint - hitInfo.point) / dist;

    // Irradiance
    glm::vec4 Le = light.specLe / (pi * A);

    return Le;
}