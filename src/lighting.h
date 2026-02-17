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
        glm::vec3 reflctRad;
        float pi = 3.14159265359;

        // MOST IMPORTANT > radiance = radiant flux / area * angle // radiant flux = energy / time // irradiance = energy / time * area
        // power leaving surface = power surface emits + (incoming power - absorbed power)
        // Shading is from > Radiance(color) seen in specific outgoing direction w sampled at point p on a surface (L0(p, w0) = Le(p,w0) + Lr(p, w0)) (Lr = reflected light afetr absorbtion in our direction)
        // BRDF (fr)  fr(p, wo, wi) = (d*L0(p,w0)) / (Li(p,wi)*(n*wi)d*wi)
        // fr(p, wo, wi) >= 0, BRDF never negative // fr(p, wo, wi) = fr(p,wi,w0) Helmhotz, direction of calc doesn't matter // Energy conservation, BRDF doesn't create energy

        // EXITANT RADIANCE
        float lightDist = glm::length(light.origin - point);
        float attentuation = 1.0f / (lightDist * lightDist);       // reduce lighting effect
        glm::vec3 lightDir = glm::normalize(light.origin - point); // wi (incoming direction)
        glm::vec3 exitRad = light.color * attentuation;

        // CALCULATE REFLECTED IRRADIANCE
        glm::vec3 incomRad = exitRad;                       // incoming irradiance
        float cosTerm = glm::dot(hitInfo.normal, lightDir); // how much light spreads out and weakens (Lambert's Cosine Law)
        reflctRad = incomRad * glm::max(cosTerm, 0.0f);     // reflected irradiance

        return reflctRad;
    }

    // Direct Light
    // struct dLight
    // {
    //     glm::vec3 direction;
    //     glm::vec3 color;
    // };

    // glm::vec3 directionalLight(dLight light, HitInfo hitInfo, glm::vec3 viewDir)
    // {
    //     // VARIABLES
    //     float pi = 3.14159265359;

    //     // Directional light direction is constant for all points
    //     glm::vec3 lightDir = glm::normalize(-light.direction);
    //     float dotNL = glm::max(glm::dot(hitInfo.normal, lightDir), 0.0f);

    //     glm::vec3 exitRad = light.color * dotNL;

    //     glm::vec3 wh = glm::normalize(viewDir + lightDir); // Half-way vector
    //     float alpha = glm::max(hitInfo.mat.roughness, 0.05f);
    //     float alpha2 = alpha * alpha;

    //     // PBR BRDF CALCULATION
    //     // D (Normal Distribution Function)
    //     float cosThetaH = glm::max(glm::dot(hitInfo.normal, wh), 0.0f);
    //     float D_denom = (cosThetaH * cosThetaH * (alpha2 - 1.0f) + 1.0f);
    //     float D = alpha2 / (pi * D_denom * D_denom + 0.0001f);

    //     // G (Geometry Function)
    //     float k = (alpha2 + 1.0f) / 8.0f;
    //     float G = (glm::dot(hitInfo.normal, lightDir) / (glm::dot(hitInfo.normal, lightDir) * (1 - k) + k + 0.0001f)) *
    //               (glm::dot(hitInfo.normal, viewDir) / (glm::dot(hitInfo.normal, viewDir) * (1 - k) + k + 0.0001f));

    //     // F0 (Fresnel Reflectance)
    //     float F0 = hitInfo.mat.metallic / hitInfo.mat.ior;
    //     float FrDot = glm::clamp(glm::dot(wh, viewDir), 0.0f, 1.0f);
    //     float FrScalar = F0 + (1.0f - F0) * (glm::pow(1.0f - FrDot, 5.0f));
    //     glm::vec3 Fr = glm::vec3(FrScalar);

    //     // Fr / PBR
    //     float brdf_denom = 4.0f * glm::max(glm::dot(hitInfo.normal, viewDir), 0.0f) * dotNL + 0.001f;
    //     glm::vec3 fr = (1.0f - Fr) * (hitInfo.mat.albedo / pi) + ((D * G * Fr) / brdf_denom);

    //     return fr * exitRad;
    // }
};