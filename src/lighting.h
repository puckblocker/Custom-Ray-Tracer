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
        glm::vec3 color = glm::vec3(0.0f);  // color / power
    };

    // DIRECTIONAL LIGHT
    struct dLight
    {
        glm::vec3 direction = glm::vec3(0.0f);
        glm::vec3 color = glm::vec3(0.0f);
    };

    // AREA LIGHT
    struct aLight
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 normal;
        glm::vec3 u; // width & rotation
        glm::vec3 v; // height & rotation
    };

    // FUNCTION SIGNATURES
    glm::vec3 pointLight(pLight light, HitInfo hitInfo, glm::vec3 &wi, float &dist);
    glm::vec3 directionalLight(dLight light, HitInfo hitInfo, glm::vec3 &wi);
    glm::vec3 areaLight(aLight light, HitInfo hitInfo, glm::vec3 &wi, float &dist);
};