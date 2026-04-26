#include "intersection.h"
#include <vector>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Intersect
{
    // Prep Local Transforms
    LocalSpaceData localSpacePrep(Ray ray, const Shape &shape, const std::vector<xForm> &xFormArray)
    {
        // TRANSFORM PREP
        // Variables
        LocalSpaceData data;
        int crntIndx = shape.objID;
        int prntIndx = -1;

        // Matrices
        glm::mat4 localTrn = glm::mat4(1.0f);
        glm::mat4 prntTrn = glm::mat4(1.0f);

        // Loop Through Indices
        for (const auto &xf : xFormArray)
        {
            if (xf.crntID == crntIndx)
            {
                localTrn = xf.transform;
                prntIndx = xf.prntID;
                break;
            }
        }

        // Global Parent Check
        bool prntFound = false;
        while (prntIndx != -1) // loop until global parent
        {
            for (int i = 0; i < xFormArray.size(); i++)
            {
                if (xFormArray[i].crntID == prntIndx)
                {
                    prntTrn = xFormArray[i].transform * prntTrn; // combine the parent transforms
                    prntIndx = xFormArray[i].prntID;             // go to next parent
                    prntFound = true;
                    break;
                }
            }

            if (!prntFound)
                break;
        }

        // Matrix Multiplication
        glm::mat4 crntTrn = prntTrn * localTrn;

        // Scale
        float scale = glm::length(glm::vec3(crntTrn[0]));
        float invScale = 1.0f / scale; // gets local scale (shrink ray instead of enlarge shape)

        // Rotation (Grab Rotation Matrix Vals)
        glm::mat3 rot = glm::mat3(crntTrn) * invScale; // grabs rotation values and purges scale
        glm::mat3 invRot = glm::transpose(rot);

        // Translation
        glm::vec3 trans = glm::vec3(crntTrn[3]);
        glm::vec3 invTrans = -(invRot * trans);

        // FRAME CHANGE (Global to Local)
        // Basis Change
        glm::mat4 basisChange = glm::mat4(glm::vec4(invRot[0] * invScale, 0.0f),
                                          glm::vec4(invRot[1] * invScale, 0.0f),
                                          glm::vec4(invRot[2] * invScale, 0.0f),
                                          glm::vec4(invTrans, 1.0f));

        data.localRay.origin = glm::vec3(basisChange * glm::vec4(ray.origin, 1.0f));
        data.localRay.direction = glm::normalize(basisChange * glm::vec4(ray.direction, 0.0f));
        data.nrmMtrx = rot;
        data.wrldToLocal = crntTrn;

        return data;
    }

    // SPHERE INTERSECTION
    HitInfo intersectSphere(Ray ray, Sphere sphere, std::vector<xForm> xFormArray)
    {
        // Variables
        LocalSpaceData ls = localSpacePrep(ray, sphere, xFormArray);
        HitInfo hitInfo;
        hitInfo.valid = false;

        float crntDist = 0.0f;
        int maxSteps = 100;

        // Fixes For Dielectrics
        float startSDF = glm::length(ls.localRay.origin - sphere.center) - sphere.radius; // starting sdf to prevent negative distance inside sphere
        float raySign;                                                                    // essentially a flip switch to help with starting sdf

        if (startSDF < 0.0f)
        {
            raySign = -1.0f;
        }
        else
        {
            raySign = 1.0f;
        }

        // MOTION BLUR (Move Sphere Based on Time)
        float velocity = 0.10f;

        if (sphere.animated == true)
        {
            sphere.center += ray.time * velocity;
        }

        // Ray Marching
        for (int i = 0; i < maxSteps; i++)
        {
            // CALCULATIONS
            glm::vec3 point = ls.localRay.origin + ls.localRay.direction * crntDist;
            float sdf = glm::length(point - sphere.center) - sphere.radius; // signed distance function (distance from current position to object)

            float newSDF = sdf * raySign; // new sdf to prevent negative distance

            // INTERSECT CHECKS
            if (newSDF < 0.0001f)
            {
                hitInfo.valid = true;
                hitInfo.distance = crntDist;
                hitInfo.objID = sphere.objID;
                hitInfo.mat = {sphere.albedo, sphere.roughness, sphere.metallic, sphere.ior, sphere.emissive > 0.0f, sphere.z, sphere.layerIOR};
                hitInfo.point = glm::vec3(ls.wrldToLocal * glm::vec4(point, 1.0f));
                glm::vec3 localNorm = glm::normalize(point - sphere.center);
                hitInfo.normal = glm::normalize(ls.nrmMtrx * localNorm);

                return hitInfo;
            }

            // March
            crntDist += newSDF; // current step distance (how far step will travel)

            // Break Out
            if (crntDist > 100.0f)
                break; // unlikely to hit, optimization
        }

        // Return Hit Information
        return hitInfo;
    }

    HitInfo Intersect::intersectTriangle(Ray ray, Triangle triangle, std::vector<xForm> xFormArray)
    {
        // Variables
        LocalSpaceData ls = localSpacePrep(ray, triangle, xFormArray);
        HitInfo hitInfo;
        hitInfo.valid = false;

        glm::vec3 edge1 = triangle.p1 - triangle.p0;
        glm::vec3 edge2 = triangle.p2 - triangle.p0;
        glm::vec3 h = glm::cross(ls.localRay.direction, edge2);
        float a = glm::dot(edge1, h);

        // CHECK FOR PARALLEL OR INSIDE PLANE OF TRIANGLE (If a is near 0)
        if (glm::abs(a) < 0.0001)
        {
            hitInfo.valid = false;
            return hitInfo;
        }

        float f = 1.0f / a;
        glm::vec3 s = ls.localRay.origin - triangle.p0;
        float u = f * glm::dot(s, h);

        // INTERSECTION OUT OF TRIANGLE BOUNDS
        if (u < 0.0f || u > 1.0f)
        {
            hitInfo.valid = false;
            return hitInfo;
        }

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ls.localRay.direction, q);

        if (v < 0.0f || u + v > 1.0f)
        {
            hitInfo.valid = false;
            return hitInfo;
        }

        // CALCULATE DISTANCE
        float t = f * glm::dot(edge2, q);

        // HIT CHECK
        if (t > 0.0001) // epsilon prevent self-intersection
        {
            // Fill Hit Info Traits
            hitInfo.valid = true;
            hitInfo.objID = triangle.objID;
            hitInfo.distance = t; // distance
            hitInfo.mat = {triangle.albedo, triangle.roughness, triangle.metallic, triangle.ior, triangle.emissive > 0.0f, triangle.z, triangle.layerIOR};
            glm::vec3 localPoint = ls.localRay.origin + ls.localRay.direction * t; // calculate point
            hitInfo.point = glm::vec3(ls.wrldToLocal * glm::vec4(localPoint, 1.0f));
            glm::vec3 localNorm = glm::cross(edge1, edge2); // calculate normal
            hitInfo.normal = glm::normalize(ls.nrmMtrx * localNorm);
            return hitInfo;
        }

        else
        {
            hitInfo.valid = false;
        }

        return hitInfo;
    }

    // PLANE INTERSECTION
    HitInfo Intersect::intersectPlane(Ray ray, Plane plane, std::vector<xForm> xFormArray)
    {
        // Variables
        LocalSpaceData ls = localSpacePrep(ray, plane, xFormArray);
        HitInfo hitInfo;
        hitInfo.valid = false;

        float crntDist = 0.0f;
        int maxSteps = 200;
        float threshold = 0.001f;

        // Ray Marching
        for (int i = 0; i < maxSteps; i++)
        {
            // CALCULATIONS
            glm::vec3 point = ls.localRay.origin + ls.localRay.direction * crntDist;
            float sdf = glm::abs(glm::dot((point - plane.position), plane.normal)); // signed distance function (distance from current position to object)

            // INTERSECT CHECKS
            if (glm::epsilonEqual(sdf, 0.0f, threshold))
            {
                hitInfo.valid = true;
                hitInfo.distance = crntDist;
                hitInfo.objID = plane.objID;
                hitInfo.mat = {plane.albedo, plane.roughness, plane.metallic, plane.ior, plane.emissive > 0.0f, plane.z, plane.layerIOR};
                hitInfo.point = glm::vec3(ls.wrldToLocal * glm::vec4(point, 1.0f));
                hitInfo.normal = glm::normalize(ls.nrmMtrx * plane.normal);

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
}