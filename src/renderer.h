#pragma once
#include "config.h"
#include "rayData.h"
#include "intersection.h"
#include "lighting.h"
#include "viewport.h"
#include <string>
#include <fstream>
#include <filesystem>

class Renderer
{
public:
    // VARIABLES
    Intersect intersect;
    Light light;
    Camera camera;
    // Scene Objects
    Intersect::Sphere sphere;
    Intersect::Plane plane;
    Intersect::Triangle triangle;
    Light::pLight pointLight;
    // Light::dLight directionalLight;

    void render(float *pixelBuffer, int resWidth, int resHeight)
    {
        camera.camViewUpdate();
        // FRAME BUFFER
        // Loop Through Pixel Rows (Preserves i and j pixels)
        for (int j = 0; j < resHeight; j++) // render all pixels
        {
            for (int i = 0; i < resWidth; i++)
            {
                // Generate Ray
                Ray ray = camera.rayGeneration(i, j);

                // Call Tracer
                glm::vec3 color = tracer(ray, 0);

                // Grab RGB Components
                int index = (j * resWidth + i) * 3; // multiply by 3 to account for RGB components and resWidth to prevent overwriting pixels
                pixelBuffer[index] = color.x;       // grab red value
                pixelBuffer[index + 1] = color.y;   // grab green value
                pixelBuffer[index + 2] = color.z;   // grab blue value
            }
        }
    }

    glm::vec3 tracer(Ray ray, unsigned int depth)
    {
        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 specCoeff = glm::vec3(0.0f);

        // Check For Max Depth
        if (depth >= 5)
            return color;

        // First Intersect
        HitInfo hitInfo;
        hitInfo.valid = false;
        hitInfo.distance = 1000000.0f;

        // Check For Sphere
        HitInfo tempHit = intersect.intersectSphere(ray, sphere);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }

