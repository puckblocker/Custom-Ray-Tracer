#include "intersection.h"
#include <vector>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>

// SPHERE INTERSECTION
HitInfo Intersect::intersectSphere(Ray ray, Sphere sphere, std::vector<xForm> xFormArray)
{
    // TRANSFORMS
    // Variables
    HitInfo hitInfo;

    int crntIndx = sphere.objID;
    int prntIndx = -1;

    // Matrices
    glm::mat4 crntTrn;
    glm::mat4 localTrn = glm::mat4(1.0f);
    glm::mat4 prntTrn;
    glm::mat4 idntMat = glm::mat4(1.0f); // dont alter last row (affine)
    prntTrn = idntMat;

    // Object Indices / Parent Indices
    for (int i = 0; i < xFormArray.size(); i++)
    {
        // Check For Current Index
        if (xFormArray[i].crntID == crntIndx)
        {
            localTrn = xFormArray[i].transform;
            prntIndx = xFormArray[i].prntID; // set parent index
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
    crntTrn = prntTrn * localTrn;

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

    // Local Ray
    Ray localRay;
    localRay.origin = glm::vec3(basisChange * glm::vec4(ray.origin, 1.0f));
    localRay.direction = glm::normalize(basisChange * glm::vec4(ray.direction, 0.0f));

    // INTERSECTION

    // Variables
    float crntDist = 0.0f;
    hitInfo.valid = false;
    int maxSteps = 100;

    // Fixes For Dielectrics
    float startSDF = glm::length(localRay.origin - sphere.center) - sphere.radius; // starting sdf to prevent negative distance inside sphere
    float raySign;                                                                 // essentially a flip switch to help with starting sdf

    if (startSDF < 0.0f)
    {
        raySign = -1.0f;
    }
    else
    {
        raySign = 1.0f;
    }

    // Ray Marching
    for (int i = 0; i < maxSteps; i++)
    {
        // CALCULATIONS
        glm::vec3 point = localRay.origin + localRay.direction * crntDist;
        float sdf = glm::length(point - sphere.center) - sphere.radius; // signed distance function (distance from current position to object)

        float newSDF = sdf * raySign; // new sdf to prevent negative distance

        // INTERSECT CHECKS
        if (newSDF < 0.0001f)
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
            glm::vec3 localNorm = glm::normalize(point - sphere.center);
            hitInfo.normal = glm::normalize(rot * localNorm);

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

// TRIANGLE INTERSECTION
// HitInfo Intersect::intersectTriangle(Ray ray, Triangle triangle, std::vector<xForm> xFormArray)
// {
//     // TRANSFORMS
//     // Variables
//     HitInfo hitInfo;

//     int crntIndx = triangle.objID;
//     int prntIndx = -1;

//     // Matrices
//     glm::mat4 crntTrn;
//     glm::mat4 localTrn = glm::mat4(1.0f);
//     glm::mat4 prntTrn;
//     glm::mat4 idntMat = glm::mat4(1.0f); // dont alter last row (affine)
//     prntTrn = idntMat;

//     // Object Indices / Parent Indices
//     for (int i = 0; i < xFormArray.size(); i++)
//     {
//         // Check For Current Index
//         if (xFormArray[i].crntID == crntIndx)
//         {
//             localTrn = xFormArray[i].transform;
//             prntIndx = xFormArray[i].prntID; // set parent index
//             break;
//         }
//     }

//     // Global Parent Check
//     bool prntFound = false;
//     while (prntIndx != -1) // loop until global parent
//     {
//         for (int i = 0; i < xFormArray.size(); i++)
//         {
//             if (xFormArray[i].crntID == prntIndx)
//             {
//                 prntTrn = xFormArray[i].transform * prntTrn; // combine the parent transforms
//                 prntIndx = xFormArray[i].prntID;             // go to next parent
//                 prntFound = true;
//                 break;
//             }
//         }

//         if (!prntFound)
//             break;
//     }

//     // Matrix Multiplication
//     crntTrn = prntTrn * localTrn;

//     // Scale
//     float scale = glm::length(glm::vec3(crntTrn[0]));
//     float invScale = 1.0f / scale; // gets local scale (shrink ray instead of enlarge shape)

//     // Rotation (Grab Rotation Matrix Vals)
//     glm::mat3 rot = glm::mat3(crntTrn) * invScale; // grabs rotation values and purges scale
//     glm::mat3 invRot = glm::transpose(rot);

//     // Translation
//     glm::vec3 trans = glm::vec3(crntTrn[3]);
//     glm::vec3 invTrans = -(invRot * trans);

//     // FRAME CHANGE (Global to Local)
//     // Basis Change
//     glm::mat4 basisChange = glm::mat4(glm::vec4(invRot[0] * invScale, 0.0f),
//                                       glm::vec4(invRot[1] * invScale, 0.0f),
//                                       glm::vec4(invRot[2] * invScale, 0.0f),
//                                       glm::vec4(invTrans, 1.0f));

//     // Local Ray
//     Ray localRay;
//     localRay.origin = glm::vec3(basisChange * glm::vec4(ray.origin, 1.0f));
//     localRay.direction = glm::normalize(basisChange * glm::vec4(ray.direction, 0.0f));

//     // INTERSECTION

//     // Variables
//     float crntDist = 0.0f;
//     hitInfo.valid = false;
//     int maxSteps = 200;

//     // Ray Marching
//     for (int i = 0; i < maxSteps; i++)
//     {
//         // POINT CALCULATION
//         glm::vec3 point = localRay.origin + localRay.direction * crntDist;

//         // SDF
//         // Edges
//         glm::vec3 ba = triangle.b - triangle.a;
//         glm::vec3 cb = triangle.c - triangle.b;
//         glm::vec3 ac = triangle.a - triangle.c;

//         // Vectors (Corner to Current Position)
//         glm::vec3 pointA = point - triangle.a;
//         glm::vec3 pointB = point - triangle.b;
//         glm::vec3 pointC = point - triangle.c;

//         // Create Perpendicular Vector To Edges
//         glm::vec3 norm = glm::cross(ba, ac); // which way triangle is facing

//         // Signed Distance Function // ASK TEACHER TO HELP EXPLAIN
//         float sdf = glm::sqrt(
//             // cross: create new vector pointing outward // dot: check if point is inside or outside // If sum = 3, then inside all 3 edges, 2, then inside 2
//             (glm::sign(glm::dot(glm::cross(ba, norm), pointA)) +
//                  glm::sign(glm::dot(glm::cross(cb, norm), pointB)) +
//                  glm::sign(glm::dot(glm::cross(ac, norm), pointC)) <
//              2.0f)
//                 ? // true = near edge, false = near face
//                 glm::min(
//                     glm::min(
//                         glm::dot(ba * glm::clamp(glm::dot(ba, pointA) / glm::dot(ba, ba), 0.0f, 1.0f) - pointA,
//                                  ba * glm::clamp(glm::dot(ba, pointA) / glm::dot(ba, ba), 0.0f, 1.0f) - pointA),

//                         glm::dot(cb * glm::clamp(glm::dot(cb, pointB) / glm::dot(cb, cb), 0.0f, 1.0f) - pointB,
//                                  cb * glm::clamp(glm::dot(cb, pointB) / glm::dot(cb, cb), 0.0f, 1.0f) - pointB)),
//                     glm::dot(ac * glm::clamp(glm::dot(ac, pointC) / glm::dot(ac, ac), 0.0f, 1.0f) - pointC,
//                              ac * glm::clamp(glm::dot(ac, pointC) / glm::dot(ac, ac), 0.0f, 1.0f) - pointC))
//                 : glm::dot(norm, pointA) * glm::dot(norm, pointA) / glm::dot(norm, norm));

//         // INTERSECTION CHECK
//         if (sdf < 0.001f)
//         {
//             hitInfo.valid = true;
//             hitInfo.distance = crntDist;
//             hitInfo.objID = triangle.objID;
//             hitInfo.mat.albedo = triangle.albedo;
//             hitInfo.mat.roughness = triangle.roughness;
//             hitInfo.mat.metallic = triangle.metallic;
//             hitInfo.mat.ior = triangle.ior;
//             hitInfo.mat.emissive = triangle.emissive;
//             hitInfo.point = point;
//             hitInfo.normal = glm::normalize(rot * norm);

//             return hitInfo;
//         }

//         // March
//         crntDist += sdf;

//         // Break Out
//         if (crntDist > 100.0f)
//             break; // unlikely to hit, optimization
//     }

//     // Return Hit Information
//     return hitInfo;
// }

HitInfo Intersect::intersectTriangle(Ray ray, Triangle triangle, std::vector<xForm> xFormArray)
{
    // TRANSFORMS
    // Variables
    HitInfo hitInfo;

    int crntIndx = triangle.objID;
    int prntIndx = -1;

    // Matrices
    glm::mat4 crntTrn;
    glm::mat4 localTrn = glm::mat4(1.0f);
    glm::mat4 prntTrn;
    glm::mat4 idntMat = glm::mat4(1.0f); // dont alter last row (affine)
    prntTrn = idntMat;

    // Object Indices / Parent Indices
    for (int i = 0; i < xFormArray.size(); i++)
    {
        // Check For Current Index
        if (xFormArray[i].crntID == crntIndx)
        {
            localTrn = xFormArray[i].transform;
            prntIndx = xFormArray[i].prntID; // set parent index
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
    crntTrn = prntTrn * localTrn;

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

    // Local Ray
    Ray localRay;
    localRay.origin = glm::vec3(basisChange * glm::vec4(ray.origin, 1.0f));
    localRay.direction = glm::normalize(basisChange * glm::vec4(ray.direction, 0.0f));

    // INTERSECTION

    // Variables
    glm::vec3 edge1 = triangle.p1 - triangle.p0;
    glm::vec3 edge2 = triangle.p2 - triangle.p0;
    glm::vec3 h = glm::cross(localRay.direction, edge2);
    float a = glm::dot(edge1, h);

    // CHECK FOR PARALLEL OR INSIDE PLANE OF TRIANGLE (If a is near 0)
    if (glm::abs(a) < 0.0001)
    {
        hitInfo.valid = false;
    }

    float f = 1.0f / a;
    glm::vec3 s = localRay.origin - triangle.p0;
    float u = f * glm::dot(s, h);

    // INTERSECTION OUT OF TRIANGLE BOUNDS
    if (u < 0.0f || u > 1.0f)
    {
        hitInfo.valid = false;
        return hitInfo;
    }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(localRay.direction, q);

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
        glm::vec3 point = localRay.origin + localRay.direction * t;     // calculate point
        glm::vec3 localNorm = glm::normalize(glm::cross(edge1, edge2)); // calculate normal
        hitInfo.normal = glm::normalize(rot * localNorm);
        hitInfo.valid = true;
        hitInfo.distance = t; // distance
        hitInfo.objID = triangle.objID;
        hitInfo.mat.albedo = triangle.albedo;
        hitInfo.mat.roughness = triangle.roughness;
        hitInfo.mat.metallic = triangle.metallic;
        hitInfo.mat.ior = triangle.ior;
        hitInfo.mat.emissive = triangle.emissive;
        hitInfo.point = point;

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
    // TRANSFORMS
    // Variables
    HitInfo hitInfo;

    int crntIndx = plane.objID;
    int prntIndx = -1;

    // Matrices
    glm::mat4 crntTrn;
    glm::mat4 localTrn = glm::mat4(1.0f);
    glm::mat4 prntTrn;
    glm::mat4 idntMat = glm::mat4(1.0f); // dont alter last row (affine)
    prntTrn = idntMat;

    // Object Indices / Parent Indices
    for (int i = 0; i < xFormArray.size(); i++)
    {
        // Check For Current Index
        if (xFormArray[i].crntID == crntIndx)
        {
            localTrn = xFormArray[i].transform;
            prntIndx = xFormArray[i].prntID; // set parent index
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
    crntTrn = prntTrn * localTrn;

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

    // Local Ray
    Ray localRay;
    localRay.origin = glm::vec3(basisChange * glm::vec4(ray.origin, 1.0f));
    localRay.direction = glm::normalize(basisChange * glm::vec4(ray.direction, 0.0f));

    // INTERSECTION

    // Variables
    float crntDist = 0.0f;
    hitInfo.valid = false;
    int maxSteps = 200;
    float threshold = 0.001f;

    // Ray Marching
    for (int i = 0; i < maxSteps; i++)
    {
        // CALCULATIONS
        glm::vec3 point = localRay.origin + localRay.direction * crntDist;
        float sdf = glm::abs(glm::dot((point - plane.position), plane.normal)); // signed distance function (distance from current position to object)

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
            hitInfo.normal = glm::normalize(rot * plane.normal);

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