#pragma once // Ensures header file only include once throughout program
#include "config.h"
#include "rayData.h"

// Set Color
inline glm::vec3 setColor(Ray &ray, bool valid, glm::vec3 outColor) // omits multiple definitions of the function
{
    // HIT CHECK
    glm::vec3 color;
    if (valid == true)
    {
        // VARIABLES
        // Material Variables
        // glm::vec3 albedo = hitInfo.mat.albedo;
        // float roughness = hitInfo.mat.roughness;
        // float metallic = hitInfo.mat.metallic;
        // float ior = hitInfo.mat.ior;
        // bool emissive = hitInfo.mat.emissive;

        // glm::vec3 point = hitInfo.point;
        color = outColor;
        float pi = 3.14159265359;
    }
    else
    {
        color = glm::vec3(0.69f, 0.88f, 1.0f);
        // color = glm::vec3(0.0f);
    }

    return color;
}