#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "rayData.h"
#include "intersection.h"
#include "lighting.h"
#include "viewport.h"

class Renderer
{
public:
    // VARIABLES
    Light light;
    Camera camera;

    // Scene Objects
    std::vector<Intersect::Sphere> spheres;
    std::vector<Intersect::Plane> planes;
    std::vector<Intersect::Triangle> triangles;
    std::vector<Intersect::xForm> xForms;
    Light::pLight pointLight;
    Light::dLight directionalLight;
    Light::aLight areaLight;

    // FUNCTION SIGNATURES
    void render(float *pixelBuffer, int resWidth, int resHeight, float &sampleCount);
    glm::vec4 tracer(Ray ray, unsigned int depth);

    // File Readers
    void loadCIE(const std::string &filename);
    void loadScene(const std::string &filename);
};