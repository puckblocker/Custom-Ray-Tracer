#pragma once
#include "config.h"
#include "viewport.h"
#include "rayData.h"

// Infor On Intersection
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
    };

    Material mat;
};

class Intersect
{
public:
    // Variable
    Camera camera;

    // SPHERE
    struct Sphere
    {
        float radius;
        glm::vec3 center;
        unsigned int objID;
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float ior;
        float emissive;
    };

    HitInfo intersectSphere(Ray ray, Sphere sphere)
    {
        // Instantiate Struct
        HitInfo hitInfo;

        // Variables
        float crntDist = 0.0f;
        hitInfo.valid = false;
        int maxSteps = 10;

        // Ray Marching
        for (int i = 0; i < maxSteps; i++)
        {
            // CALCULATIONS
            glm::vec3 point = ray.origin + ray.direction * crntDist;
            float sdf = glm::length(point - sphere.center) - sphere.radius; // signed distance function (distance from current position to object)

            // FRAME CHANGE (global to local)
            // glm::mat4 fcGlobal = glm::mat4()

            // INTERSECT CHECKS
            if (sdf < 0.0001f)
            {
                hitInfo.valid = true;
                hitInfo.distance = crntDist;
                hitInfo.objID = sphere.objID;
                hitInfo.mat.albedo = sphere.albedo;
                hitInfo.mat.roughness = sphere.roughness;
                hitInfo.mat.metallic = sphere.metallic;
                hitInfo.mat.ior = sphere.ior;
                hitInfo.mat.emissive = sphere.emissive;
                hitInfo.point = point;
                hitInfo.normal = glm::normalize(point - sphere.center);

                return hitInfo;
            }

            // March
            crntDist += sdf; // current step distance (how far step will travel)

            // Break Out
            if (crntDist > 100.0f)
                break; // unlikely to hit, optimization
        }

        // Return Hit Information
        return hitInfo;
    }

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
    HitInfo intersectTriangle(Ray ray, Triangle triangle)
    {
        // Instantiate Struct
        HitInfo hitInfo;

        // Variables
        float crntDist = 0.0f;
        hitInfo.valid = false;
        int maxSteps = 10;

        // Ray Marching
        for (int i = 0; i < maxSteps; i++)
        {
            // POINT CALCULATION
            glm::vec3 point = ray.origin + ray.direction * crntDist;

            // SDF
            // Edges
            glm::vec3 ba = triangle.b - triangle.a;
            glm::vec3 cb = triangle.c - triangle.b;
            glm::vec3 ac = triangle.a - triangle.c;

            // Vectors (Corner to Current Position)
            glm::vec3 pointA = point - triangle.a;
            glm::vec3 pointB = point - triangle.b;
            glm::vec3 pointC = point - triangle.c;

            // Create Perpendicular Vector To Edges
            glm::vec3 norm = glm::cross(ba, ac); // which way triangle is facing

            // Signed Distance Function // ASK TEACHER TO HELP EXPLAIN
            float sdf = glm::sqrt(
                // cross: create new vector pointing outward // dot: check if point is inside or outside // If sum = 3, then inside all 3 edges, 2, then inside 2
                (glm::sign(glm::dot(glm::cross(ba, norm), pointA)) +
                     glm::sign(glm::dot(glm::cross(cb, norm), pointB)) +
                     glm::sign(glm::dot(glm::cross(ac, norm), pointC)) <
                 2.0f)
                    ? // true = near edge, false = near face
                    glm::min(
                        glm::min(
                            glm::dot(ba * glm::clamp(glm::dot(ba, pointA) / glm::dot(ba, ba), 0.0f, 1.0f) - pointA,
                                     ba * glm::clamp(glm::dot(ba, pointA) / glm::dot(ba, ba), 0.0f, 1.0f) - pointA),

                            glm::dot(cb * glm::clamp(glm::dot(cb, pointB) / glm::dot(cb, cb), 0.0f, 1.0f) - pointB,
                                     cb * glm::clamp(glm::dot(cb, pointB) / glm::dot(cb, cb), 0.0f, 1.0f) - pointB)),
                        glm::dot(ac * glm::clamp(glm::dot(ac, pointC) / glm::dot(ac, ac), 0.0f, 1.0f) - pointC,
                                 ac * glm::clamp(glm::dot(ac, pointC) / glm::dot(ac, ac), 0.0f, 1.0f) - pointC))
                    : glm::dot(norm, pointA) * glm::dot(norm, pointA) / glm::dot(norm, norm));

            // INTERSECTION CHECK
            if (sdf < 0.001f)
            {
                hitInfo.valid = true;
                hitInfo.distance = crntDist;
                return hitInfo;
            }

            // March
            crntDist += sdf;

            // Break Out
            if (crntDist > 100.0f)
                break; // unlikely to hit, optimization
        }

        // Return Hit Information
        return hitInfo;
    }

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

    HitInfo intersectPlane(Ray ray, Plane plane)
    {
        // Instantiate Struct
        HitInfo hitInfo;

        // Variables
        float crntDist = 0.0f;
        hitInfo.valid = false;
        int maxSteps = 50;
        float threshold = 0.001f;

        // Ray Marching
        for (int i = 0; i < maxSteps; i++)
        {
            // CALCULATIONS
            glm::vec3 point = ray.origin + ray.direction * crntDist;
            float sdf = glm::dot((point - plane.position), plane.normal); // signed distance function (distance from current position to object)
            // INTERSECT CHECKS
            if (glm::epsilonEqual(sdf, 0.0f, threshold))
            {
                hitInfo.valid = true;
                hitInfo.distance = crntDist;
                hitInfo.objID = plane.objID;
                hitInfo.mat.albedo = plane.albedo;
                hitInfo.mat.roughness = plane.roughness;
                hitInfo.mat.metallic = plane.metallic;
                hitInfo.mat.ior = plane.ior;
                hitInfo.mat.emissive = plane.emissive;
                hitInfo.point = point;
                hitInfo.normal = plane.normal;
                return hitInfo;
            }

            // March
            crntDist += sdf; // current step distance (how far step will travel)

            // Break Out
            if (crntDist > 100.0f)
                break; // unlikely to hit, optimization
        }

        // Return Hit Information
        return hitInfo;
    }
};