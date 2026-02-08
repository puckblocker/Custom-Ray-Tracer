#pragma once
#include "config.h"
#include "viewport.h"
#include "rayData.h"

class Light
{
public:
    // POINT LIGHT
    glm::vec3 pointLight(glm::vec3 point)
    {
        // VARIABLES
        glm::vec3 outColor;

        // MOST IMPORTANT > radiance = radiant flux / area * angle // radiant flux = energy / time // irradiance = energy / time * area
        // power leaving surface = power surface emits + (incoming power - absorbed power)
        // Shading is from > Radiance(color) seen in specific outgoing direction w sampled at point p on a surface (L0(p, w0) = Le(p,w0) + Lr(p, w0)) (Lr = reflected light afetr absorbtion in our direction)
        // BRDF (fr)  fr(p, wo, wi) = (d*L0(p,w0)) / (Li(p,wi)*(n*wi)d*wi)
        // fr(p, wo, wi) >= 0, BRDF never negative // fr(p, wo, wi) = fr(p,wi,w0) Helmhotz, direction of calc doesn't matter // Energy conservation, BRDF doesn't create energy

        // Return New Color
        return outColor;
    }
};