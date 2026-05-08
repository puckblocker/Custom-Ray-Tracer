#pragma once

#include <glm/glm.hpp>

#include "intersection.h"
#include "config.h"
#include "viewport.h"
#include "rayData.h"

class Light
{
public:
    // POINT LIGHT
    struct pLight
    {
        glm::vec3 origin = glm::vec3(0.0f); // light position in global coords
        glm::vec3 color = glm::vec4(0.0f);  // color / power
        glm::vec4 specLe;
    };

    // DIRECTIONAL LIGHT
    struct dLight
    {
        glm::vec3 direction = glm::vec3(0.0f);
        glm::vec3 color = glm::vec4(0.0f);
        glm::vec4 specLe;
    };

    // AREA LIGHT
    struct aLight
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec3 color = glm::vec4(0.0f);
        glm::vec3 normal;
        glm::vec4 specLe;
        glm::vec3 u; // width & rotation
        glm::vec3 v; // height & rotation
    };

    // FUNCTION SIGNATURES
    glm::vec4 pointLight(pLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi, float &dist);
    glm::vec4 directionalLight(dLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi);
    glm::vec4 areaLight(aLight light, Ray ray, HitInfo hitInfo, glm::vec3 &wi, float &dist);
};