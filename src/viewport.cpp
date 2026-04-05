#include "viewport.h"

using namespace Help;

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

    // Lens Distance Calculations
    focalDist = 0.5f * focusDist;

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
    glm::vec3 smpleSqr = glm::vec3(RandFloat() - 0.5f, RandFloat() - 0.5f, 0.0f); // random point in sample area
    glm::vec3 pixelOffset = smpleSqr;

    // Jitter The Pixel
    glm::vec2 jitter = glm::vec2(RandFloat(), RandFloat()); // randomize the sample position

    // Find Ray Direction (Pinhole)
    glm::vec3 pixelPos = viewport.origin + (((float)i + jitter.x) * image.pixelWidth * basis.xhat) +
                         (((float)j + jitter.y) * image.pixelHeight * basis.yhat);
    ray.direction = glm::normalize(pixelPos - origin); // Calculated by geting normalized unit of the vector between ray origin and camera origin

    // DEPTH OF FIELD CALCUlATIONS (Lens)
    glm::vec3 focalPoint = origin + (ray.direction * focusDist); // focal point from the focus point and the focus distance
    glm::vec3 lensSample = lensRandom() * (lensDiameter / 2.0f); // grab random value within the lens radius

    // Ray Origin
    ray.origin = origin + (lensSample.x * basis.xhat) + (lensSample.y * basis.yhat); // moves ray origin back to camera origin

    // Ray Direction
    ray.direction = glm::normalize(focalPoint - ray.origin);

    // Get a Random Time (Motion Blur)
    ray.time = RandFloat();

    return ray;
}

// Random Point On Lens
glm::vec3 Camera::lensRandom()
{
    while (1)
    {
        // Generate Random Point Within -1 and 1
        glm::vec3 point = glm::vec3(RandFloat() * 2.0f - 1.0f, RandFloat() * 2.0f - 1.0f, 0.0f);

        // Check For Point Inside Lens
        if (glm::length2(point) < 1.0f)
            return point;
    }
}