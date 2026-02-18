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
    struct dLight
    {
        glm::vec3 direction;
        glm::vec3 color;
    };

    glm::vec3 directionalLight(dLight light, HitInfo hitInfo, glm::vec3 viewDir)
    {
        // Light Direction
        glm::vec3 lightDir = glm::normalize(-light.direction);

        // Cosine Term (How Much Light Is Hitting)
        float lightAmnt = glm::max(glm::dot(hitInfo.normal, lightDir), 0.0f);

        glm::vec3 exitRad = light.color * lightAmnt;

        return exitRad;
    }
};