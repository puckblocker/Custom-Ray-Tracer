#pragma once

#include <glm/glm.hpp>

#include "rayData.h"

// Info On Intersection
struct HitInfo
{
    glm::vec3 point;
    bool valid;
    float distance;
    unsigned int objID;
    glm::vec3 normal; // normal to surface at intersect point

    // Materials
    struct Material
    {
        glm::vec3 albedo; // object's reflectance/color
        float roughness;  // 0 = smooth // 1 = rough
        bool metallic;    // metallic / conductor
        float ior;        // Index of Refraction (is object glass)
        bool emissive;    // is object a light
    };

    Material mat;
};

// Info On Transforms
struct xForm
{
    int crntID;
    int prntID;
    glm::mat4 transform; // matrix transforms
};

class Intersect
{
public:
    // SPHERE
    struct Sphere
    {
        float radius;
        glm::vec3 center;
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        bool metallic;
        float ior;
        float emissive;
    };

    // TRIANGLE
    struct Triangle
    {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float ior;
        float emissive;
    };

    // PLANE
    struct Plane
    {
        glm::vec3 position;
        glm::vec3 normal;
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float ior;
        float emissive;
    };

    // FUNCTION SIGNATURES
    HitInfo intersectSphere(Ray ray, Sphere sphere);
    HitInfo intersectTriangle(Ray ray, Triangle triangle);
    HitInfo intersectPlane(Ray ray, Plane plane);
};