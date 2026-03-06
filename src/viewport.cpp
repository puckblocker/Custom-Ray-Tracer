#include "viewport.h"

// Camera Viewport
void Camera::camViewUpdate()
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
Ray Camera::rayGeneration(float i, float j)
{
    // Instantiate struct
    Ray ray;

    // Pixel Sample
    glm::vec3 smpleSqr = glm::vec3(rand() / (RAND_MAX + 1.0f) - 0.5f, rand() / (RAND_MAX + 1.0f) - 0.5f, 0.0f); // random point in sample area
    glm::vec3 pixelOffset = smpleSqr;

    // Jitter The Pixel
    glm::vec2 jitter = glm::vec2(rand() / (RAND_MAX + 1.0f), rand() / (RAND_MAX + 1.0f)); // randomize the sample position

    // Find the Ray Origin // TEST PIXEL OFFSET of .5 <-
    // ray.origin = viewport.origin + (((float)i + 0.5f) * image.pixelWidth * basis.xhat) + (((float)j + 0.5f) * image.pixelHeight * basis.yhat); // Calculated through the our basis and pixel dimensions
    glm::vec3 pixelPos = viewport.origin + (((float)i + jitter.x) * image.pixelWidth * basis.xhat) +
                         (((float)j + jitter.y) * image.pixelHeight * basis.yhat);
    ray.origin = origin; // moves ray origin back to camera origin
    // Find Ray Direction
    ray.direction = glm::normalize(pixelPos - origin); // Calculated by geting normalized unit of the vector between ray origin and camera origin

    // Get a Random Time (Motion Blur)
    ray.time = rand() / (RAND_MAX + 1.0f);

    return ray;
}