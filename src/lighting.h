#pragma once
#include "config.h"
#include "viewport.h"
#include "rayData.h"

class Light
{
public:
    // POINT LIGHT
    struct pLight
    {
        glm::vec3 origin; // light position in global coords
        glm::vec3 color;  // exitant radiance
    };
    glm::vec3 pointLight(glm::vec3 point, pLight light, HitInfo hitInfo, glm::vec3 viewDir)
    {
        // VARIABLES
        glm::vec3 outColor;
        float pi = 3.14159265359;

        // MOST IMPORTANT > radiance = radiant flux / area * angle // radiant flux = energy / time // irradiance = energy / time * area
        // power leaving surface = power surface emits + (incoming power - absorbed power)
        // Shading is from > Radiance(color) seen in specific outgoing direction w sampled at point p on a surface (L0(p, w0) = Le(p,w0) + Lr(p, w0)) (Lr = reflected light afetr absorbtion in our direction)
        // BRDF (fr)  fr(p, wo, wi) = (d*L0(p,w0)) / (Li(p,wi)*(n*wi)d*wi)
        // fr(p, wo, wi) >= 0, BRDF never negative // fr(p, wo, wi) = fr(p,wi,w0) Helmhotz, direction of calc doesn't matter // Energy conservation, BRDF doesn't create energy

        // EXITANT RADIANCE
        float lightDist = glm::length(light.origin - point);
        float attentuation = 1.0f / (lightDist * lightDist);                   // reduce lighting effect
        glm::vec3 lightDir = glm::normalize(light.origin - point);             // wi
        float irradiance = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f); // how much light grabbed by surface (angle light is hitting surface) capped to prevent negative light
        glm::vec3 exitRad = light.color * attentuation * irradiance;

        // PBR BRDF CALCULATION
        glm::vec3 R = hitInfo.mat.albedo;

        // Trowbridge Reitz (Normal Distribution Function)
        // Distribution D(how many micro-facets are aligned / roughness or surface)
        glm::vec3 wh = glm::normalize(0.5f * (viewDir + lightDir)); // half-way vector
        float alpha = hitInfo.mat.roughness;                        // I may squar roughness to blend transition
        float alpha2 = alpha * alpha;
        float D = alpha2 / (pi * (((glm::dot(hitInfo.normal, wh)) * glm::dot(hitInfo.normal, wh)) * (alpha2 - 1) + 1));
        // Geometry G(how many mirco-facets are blocked / bumps)
        float k = (alpha2 + 1.0f) / 8.0f;
        float G = (glm::dot(hitInfo.normal, lightDir) / (glm::dot(hitInfo.normal, lightDir) * (1 - k) + k)) * // half of G
                  (glm::dot(hitInfo.normal, viewDir) / (glm::dot(hitInfo.normal, viewDir) * (1 - k) + k));    // other half of G

        // Fr Fresnal Reflectance Schlick Approximation
        float F0 = hitInfo.mat.metallic / hitInfo.mat.ior; // reflectance at normal incidence
        float FrScalar = F0 + (1.0f - F0) * (glm::pow(1.0f - glm::dot(wh, viewDir), 5.0f));
        glm::vec3 Fr = glm::vec3(FrScalar);

        // PBR BRDF Added Max to Prevent Error of Division by Zero When Perpendicular
        glm::vec3 fr = (1.0f - Fr) * (hitInfo.mat.albedo / pi) + ((D * G * Fr) / (4 * glm::max(glm::dot(hitInfo.normal, viewDir), 0.0f) * glm::max(glm::dot(hitInfo.normal, lightDir), 0.0f) + 0.0001f));

        // Return New Color
        outColor = fr * exitRad;
        return outColor;
    }
};