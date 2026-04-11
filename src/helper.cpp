#include "helper.h"

namespace Help
{
    // Random Number Generator
    float RandFloat()
    {
        // return (rand() / (RAND_MAX + 1.0f));

        // Generate Seed With Hardware Entropy
        std::random_device r;

        // Number Range
        std::uniform_real_distribution<float> distr(0, 1);

        // Generate Normal Distribution
        std::mt19937 gen(r());

        // Generate Random Number
        return distr(gen);
    }

    // Fresnel Reflectance
    glm::vec3 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec3 F0)
    {
        float cosTheta = glm::clamp(glm::dot(normal, direction), 0.0f, 1.0f);
        glm::vec3 Fr = F0 + (1.0f - F0) * glm::pow((1.0f - cosTheta), 5.0f);
        return Fr;
    }

    // ROUGH SPECULAR (Trowbridge-Reitz)
    // Normal Distribution Function (Approx relative surface area of microfacets exactly aligned to the halfway vector wh (from learnopengl))
    float DistrFunc(float alphaSqr, glm::vec3 normal, glm::vec3 wh)
    {
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

    glm::vec3 BSDF(HitInfo hitInfo, glm::vec3 w0, glm::vec3 &wi)
    {
        // Variables
        float ior = hitInfo.mat.ior;
        glm::vec3 normal = hitInfo.normal;
        glm::vec3 albedo = hitInfo.mat.albedo;
        float nDotw0 = dot(hitInfo.normal, w0);
        glm::vec3 Fr;
        glm::vec3 F0 = glm::vec3(0.04f);

        // SMOOTH SPECULAR
        if (hitInfo.mat.roughness == 0.0f && (ior > 1.0f || hitInfo.mat.metallic))
        {
            // Smooth Dielectric
            if (ior > 1.0f)
            {
                // Inside / Outside Check
                bool outside = glm::dot(w0, normal) > 0.0f;
                glm::vec3 n = outside ? normal : -normal;
                float eta = outside ? (1.0f / ior) : ior;

                // CORRECTED SCHLICK APPROXIMATION
                F0 = glm::vec3(glm::pow((ior - 1) / (ior + 1), 2.0f));

                // Variables
                Fr = FrsRflct(n, w0, F0);
                float uc = RandFloat();
                float pt = 1.0f - Fr.r;

                // DISCRETE SAMPLING
                // Perfect Specular Reflection
                if (uc < (Fr.r / (Fr.r + pt)))
                {
                    wi = glm::reflect(-w0, n);
                    return glm::vec3(1.0f);
                }
                // Perfect Specular Transmission
                else
                {
                    wi = glm::refract(-w0, n, eta);

                    // Catch Grazing Angle (Total Internal Reflection)
                    if (glm::length(wi) == 0.0f)
                    {
                        wi = glm::reflect(-w0, n);
                    }
                    return glm::vec3(1.0f);
                }
            }
            // Smooth Conductor
            else if (hitInfo.mat.metallic)
            {
                // CORRECTED SCHLICK APPROXIMATION
                F0 = (glm::pow(ior - 1.0f, 2.0f) + (albedo * albedo)) / (glm::pow(ior + 1.0f, 2.0f) + (albedo * albedo));

                wi = glm::reflect(-w0, normal);

                // BSDF CALCULATION
                // return Fr / glm::abs(nDotw0);
                Fr = FrsRflct(normal, w0, F0);
                return Fr;
            }
        }
        // ROUGH SPECULAR
        else
        {
            // COS WEIGHTED UIT HEMISPHERE SAMPLING (IMPORTANCE SAMPLING)
            // Random Variables
            float Xi0 = RandFloat();
            float Xi1 = RandFloat();

            // Uniform Disc Sample
            float dXi0 = 2 * Xi0 - 1.0f;
            float dXi1 = 2 * Xi1 - 1.0f;
            float r;
            float theta;

            // Prevent Division By Zero
            if (dXi0 == 0 && dXi1 == 0)
            {
                r = 0.0f;
                theta = 0.0f;
            }

            else if (glm::abs(dXi0) > glm::abs(dXi1))
            {
                theta = (pi / (4.0f)) * (dXi1 / dXi0);
                r = dXi0;
            }
            else
            {
                theta = (pi / (2.0f)) - (pi / (4.0f)) * (dXi0 / dXi1);
                r = dXi1;
            }

            // Project Point Onto Hemisphere
            wi.x = r * glm::cos(theta);
            wi.y = r * glm::sin(theta);
            wi.z = glm::sqrt(1 - r * r);

            // PDF / PROBABILITY
            // Sampling and Direction Probability (Cosine Weighted)
            float pw = glm::cos(theta) / pi;

            // Convert wi To World Space
            // Axis Calculation
            glm::vec3 normUp = (abs(hitInfo.normal.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
            glm::vec3 orthoU = glm::normalize(glm::cross(normUp, hitInfo.normal));
            glm::vec3 orthoUp = glm::cross(orthoU, hitInfo.normal);

            // Rotation Matrix
            glm::mat3 localSpace = glm::mat3(orthoU, orthoUp, hitInfo.normal);

            // Rotate Into World Space
            wi = glm::normalize(localSpace * wi);

            // CORRECT SCHLICK APPROX
            if (hitInfo.mat.metallic)
            {
                F0 = (glm::pow(ior - 1.0f, 2.0f) + (albedo * albedo)) / (glm::pow(ior + 1.0f, 2.0f) + (albedo * albedo));
            }

            // Trowbridge Reitz (Normal Distr Function)
            glm::vec3 wh = glm::normalize(0.5f * (w0 + wi));      // half way vector
            float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
            float alphaSqr = alpha * alpha;
            float k = (alphaSqr + 1.0f) / 8.0f;
            float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
            float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

            // Rough Specular BRDF
            float nDotwi = dot(hitInfo.normal, wi);
            float nDotw0 = dot(hitInfo.normal, w0);
            glm::vec3 Fr = FrsRflct(normal, w0, F0);
            glm::vec3 roughSpec = (D * G * Fr) / (4.0f * nDotw0 * nDotwi);

            // Diffuse Lobe (Lambertian)
            glm::vec3 kD = (1.0f - Fr) * (1.0f - hitInfo.mat.metallic); // diffuse ratio
            glm::vec3 diffuse = (kD * albedo) / pi;

            // PBR BRDF RESULT (Direct Lighting)
            return (roughSpec + diffuse) * pi;
        }

        // Error Detection
        return glm::vec3(0.0f);
    }
}