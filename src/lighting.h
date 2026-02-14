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

        // EXITANT RADIANCE
        float lightDist = glm::length(light.origin - point);
        float attentuation = 1.0f / (lightDist * lightDist);       // reduce lighting effect
        glm::vec3 lightDir = glm::normalize(light.origin - point); // wi
        // float irradiance = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f); // how much light grabbed by surface (angle light is hitting surface) capped to prevent negative light
        glm::vec3 exitRad = light.color * attentuation;

        // PBR BRDF CALCULATION
        glm::vec3 R = hitInfo.mat.albedo;

        // Trowbridge Reitz (Normal Distribution Function)
        // Distribution D(how many micro-facets are aligned / roughness or surface)
        glm::vec3 wh = glm::normalize(0.5f * (viewDir + lightDir)); // half-way vector
        float alpha = glm::max(hitInfo.mat.roughness, 0.05f);       // I may square roughness to blend transition
        float alpha2 = alpha * alpha;
        float D = alpha2 / (0.001f + pi * (((glm::dot(hitInfo.normal, wh)) * glm::dot(hitInfo.normal, wh)) * (alpha2 - 1) + 1));
        // Geometry G(how many mirco-facets are blocked / bumps)
        float k = (alpha2 + 1.0f) / 8.0f;
        float G = (glm::dot(hitInfo.normal, lightDir) / (glm::dot(hitInfo.normal, lightDir) * (1 - k) + k)) * // half of G
                  (glm::dot(hitInfo.normal, viewDir) / (glm::dot(hitInfo.normal, viewDir) * (1 - k) + k));    // other half of G

        // Fr Fresnel Reflectance Calculations
        float iorRatio = (hitInfo.mat.ior - 1.0f) / (hitInfo.mat.ior + 1.0f);
        float F0_dielectric = iorRatio * iorRatio;
        float F0 = glm::mix(F0_dielectric, 0.8f, hitInfo.mat.metallic);

        float FrDot = glm::clamp(glm::dot(wh, viewDir), 0.0f, 1.0f); // prevent negative (cuts off light)
        float FrScalar = F0 + (1.0f - F0) * (glm::pow(1.0f - FrDot, 5.0f));
        glm::vec3 Fr = glm::vec3(FrScalar);

        // PBR BRDF Added Max to Prevent Error of Division by Zero When Perpendicular
        float denom = 4.0f * glm::max(glm::dot(hitInfo.normal, viewDir), 0.0f) * glm::max(glm::dot(hitInfo.normal, lightDir), 0.0f) + 0.001f;
        glm::vec3 fr = (1.0f - Fr) * (hitInfo.mat.albedo / pi) + ((D * G * Fr) / denom);
        // Return New Color
        outColor = fr * exitRad;
        return outColor;
    }

    // Direct Light
    struct dLight
    {
        glm::vec3 direction;
        glm::vec3 color;
    };

    glm::vec3 directionalLight(dLight light, HitInfo hitInfo, glm::vec3 viewDir)
    {
        // VARIABLES
        float pi = 3.14159265359;

        // Directional light direction is constant for all points
        glm::vec3 lightDir = glm::normalize(-light.direction);
        float dotNL = glm::max(glm::dot(hitInfo.normal, lightDir), 0.0f);

        glm::vec3 exitRad = light.color * dotNL;

        glm::vec3 wh = glm::normalize(viewDir + lightDir); // Half-way vector
        float alpha = glm::max(hitInfo.mat.roughness, 0.05f);
        float alpha2 = alpha * alpha;

        // PBR BRDF CALCULATION
        // D (Normal Distribution Function)
        float cosThetaH = glm::max(glm::dot(hitInfo.normal, wh), 0.0f);
        float D_denom = (cosThetaH * cosThetaH * (alpha2 - 1.0f) + 1.0f);
        float D = alpha2 / (pi * D_denom * D_denom + 0.0001f);

        // G (Geometry Function)
        float k = (alpha2 + 1.0f) / 8.0f;
        float G = (glm::dot(hitInfo.normal, lightDir) / (glm::dot(hitInfo.normal, lightDir) * (1 - k) + k + 0.0001f)) *
                  (glm::dot(hitInfo.normal, viewDir) / (glm::dot(hitInfo.normal, viewDir) * (1 - k) + k + 0.0001f));

        // F0 Calculation
        float iorRatio = (hitInfo.mat.ior - 1.0f) / (hitInfo.mat.ior + 1.0f);
        float F0_dielectric = iorRatio * iorRatio;
        float F0 = glm::mix(F0_dielectric, 0.8f, hitInfo.mat.metallic);

        float FrDot = glm::clamp(glm::dot(wh, viewDir), 0.0f, 1.0f);
        float FrScalar = F0 + (1.0f - F0) * (glm::pow(1.0f - FrDot, 5.0f));
        glm::vec3 Fr = glm::vec3(FrScalar);

        // Fr / PBR
        float brdf_denom = 4.0f * glm::max(glm::dot(hitInfo.normal, viewDir), 0.0f) * dotNL + 0.001f;
        glm::vec3 fr = (1.0f - Fr) * (hitInfo.mat.albedo / pi) + ((D * G * Fr) / brdf_denom);

        return fr * exitRad;
    }
};