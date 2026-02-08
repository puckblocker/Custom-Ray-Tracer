#pragma once // Ensures header file only include once throughout program
#include "config.h"
#include "rayData.h"

class Camera
{
public:
    // Camera
    glm::vec3 origin = glm::vec3(0.0f, 0.0f, 10.0f); // focal point, standard right hand coords
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);      // vertical orientation
    glm::vec3 gaze = glm::vec3(0.0f, 0.0f, -1.0f);   // direction camera is facing
    float length = 10.0f;                            // distance from viewport center to camera

    // Viewport
    struct Viewport
    {
        float height = 2.0f; // values so view is simplified to between -1 and +1
        float width = 2.0f;
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

    // Camera Viewport
    void camViewUpdate()
    {
        // TODO: Create camera and viewport
        // Build 3D Basis
        glm::vec3 normGaze = glm::normalize(gaze);
        glm::vec3 normUp = glm::normalize(up);                            // normalized up vector to help find the 3rd basis vector
        glm::vec3 orthoU = glm::normalize(glm::cross(normGaze, normUp));  // 3rd vector for cartesian that is perpendicular to the other vectors
        glm::vec3 orthoUp = glm::normalize(glm::cross(orthoU, normGaze)); // fix the up vector to be orthogonal to other vectors
        basis.xhat = orthoU;
        basis.yhat = orthoUp;
        basis.zhat = -normGaze; // standard right hand system is reason for negative (flips variable directions without it)

        // Viewport Origin Calculations
        glm::vec3 viewportCenter = origin + (length * gaze);
        viewport.origin = viewportCenter - 0.5f * (viewport.width * orthoU) - 0.5f * (viewport.height * orthoUp);

        // Pixel Calculations
        image.pixelHeight = viewport.height / image.height;
        image.pixelWidth = viewport.width / image.width;
    }

    // Ray Generation
    Ray rayGeneration(unsigned i, unsigned j)
    {
        // Instantiate struct
        Ray ray;

        // Find the Ray Origin // TEST PIXEL OFFSET of .5 <-
        ray.origin = viewport.origin + (((float)i + 0.5f) * image.pixelWidth * basis.xhat) + (((float)j + 0.5f) * image.pixelHeight * basis.yhat); // Calculated through the our basis and pixel dimensions
        // Find Ray Direction
        ray.direction = glm::normalize(ray.origin - origin); // Calculated by geting normalized unit of the vector between ray origin and camera origin

        return ray;
    }
};