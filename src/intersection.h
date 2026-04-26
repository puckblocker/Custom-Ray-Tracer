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
        float metallic;   // metallic / conductor
        float ior;        // Index of Refraction (is object glass)
        bool emissive;    // is object a light
        float z;          // depth
        float layerIOR;   // ior for layered
    };

    Material mat;
};

namespace Intersect
{
    // Info On Transforms
    struct xForm
    {
        int crntID;
        int prntID;
        glm::mat4 transform; // matrix transforms
    };

    struct LocalSpaceData
    {
        Ray localRay;
        glm::mat4 wrldToLocal;
        glm::mat3 nrmMtrx;
    };

    // Parent Class
    struct Shape
    {
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float ior;
        float emissive;
        float z;        // depth
        float layerIOR; // ior for layered
        bool animated = false;
    };

    // Children
    // SPHERE
    struct Sphere : public Shape
    {
        float radius;
        glm::vec3 center;
    };

    // TRIANGLE
    struct Triangle : public Shape
    {
        glm::vec3 p0, p1, p2;
    };

    // PLANE
    struct Plane : public Shape
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    // FUNCTION SIGNATURES
    LocalSpaceData localSpacePrep(Ray ray, const Shape &shape, const std::vector<xForm> &xFormArray);
    HitInfo intersectSphere(Ray ray, Sphere sphere, std::vector<xForm> xFormArray);
    HitInfo intersectTriangle(Ray ray, Triangle triangle, std::vector<xForm> xFormArray);
    HitInfo intersectPlane(Ray ray, Plane plane, std::vector<xForm> xFormArray);
};