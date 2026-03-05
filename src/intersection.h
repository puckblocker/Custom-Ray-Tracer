#pragma once

#include <glm/glm.hpp>
#include <vector>

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

class Intersect
{
public:
    // Info On Transforms
    struct xForm
    {
        int crntID;
        int prntID;
        glm::mat4 transform; // matrix transforms
    };

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
        bool animated;
    };

    // TRIANGLE
    struct Triangle
    {
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float ior;
        float emissive;
        bool animated = false;
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
        bool animated = false;
    };

    // FUNCTION SIGNATURES
    HitInfo intersectSphere(Ray ray, Sphere sphere, std::vector<xForm> xFormArray);
    HitInfo intersectTriangle(Ray ray, Triangle triangle, std::vector<xForm> xFormArray);
    HitInfo intersectPlane(Ray ray, Plane plane, std::vector<xForm> xFormArray);
};