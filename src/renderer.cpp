#include "renderer.h"
#include <iostream>
#include <fstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "omp.h"

using namespace Intersect;
using namespace Help;

// RENDER
void Renderer::render(float *pixelBuffer, int resWidth, int resHeight, float &sampleCount)
{
    camera.camViewUpdate();

// FRAME BUFFER W/ PARALLELIZATION
// Loop Through Pixel Rows (Preserves i and j pixels)
#pragma omp parallel for collapse(2) schedule(dynamic, 1) // creates threads to run portions of the loop in parallel
    for (int j = 0; j < resHeight; j++)                   // render all pixels
    {
        for (int i = 0; i < resWidth; i++)
        {
            // ANTI-ALIASING (Multiple Rays)
            // Varaibles
            Ray ray;

            glm::vec3 color(0.0f);
            int smpleAmnt = 4; // samples per pixel
            float average = (float)smpleAmnt / (smpleAmnt + sampleCount);

            // Generate Jittered Rays (Jitter Happens Inside Ray)
            for (int index = 0; index < smpleAmnt; index++)
            {
                // Generate Ray
                ray = camera.rayGeneration(i, j);

                // Call Tracer
                glm::vec3 sampleColor = tracer(ray, 0);

                // NaN Check
                if (glm::any(glm::isnan(sampleColor)))
                {
                }
                else
                {
                    color += sampleColor;
                }
            }

            // Average Radiance Calculation (Monte Carlo)
            color = color / float(smpleAmnt);

            int index = (j * resWidth + i) * 3; // multiply by 3 to account for RGB components and resWidth to prevent overwriting pixels

            // PROGRESSIVE RENDERER (Average Total Samples)
            if (sampleCount > 0)
            {
                // color = ((pixelBufferTemp[index] * sampleCount) + color) / (sampleCount + 1.0f);
                pixelBuffer[index] = glm::mix(pixelBuffer[index], color.r, average);         // weighted red
                pixelBuffer[index + 1] = glm::mix(pixelBuffer[index + 1], color.g, average); // weighted green
                pixelBuffer[index + 2] = glm::mix(pixelBuffer[index + 2], color.b, average); // weighted blue
            }
            else
            {
                // Grab RGB Components
                pixelBuffer[index] = color.x;     // grab red value
                pixelBuffer[index + 1] = color.y; // grab green value
                pixelBuffer[index + 2] = color.z; // grab blue value
            }
        }
    }
}

