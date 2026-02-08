#pragma once // Ensures header file only include once throughout program
#include "config.h"
#include "rayData.h"

// Set Color
inline glm::vec3 setColor(Ray &ray, bool valid) // omits multiple definitions of the function
{
    // Color Change
    glm::vec3 color;
    if (valid == true)
    {
        color = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
        color = glm::vec3(0.0f);

    return color;
}