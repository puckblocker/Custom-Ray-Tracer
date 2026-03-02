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
    Intersect intersect;
    Light light;
    Camera camera;

    // Scene Objects
    std::vector<Intersect::Sphere> spheres;
    std::vector<Intersect::Plane> planes;
    std::vector<Intersect::Triangle> triangles;
    std::vector<Intersect::xForm> xForms;
    Light::pLight pointLight;
    Light::dLight directionalLight;

    // FUNCTION SIGNATURES
    glm::vec3 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0);
    float DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh);
    float GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi);
    glm::vec3 BRDF(glm::vec3 R, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi);
    void render(float *pixelBuffer, int resWidth, int resHeight);
    glm::vec3 tracer(Ray ray, unsigned int depth);
    void loadScene(const std::string &filename);
};