// TRACER
glm::vec3 Renderer::tracer(Ray ray, unsigned int depth)
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
        tempHit = intersectSphere(ray, spheres[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Check For Plane
    for (i = 0; i < planes.size(); i++)
    {
        tempHit = intersectPlane(ray, planes[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Check For Triangle
    for (i = 0; i < triangles.size(); i++)
    {
        tempHit = intersectTriangle(ray, triangles[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Non Light Hit
    if (hitInfo.valid && hitInfo.mat.emissive == false)
    {
        // VARIABLES
        glm::vec3 wiDirect;
        float pdf = 1.0f;
        float lightDist = 1.0f;

        // LIGHT SAMPLING & SHADOWS
        glm::vec3 Le;
        // Le = light.pointLight(pointLight, hitInfo, wiDirect, lightDist);
        // Le = light.directionalLight(directionalLight, hitInfo, wiDirect);

        if (glm::length(areaLight.color) > 0.0f)
        {
            Le = light.areaLight(areaLight, hitInfo, wiDirect, lightDist);
        }

        else if (glm::length(pointLight.color) > 0.0f)
        {
            Le = light.pointLight(pointLight, hitInfo, wiDirect, lightDist);
        }
        else if (glm::length(directionalLight.color) > 0.0f)
        {
            Le = light.directionalLight(directionalLight, hitInfo, wiDirect);
        }

        // Generate Shadows
        bool inShadow = false;
        Ray shadowRay;
        shadowRay.origin = hitInfo.point + (hitInfo.normal * 0.01f); // offset to avoid self shadowing
        shadowRay.direction = wiDirect;

        // Shadow Block Check Sphere
        HitInfo shadowHit;
        for (i = 0; i < spheres.size(); i++)
        {
            shadowHit = intersectSphere(shadowRay, spheres[i], xForms);
            if (shadowHit.valid && shadowHit.distance < lightDist && hitInfo.objID != shadowHit.objID) // Check for hit and behind object
                inShadow = true;
        }

        // Shadow Block Check Plane
        for (i = 0; i < planes.size(); i++)
        {
            shadowHit = intersectPlane(shadowRay, planes[i], xForms);
            if (shadowHit.valid && shadowHit.distance < lightDist && hitInfo.objID != shadowHit.objID)
                inShadow = true;
        }

        // COLOR / REFLECTANCE (BRDF)
        glm::vec3 R = hitInfo.mat.albedo;
        glm::vec3 w0 = -ray.direction;
        glm::vec3 directLight = glm::vec3(0.0f);

        // Not In Shadow
        if (!inShadow)
        {
            // DIRECT LIGHTING
            glm::vec3 rflctDirect = DirectBxDF(hitInfo, w0, wiDirect);

            float nDotWi = glm::dot(hitInfo.normal, wiDirect); // lambert's cos law
            directLight = rflctDirect * nDotWi * Le;
        }

        // INDIRECT LIGHT
        glm::vec3 wiIndirect;
        glm::vec3 rflctIndirect;

        if (hitInfo.mat.z == 0.0f)
        {
            rflctIndirect = BxDF(hitInfo, w0, wiIndirect, pdf);
        }
        else
        {
            rflctIndirect = LayeredBxDF(hitInfo, w0, wiIndirect, pdf);
        }

        Ray bounceRay;
        bounceRay.direction = wiIndirect;
        glm::vec3 normOffset = glm::dot(wiIndirect, hitInfo.normal) < 0.0f ? -hitInfo.normal : hitInfo.normal; // prevent self intersection
        bounceRay.origin = hitInfo.point + (normOffset * 0.002f);

        // Indirect Light Calculation
        glm::vec3 Li = tracer(bounceRay, depth + 1);

        // color += (rflct * pi) * Li;
        float nDotwi = glm::abs(glm::dot(hitInfo.normal, wiIndirect)); // lambert's cos law
        glm::vec3 indirectLight = (rflctIndirect * Li * nDotwi) / pdf;

        // Final Color
        color += indirectLight + directLight;

        return color;
    }
    // Exited Scene
    else
    {
        // Set Sky Color
        return glm::vec3(0.69f, 0.88f, 1.0f);

        // Add Emission From All Infinite Lights
        // return directionalLight.color;
    }
}

// SCENE
void Renderer::loadScene(const std::string &filename)
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
            file >> newSphere.metallic >> newSphere.roughness >> newSphere.ior >> newSphere.emissive >> newSphere.z >> newSphere.layerIOR;
            file >> newSphere.animated;
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
            file >> newTriangle.p0.x >> newTriangle.p0.y >> newTriangle.p0.z;
            file >> newTriangle.p1.x >> newTriangle.p1.y >> newTriangle.p1.z;
            file >> newTriangle.p2.x >> newTriangle.p2.y >> newTriangle.p2.z;
            // Material Stats
            file >> newTriangle.albedo.r >> newTriangle.albedo.g >> newTriangle.albedo.b;
            file >> newTriangle.metallic >> newTriangle.roughness >> newTriangle.ior >> newTriangle.emissive >> newTriangle.z >> newTriangle.layerIOR;

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
            file >> newPlane.metallic >> newPlane.roughness >> newPlane.ior >> newPlane.emissive >> newPlane.z >> newPlane.layerIOR;
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

        else if (type == "aLight")
        {
            // Light Stats
            file >> areaLight.origin.x >> areaLight.origin.y >> areaLight.origin.z;
            file >> areaLight.color.r >> areaLight.color.g >> areaLight.color.b;
            file >> areaLight.normal.x >> areaLight.normal.y >> areaLight.normal.z;
            file >> areaLight.u.x >> areaLight.u.y >> areaLight.u.z;
            file >> areaLight.v.x >> areaLight.v.y >> areaLight.v.z;
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

        else if (type == "Lens")
        {
            file >> camera.lensDiameter >> camera.focusDist;
            std::cout << "Lens diameter: " << camera.lensDiameter << std::endl;
        }

        else if (type == "Transform")
        {
            // Variables
            Intersect::xForm trnsfrm;
            trnsfrm.transform = glm::mat4(1.0f);
            glm::vec4 rot(1.0f);
            glm::vec3 trns(1.0f);
            float scale = 1.0f;

            // Object IDs
            file >> trnsfrm.crntID >> trnsfrm.prntID;
            // Rotation
            file >> rot.x >> rot.y >> rot.z >> rot.w;
            // Translation
            file >> trns.x >> trns.y >> trns.z;
            // Scale
            file >> scale;

            // Transforms
            trnsfrm.transform = glm::translate(trnsfrm.transform, trns);
            trnsfrm.transform = glm::rotate(trnsfrm.transform, glm::radians(rot.w), glm::vec3(rot));
            trnsfrm.transform = glm::scale(trnsfrm.transform, glm::vec3(scale));

            xForms.push_back(trnsfrm);

            std::cout << "Transform for ID " << trnsfrm.crntID << ":\n";
            std::cout << glm::to_string(trnsfrm.transform) << "\n\n";
        }
    }
    file.close();
}