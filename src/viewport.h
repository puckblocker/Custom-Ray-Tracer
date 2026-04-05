#pragma once // Ensures header file only include once throughout program

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include "rayData.h"
#include "helper.h"

class Camera
{
public:
    // Camera
    glm::vec3 origin = glm::vec3(0.0f); // focal point, standard right hand coords
    glm::vec3 up = glm::vec3(0.0f);     // vertical orientation
    glm::vec3 gaze = glm::vec3(0.0f);   // direction camera is facing
    float length;                       // distance from viewport center to camera
    float lensDiameter;                 // size of camera lens (user provided)
    float focusDist;                    // camera focus distance (user provided)
    float focalDist;                    // camera focal distance

    // Viewport
    struct Viewport
    {
        float height; // values so view is simplified to between -1 and +1
        float width;
        glm::vec3 origin;
    };

    // Image
    struct Image
    {
        int width = 640;
        int height = 640;
        float pixelHeight; // individual pixel height
        float pixelWidth;
    };

    // Camera's frame (right, up, forward)
    struct Basis
    {
        glm::vec3 xhat; // right
        glm::vec3 yhat; // up
        glm::vec3 zhat; // forward
    };

    // Instantiate Structs
    Viewport viewport;
    Image image;
    Basis basis;

    // FUNCTION SIGNATURES
    Ray rayGeneration(float i, float j);
    void camViewUpdate();
    glm::vec3 lensRandom();
};