        // Check For Plane
        tempHit = intersect.intersectPlane(ray, plane);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }

        // Check For Triangle
        // tempHit = intersect.intersectTriangle(ray, triangle);
        // if (tempHit.valid && tempHit.distance < hitInfo.distance)
        // {
        //     hitInfo = tempHit;
        // }

        // Generate Light
        if (hitInfo.valid)
        {
            // Shadow Variables
            glm::vec3 toLight = pointLight.origin - hitInfo.point;
            float distToLight = glm::length(toLight);
            bool inShadow = false;

            // Generate Shadows
            Ray shadowRay;
            shadowRay.origin = hitInfo.point + (hitInfo.normal * 0.01f); // offset to avoid self shadowing
            shadowRay.direction = glm::normalize(toLight);

            // Shadow Block Check
            // Sphere
            HitInfo shadowHit = intersect.intersectSphere(shadowRay, sphere);
            if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID) // Check for hit and behind object
                inShadow = true;

            // Triangle
            // shadowHit = intersect.intersectTriangle(shadowRay, triangle);
            // if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID)
            //     inShadow = true;

            // Plane
            shadowHit = intersect.intersectPlane(shadowRay, plane);
            if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID)
                inShadow = true;

            // Shadow Check
            if (inShadow)
            {
                // Set Color
                specCoeff = glm::vec3(0.05f) * hitInfo.mat.albedo;
            }
            else
            {
                glm::vec3 viewDir = glm::normalize(camera.origin - hitInfo.point); // w0
                // Set Color
                glm::vec3 pLightRad = light.pointLight(hitInfo.point, pointLight, hitInfo, viewDir);
                // glm::vec3 dLightColor = light.directionalLight(directionalLight, hitInfo, viewDir);
                // color = pLightColor + dLightColor;
                specCoeff = pLightRad;
            }

            // WHITTED RAY TRACER
            glm::vec3 indirectRad;

            // Metallic Surface
            if (hitInfo.mat.metallic)
            {
                // Reflection Ray
                Ray reflctRay;
                reflctRay.origin = hitInfo.point + (hitInfo.normal * 0.001f);
                reflctRay.direction = glm::reflect(ray.direction, hitInfo.normal);
                // reflctRay.direction = ray.direction - 2.0f * glm::dot(ray.direction, hitInfo.normal) * hitInfo.normal;

                glm::vec3 reflctColor = glm::vec3(1.0f) * tracer(reflctRay, depth + 1);

                color = specCoeff + (reflctColor * hitInfo.mat.albedo);
            }

            // Glass Surface
            else if (hitInfo.mat.ior >= 1.5f)
            {
                // Reflection Ray
                Ray reflctRay;
                reflctRay.origin = hitInfo.point + (hitInfo.normal * 0.001f);
                reflctRay.direction = glm::reflect(ray.direction, hitInfo.normal);

                glm::vec3 reflctColor = glm::vec3(1.0f) * tracer(reflctRay, depth + 1);

                // Refraction Ray
                Ray refrctRay;
                refrctRay.origin = hitInfo.point + (hitInfo.normal * 0.001f);
                refrctRay.direction = glm::refract(-ray.direction, -hitInfo.normal, hitInfo.mat.ior);

                glm::vec3 refrctColor = glm::vec3(1.0f) * tracer(reflctRay, depth + 1);

                color = specCoeff + (reflctColor * refrctColor);
            }

            else
            {
                color = specCoeff;
            }

            // WHITTED RAY TRACER
            // glm::vec3 indirectColor = glm::vec3(0.0f);

            // Mirror Surface
            // if (hitInfo.mat.metallic > 0.1f)
            // {
            //     // Ray Setup
            //     Ray reflectRay;
            //     reflectRay.origin = hitInfo.point + (hitInfo.point * 0.001f);
            //     reflectRay.direction = glm::reflect(ray.direction, hitInfo.normal);

            //     // Call Itself (Recursion)
            //     indirectColor += tracer(reflectRay, depth + 1) * hitInfo.mat.albedo;
            // }

            // Glass / Translucent Surface
            // if (hitInfo.mat.ior > 1.0f)
            // {
            //     // Variables / Setup
            //     float ior = hitInfo.mat.ior;
            //     float refractIndex = 1.0f / ior; // measure of air to glass ratio
            //     glm::vec3 normal = hitInfo.normal;

            //     // Check For Inside Object
            //     if (glm::dot(ray.direction, hitInfo.normal) > 0.0f)
            //     {
            //         refractIndex = ior;
            //         // Flip Normal
            //         normal = -hitInfo.normal;
            //     }

            //     glm::vec3 refractDir = glm::refract(ray.direction, normal, refractIndex);

            //     // Check For Internal Reflection
            //     if (glm::length(refractDir) > 0.001f)
            //     {
            //         // Ray Setup
            //         Ray refractRay;
            //         refractRay.origin = hitInfo.point - (normal * 0.001f);
            //         refractRay.direction = refractDir;

            //         // Recursion
            //         indirectColor += tracer(refractRay, depth + 1);
            //     }
            //     else
            //     {
            //         // Ray Setup
            //         Ray reflectRay;
            //         reflectRay.origin = hitInfo.point + (hitInfo.point * 0.001f);
            //         reflectRay.direction = glm::reflect(ray.direction, hitInfo.normal);

            //         // Recursion
            //         indirectColor += tracer(reflectRay, depth + 1) * hitInfo.mat.albedo;
            //     }
            // }

            // Color For Glass
            // if (hitInfo.mat.ior > 1.0f)
            // {
            //     return indirectColor;
            // }
            // else
            // {
            //     // Set Color
            //     return indirectColor + color;
            // }

            return color;
        }
        else
        {
            // Set Color
            return glm::vec3(0.69f, 0.88f, 1.0f);
        }
    }

    void loadScene(const std::string &filename)
    {
        std::ifstream file(filename);

        // Check For File
        if (!file.is_open())
        {
            std::cout << "Failed to Open Scene File: " << filename << std::endl;
            return;
        }

        // Object Type
        std::string type;
        unsigned int objID = 0;

        // Read File
        while (file >> type)
        {
            if (type == "Sphere")
            {
                // Shape Stats
                file >> sphere.center.x >> sphere.center.y >> sphere.center.z;
                file >> sphere.radius;
                // Material Stats
                file >> sphere.albedo.r >> sphere.albedo.g >> sphere.albedo.b;
                file >> sphere.metallic >> sphere.roughness >> sphere.ior >> sphere.emissive;
                sphere.objID += objID;
                std::cout << "Sphere Position: " << sphere.center.x << " " << sphere.center.y << " " << sphere.center.z << std::endl;
                std::cout << "Radius: " << sphere.radius << std::endl;
                std::cout << "Metallic: " << sphere.metallic << " Roughness: " << sphere.roughness << " IOR: " << sphere.ior << " Emissive: " << sphere.emissive << std::endl;
            }
            // else if(type == "triangle")
            // {
            //     // Shape Stats
            //     file >> triangle.center.x >> triangle.center.y >> triangle.center.z;
            //     file >> triangle.radius;
            //     // Material Stats
            //     file >> triangle.albedo .r >> triangle.albedo.g >> triangle.albedo.b;
            //     file >> triangle.metallic >> triangle.roughness >> triangle.ior >> triangle.emissive;
            //     triangle.objID += objID;
            // }
            else if (type == "Plane")
            {
                // Shape Stats
                file >> plane.position.x >> plane.position.y >> plane.position.z;
                file >> plane.normal.x >> plane.normal.y >> plane.normal.z;
                // Material Stats
                file >> plane.albedo.r >> plane.albedo.g >> plane.albedo.b;
                file >> plane.metallic >> plane.roughness >> plane.ior >> plane.emissive;
                plane.objID += objID;
            }
            else if (type == "pLight")
            {
                // Light Stats
                file >> pointLight.origin.x >> pointLight.origin.y >> pointLight.origin.z;
                file >> pointLight.color.r >> pointLight.color.g >> pointLight.color.b;
            }
            // else if (type == "dLight")
            // {
            //     // Light Stats
            //     file >> directionalLight.direction.x >> directionalLight.direction.y >> directionalLight.direction.z;
            //     file >> directionalLight.color.r >> directionalLight.color.g >> directionalLight.color.b;
            // }
        }
        file.close();
    }
};