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
    std::vector<Intersect::Sphere> spheres;
    std::vector<Intersect::Plane> planes;
    std::vector<Intersect::Triangle> triangles;
    Light::pLight pointLight;
    Light::dLight directionalLight;

    // Fresnel Reflectance
    glm::vec3 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0)
    {
        glm::vec3 Fr = F0 + (1.0f - F0) * glm::pow((1.0f - glm::dot(normal, direction)), 5.0f);
        return Fr;
    }

    // ROUGH SPECULAR (Trowbridge-Reitz)
    // Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
    float DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh)
    {
        float pi = 3.14159265359;
        float D = alphaSqr / (pi * ((glm::dot(normal, wh) * glm::dot(normal, wh)) * (alphaSqr - 1) + 1));
        return D;
    }
    // Geometry Function (Approx relative surface area where micro surface details overshadow each other, causing light rays to be covered (from opengl))
    float GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi)
    {
        float w0HalfVec = glm::dot(normal, w0) / (glm::dot(normal, w0) * (1 - k) + k);
        float wiHalfVec = glm::dot(normal, wi) / (glm::dot(normal, wi) * (1 - k) + k);
        float G = glm::max(w0HalfVec, 0.0f) * glm::max(wiHalfVec, 0.0f); // clamped to prevent negative light (light from behind object)
        return G;
    }

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
        HitInfo tempHit;
        int i = 0;
        for (i = 0; i < spheres.size(); i++)
        {
            tempHit = intersect.intersectSphere(ray, spheres[i]);
            if (tempHit.valid && tempHit.distance < hitInfo.distance)
            {
                hitInfo = tempHit;
            }
        }

        // Check For Plane
        for (i = 0; i < planes.size(); i++)
        {
            tempHit = intersect.intersectPlane(ray, planes[i]);
            if (tempHit.valid && tempHit.distance < hitInfo.distance)
            {
                hitInfo = tempHit;
            }
        }

        // Check For Triangle
        for (i = 0; i < triangles.size(); i++)
        {
            tempHit = intersect.intersectTriangle(ray, triangles[i]);
            if (tempHit.valid && tempHit.distance < hitInfo.distance)
            {
                hitInfo = tempHit;
            }
        }

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
            HitInfo shadowHit;
            for (i = 0; i < spheres.size(); i++)
            {
                shadowHit = intersect.intersectSphere(shadowRay, spheres[i]);
                if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID) // Check for hit and behind object
                    inShadow = true;
            }

            // Triangle
            for (i = 0; i < triangles.size(); i++)
            {
                shadowHit = intersect.intersectTriangle(shadowRay, triangles[i]);
                if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID)
                    inShadow = true;
            }

            // Plane
            for (i = 0; i < planes.size(); i++)
            {
                shadowHit = intersect.intersectPlane(shadowRay, planes[i]);
                if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID)
                    inShadow = true;
            }

            // In Shadow
            if (inShadow)
            {
                // Set Color
                specCoeff = glm::vec3(0.05f) * hitInfo.mat.albedo;
            }
            // Not In Shadow
            else
            {
                // PBR BRDF CALCULATIONS (DIRECT LIGHTING)
                float pi = 3.14159265359;
                glm::vec3 R = hitInfo.mat.albedo;                             // reflectance
                glm::vec3 w0 = glm::normalize(camera.origin - hitInfo.point); // outgoing direction
                glm::vec3 wi = glm::normalize(-directionalLight.direction);   // incoming direction

                glm::vec3 dLightRad = light.directionalLight(directionalLight, hitInfo, w0);

                // Ideal Diffuse
                glm::vec3 idealDiffuse = R / pi;
                float cosTerm = glm::dot(hitInfo.normal, wi);
                cosTerm = glm::max(cosTerm, 0.0f); // may need to combine this with ^

                // IDEAL SPECULAR
                // Fresnel Schlick Approx (Calculate Reflection Based On Incident Angle)
                glm::vec3 F0 = glm::vec3(0.04f);
                if (hitInfo.mat.metallic)
                {
                    F0 = glm::mix(F0, hitInfo.mat.albedo, hitInfo.mat.metallic); // mix the fresnel reflection with the base color based on how metallic object is
                }

                // Ideal Specular BRDF
                glm::vec3 wr;
                glm::vec3 idealSpec = glm::vec3(0.0f); // ideal spec BRDF is 0 by default
                if (wi == wr)                          // only changes at angle of reflection
                {
                    idealSpec = FrsRflct(hitInfo.normal, wr, F0) / glm::dot(hitInfo.normal, wr);
                }

                // ROUGH SPECULAR (Direction Diffuse)

                // Trowbridge Reitz (Normal Distr Function)
                glm::vec3 wh = glm::normalize(0.5f * (w0, wi));       // half way vector
                float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
                float alphaSqr = alpha * alpha;
                float k = (alphaSqr + 1.0f) / 8.0f;
                float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
                float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

                // Rough Specular BRDF
                glm::vec3 roughSpec = (D * G * FrsRflct(hitInfo.normal, wh, w0)) / (4.0f * glm::dot(hitInfo.normal, w0) * glm::dot(hitInfo.normal, wi));

                // glm::vec3 viewDir = glm::normalize(camera.origin - hitInfo.point); // w0
                // // Set Color
                // // glm::vec3 pLightRad = light.pointLight(hitInfo.point, pointLight, hitInfo, viewDir);
                // glm::vec3 dLightColor = light.directionalLight(directionalLight, hitInfo, viewDir);
                // // specCoeff += dLightColor * hitInfo.mat.albedo;
                // // specCoeff += dLightColor;

                // // PBR BRDF CALCULATION
                // float pi = 3.14159265359;
                // glm::vec3 lightDir = glm::normalize(pointLight.origin - hitInfo.point); // wi (incoming direction)
                // lightDir = glm::normalize(-directionalLight.direction);
                // glm::vec3 R = hitInfo.mat.albedo;

                // // Trowbridge Reitz (Normal Distribution Function)
                // glm::vec3 wh = glm::normalize(0.5f * (viewDir + lightDir)); // half-way vector (from Blinn Phong, used to calculate reflection angle)
                // float alpha = glm::max(hitInfo.mat.roughness, 0.01f);       // roughness parameter
                // float alphaSqr = alpha * alpha;
                // float k = (alphaSqr + 1.0f) / 8.0f;                       // remapping of roughness parameter
                // float D = DistrFunc(alphaSqr, hitInfo.normal, wh);        // normal distribution function
                // float G = GeomFunc(k, hitInfo.normal, viewDir, lightDir); // geometry function

                // // Frensal-Schlick (Ideal Specular)
                // glm::vec3 F0 = glm::vec3(0.04f);                             // fresnel reflectance, base reflectivity (holds true for most dielectrics, value from learnopengl.com)
                // F0 = glm::mix(F0, hitInfo.mat.albedo, hitInfo.mat.metallic); // take base reflectivity or surface color based on if metallic
                // glm::vec3 Fr = FrsRflct(wh, viewDir, F0);                    // fresnel term

                // // Ideal Diffuse
                // glm::vec3 idealDiffuse = glm::vec3(0.0f);
                // if (hitInfo.mat.ior < 1.5f)
                // {
                //     idealDiffuse = R / pi; // scattering of light (hemisphere)
                // }

                // // Final BRDF Result
                // glm::vec3 roughSpec = (D * G * Fr) / glm::max((0.001f + 4.0f * (glm::dot(hitInfo.normal, viewDir) * glm::dot(hitInfo.normal, lightDir))), 0.001f);
                // glm::vec3 refrctEnergy = 1.0f - Fr; // energy available to refraction
                // // glm::vec3 fr = refrctEnergy * idealDiffuse + roughSpec;
                // glm::vec3 fr = refrctEnergy * idealDiffuse;
                // // specCoeff += pLightRad;
                // specCoeff += dLightColor * fr;
            }

            color += specCoeff;
            // WHITTED RAY TRACER
            glm::vec3 indirectRad;

            // Metallic Surface
            if (hitInfo.mat.metallic)
            {
                // Reflection Ray
                Ray reflctRay;
                reflctRay.origin = hitInfo.point + (hitInfo.normal * 0.001f);
                reflctRay.direction = glm::reflect(ray.direction, hitInfo.normal);

                glm::vec3 reflctColor = glm::vec3(1.0f) * tracer(reflctRay, depth + 1);

                color += (reflctColor * hitInfo.mat.albedo);
            }

            // Glass Surface
            else if (hitInfo.mat.ior >= 1.5f)
            {
                glm::vec3 attenuation = glm::vec3(1.0f);

                // Reflection Ray
                Ray reflctRay;
                glm::vec3 reflctColor = glm::vec3(0.0f);

                // Refraction Ray
                Ray refrctRay;
                glm::vec3 refrctColor = glm::vec3(0.0f);

                // Direction Through Snell's Law
                float ior; // ratio of refract index (air) / refract index (glass)
                glm::vec3 normal;

                // Outside Object
                if (glm::dot(ray.direction, hitInfo.normal) < 0.0f)
                {
                    ior = 1.0f / hitInfo.mat.ior; // air
                    normal = hitInfo.normal;
                }
                // Inside Object
                else
                {
                    ior = hitInfo.mat.ior;    // glass
                    normal = -hitInfo.normal; // flip normal
                }

                // Snell's Law Variables
                float cosTheta = glm::min(glm::dot(-ray.direction, normal), 1.0f); // cosine angle of incidence (how straight on light is hitting surface)
                float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);            // sine angle of incidence (how much light bends)

                // Total Internal Reflection (100% of Light Reflects)
                if (ior * sinTheta > 1.0f) // only reflect
                {
                    reflctRay.origin = hitInfo.point + (normal * 0.001f);
                    reflctRay.direction = glm::reflect(ray.direction, normal);

                    reflctColor = attenuation * tracer(reflctRay, depth + 1);

                    color += reflctColor;
                }
                else // refract and reflect
                {
                    // Send Refraction Ray
                    refrctRay.origin = hitInfo.point - (normal * 0.001f);
                    refrctRay.direction = glm::refract(ray.direction, normal, ior);
                    glm::vec3 refrctRes = attenuation * tracer(refrctRay, depth + 1);

                    // Send Reflection Ray (Does Both^)
                    reflctRay.origin = hitInfo.point + (normal * 0.001f);
                    reflctRay.direction = glm::reflect(ray.direction, normal);
                    glm::vec3 reflctRes = tracer(reflctRay, depth + 1);

                    // Schlick Approximation For Reflectance Percentage
                    glm::vec3 F0 = glm::vec3(0.04f); // how much light will pass throuh when looking straight at glass
                    glm::vec3 viewDir = -ray.direction;
                    glm::vec3 frsnl = FrsRflct(normal, viewDir, F0); // find percentage of reflection

                    // Implement the Reflectance Percentage
                    reflctColor = reflctRes * frsnl;
                    refrctColor = refrctRes * (glm::vec3(1.0f) - frsnl);

                    // Combine the Reflection and Refraction
                    color += (refrctColor + reflctColor) * attenuation;
                }
            }

            return color;
        }
        else
        {
            // Set Sky Color
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
                Intersect::Sphere newSphere;
                file >> newSphere.center.x >> newSphere.center.y >> newSphere.center.z;
                file >> newSphere.radius;
                // Material Stats
                file >> newSphere.albedo.r >> newSphere.albedo.g >> newSphere.albedo.b;
                file >> newSphere.metallic >> newSphere.roughness >> newSphere.ior >> newSphere.emissive;
                objID += 1;
                newSphere.objID = objID;

                spheres.push_back(newSphere);

                std::cout << "Sphere Position: " << newSphere.center.x << " " << newSphere.center.y << " " << newSphere.center.z << std::endl;
                std::cout << "Radius: " << newSphere.radius << std::endl;
                std::cout << "Metallic: " << newSphere.metallic << " Roughness: " << newSphere.roughness << " IOR: " << newSphere.ior << " Emissive: " << newSphere.emissive << std::endl;
            }
            else if (type == "Triangle")
            {
                Intersect::Triangle newTriangle;
                // Shape Stats
                file >> newTriangle.a.x >> newTriangle.a.y >> newTriangle.a.z;
                file >> newTriangle.b.x >> newTriangle.b.y >> newTriangle.b.z;
                file >> newTriangle.c.x >> newTriangle.c.y >> newTriangle.c.z;
                // Material Stats
                file >> newTriangle.albedo.r >> newTriangle.albedo.g >> newTriangle.albedo.b;
                file >> newTriangle.metallic >> newTriangle.roughness >> newTriangle.ior >> newTriangle.emissive;

                objID += 1;
                newTriangle.objID = objID;
                triangles.push_back(newTriangle);
            }
            else if (type == "Plane")
            {
                Intersect::Plane newPlane;
                // Shape Stats
                file >> newPlane.position.x >> newPlane.position.y >> newPlane.position.z;
                file >> newPlane.normal.x >> newPlane.normal.y >> newPlane.normal.z;
                // Material Stats
                file >> newPlane.albedo.r >> newPlane.albedo.g >> newPlane.albedo.b;
                file >> newPlane.metallic >> newPlane.roughness >> newPlane.ior >> newPlane.emissive;
                objID += 1;
                newPlane.objID = objID;

                planes.push_back(newPlane);
            }
            else if (type == "pLight")
            {
                // Light::pLight newPointLight;
                //  Light Stats
                file >> pointLight.origin.x >> pointLight.origin.y >> pointLight.origin.z;
                file >> pointLight.color.r >> pointLight.color.g >> pointLight.color.b;
            }
            else if (type == "dLight")
            {
                // Light Stats
                file >> directionalLight.direction.x >> directionalLight.direction.y >> directionalLight.direction.z;
                file >> directionalLight.color.r >> directionalLight.color.g >> directionalLight.color.b;
            }
            else if (type == "Camera")
            {
                file >> camera.origin.x >> camera.origin.y >> camera.origin.z;
                file >> camera.up.x >> camera.up.y >> camera.up.z;
                file >> camera.gaze.x >> camera.gaze.y >> camera.gaze.z;
                file >> camera.length;
            }
            else if (type == "Viewport")
            {
                file >> camera.viewport.height >> camera.viewport.width;
            }
        }
        file.close();
    }
};