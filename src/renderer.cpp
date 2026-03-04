#include "renderer.h"
#include <iostream>
#include <fstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "omp.h"

// HELPER FUNCTIONS
// Fresnel Reflectance
glm::vec3 Renderer::FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0)
{
    glm::vec3 Fr = F0 + (1.0f - F0) * glm::pow((1.0f - glm::dot(normal, direction)), 5.0f);
    return Fr;
}

// ROUGH SPECULAR (Trowbridge-Reitz)
// Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
float Renderer::DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh)
{
    float pi = 3.14159265359;
    float D = alphaSqr / (pi * ((glm::dot(normal, wh) * glm::dot(normal, wh)) * (alphaSqr - 1) + 1));
    return D;
}

// Geometry Function (Approx relative surface area where micro surface details overshadow each other, causing light rays to be covered (from opengl))
float Renderer::GeomFunc(float k, glm::vec3 normal, glm::vec3 w0, glm::vec3 wi)
{
    float w0HalfVec = glm::dot(normal, w0) / (glm::dot(normal, w0) * (1 - k) + k);
    float wiHalfVec = glm::dot(normal, wi) / (glm::dot(normal, wi) * (1 - k) + k);
    float G = glm::max(w0HalfVec, 0.0f) * glm::max(wiHalfVec, 0.0f); // clamped to prevent negative light (light from behind object)
    return G;
}

// PBR BRDF CALCULATIONS (DIRECT LIGHTING)
glm::vec3 Renderer::BRDF(glm::vec3 R, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi)
{
    float pi = 3.14159265359;

    // Ideal Diffuse
    glm::vec3 idealDiffuse = R / pi;

    // IDEAL SPECULAR
    // Fresnel Schlick Approx (Calculate Reflection Based On Incident Angle)
    glm::vec3 F0 = glm::vec3(0.04f);
    if (hitInfo.mat.metallic)
    {
        F0 = glm::mix(F0, hitInfo.mat.albedo, hitInfo.mat.metallic); // mix the fresnel reflection with the base color based on how metallic object is
    }

    // ROUGH SPECULAR (Direction Diffuse)

    // Trowbridge Reitz (Normal Distr Function)
    glm::vec3 wh = glm::normalize(0.5f * (w0 + wi));      // half way vector
    float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
    float alphaSqr = alpha * alpha;
    float k = (alphaSqr + 1.0f) / 8.0f;
    float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
    float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

    // Rough Specular BRDF
    glm::vec3 roughSpec = (D * G * FrsRflct(hitInfo.normal, wh, w0)) / (4.0f * glm::dot(hitInfo.normal, w0) * glm::dot(hitInfo.normal, wi));

    // PBR BRDF RESULT (Direct Lighting)
    glm::vec3 directLighting = idealDiffuse + roughSpec;

    return directLighting;
}

// RENDER
void Renderer::render(float *pixelBuffer, int resWidth, int resHeight)
{
    camera.camViewUpdate();

// FRAME BUFFER W/ PARALLELIZATION
// Loop Through Pixel Rows (Preserves i and j pixels)
#pragma omp parallel for                // creates threads to run portions of the loop in parallel
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
        tempHit = intersect.intersectSphere(ray, spheres[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Check For Plane
    for (i = 0; i < planes.size(); i++)
    {
        tempHit = intersect.intersectPlane(ray, planes[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Check For Triangle
    for (i = 0; i < triangles.size(); i++)
    {
        tempHit = intersect.intersectTriangle(ray, triangles[i], xForms);
        if (tempHit.valid && tempHit.distance < hitInfo.distance)
        {
            hitInfo = tempHit;
        }
    }

    // Generate Light
    if (hitInfo.valid)
    {
        // Shadow Variables
        glm::vec3 toLight; // = pointLight.origin - hitInfo.point;
        float distToLight; // = glm::length(toLight);
        glm::vec3 lightRad;
        bool inShadow = false;
        float pi = 3.14159265359;
        glm::vec3 R = hitInfo.mat.albedo;                             // reflectance
        glm::vec3 w0 = glm::normalize(camera.origin - hitInfo.point); // outgoing direction

        if (glm::length(directionalLight.color) > 0.0f)
        {
            toLight = glm::normalize(-directionalLight.direction);
            distToLight = 1000000.0f;
            lightRad = light.directionalLight(directionalLight, hitInfo, w0);
        }
        else if (glm::length(pointLight.color) > 0.0f)
        {
            toLight = pointLight.origin - hitInfo.point;
            distToLight = glm::length(toLight);
            lightRad = light.pointLight(hitInfo.point, pointLight, hitInfo, w0);
        }

        glm::vec3 wi = glm::normalize(toLight); // incoming direction

        // Generate Shadows
        Ray shadowRay;
        shadowRay.origin = hitInfo.point + (hitInfo.normal * 0.01f); // offset to avoid self shadowing
        shadowRay.direction = glm::normalize(toLight);

        // Shadow Block Check
        // Sphere
        HitInfo shadowHit;
        for (i = 0; i < spheres.size(); i++)
        {
            shadowHit = intersect.intersectSphere(shadowRay, spheres[i], xForms);
            if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID) // Check for hit and behind object
                inShadow = true;
        }

        // Triangle
        for (i = 0; i < triangles.size(); i++)
        {
            shadowHit = intersect.intersectTriangle(shadowRay, triangles[i], xForms);
            if (shadowHit.valid && shadowHit.distance < distToLight && hitInfo.objID != shadowHit.objID)
                inShadow = true;
        }

        // Plane
        for (i = 0; i < planes.size(); i++)
        {
            shadowHit = intersect.intersectPlane(shadowRay, planes[i], xForms);
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
            // PBR BRDF Calculations (DIRECT LIGHTING)
            glm::vec3 directLighting = BRDF(R, hitInfo, w0, wi);

            // Add To Color
            specCoeff = directLighting * lightRad;
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
            file >> newTriangle.p0.x >> newTriangle.p0.y >> newTriangle.p0.z;
            file >> newTriangle.p1.x >> newTriangle.p1.y >> newTriangle.p1.z;
            file >> newTriangle.p2.x >> newTriangle.p2.y >> newTriangle.p2.z;
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