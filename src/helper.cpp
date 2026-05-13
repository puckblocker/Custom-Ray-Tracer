#include "helper.h"
#include "intersection.h"

namespace Help
{
    std::vector<CIE_Table> cieTable;

    // Random Number Generator
    float RandFloat()
    {
        // return (rand() / (RAND_MAX + 1.0f));

        // Generate Seed With Hardware Entropy
        thread_local std::random_device r;

        // Number Range
        std::uniform_real_distribution<float> distr(0, 1);

        // Generate Normal Distribution
        thread_local std::mt19937 gen(r());

        // Generate Random Number
        return distr(gen);
    }

    // Fresnel Reflectance
    glm::vec4 FrsRflct(glm::vec3 normal, glm::vec3 direction, glm::vec4 F0)
    {
        float cosTheta = glm::clamp(glm::dot(normal, direction), 0.0f, 1.0f);
        glm::vec4 Fr = F0 + (1.0f - F0) * glm::pow((1.0f - cosTheta), 5.0f);
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

    // LAYERED BSDF (Random Instead of Deterministic)
    glm::vec4 LayeredBxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 &wi, float &pdf)
    {
        int maxDepth = 5;

        // Top Layer Hit
        HitInfo topLayer = hitInfo;
        topLayer.mat.roughness = 0.0f;
        topLayer.mat.metallic = 0.0f;
        topLayer.mat.ior = hitInfo.mat.layerIOR;
        topLayer.mat.albedo = glm::vec3(1.0f);

        // BxDF CALL
        glm::vec4 f = BxDF(ray, topLayer, w0, wi, pdf); // initial check for if entered layer of not

        // Outside Check
        float cosThetaOut = glm::dot(wi, hitInfo.normal);
        float cosThetaIn = glm::dot(w0, hitInfo.normal);

        // REFLECTED
        // if (cosThetaOut > 0.0f && cosThetaIn > 0.0f)
        // {
        //     return f;
        // }

        // REFLECTED
        if (wi.z > 0.0f && w0.z > 0.0f)
        {
            return f; // return BxDF
        }

        // TRANSMITTED
        glm::vec3 w; // current direction
        w = wi;      // starting direction
        float z = hitInfo.mat.z;

        // RANDOM WALK
        for (int depth = 0; depth < maxDepth; ++depth)
        {
            // Beer's Law Absorbance
            float distance = hitInfo.mat.z / glm::abs(w.z);
            float cosThetaW = glm::dot(w, hitInfo.normal);
            // float distance = hitInfo.mat.z / glm::max(glm::abs(cosThetaW), 0.001f);
            f *= std::exp(-distance);

            // z = (z == hitInfo.mat.z) ? 0.0f : hitInfo.mat.z;

            // Boundary Hit Check
            HitInfo crntHit = hitInfo;

            // Bottom Boundary
            if (z == 0.0f)
            {
                crntHit.mat = hitInfo.mat;
            }
            // Top Boundary
            else
            {
                crntHit.mat = topLayer.mat;
            }

            // Bounce
            float pdfBounce = 0.0f;
            glm::vec4 color = BxDF(ray, crntHit, -w, wi, pdfBounce);
            f *= color;
            pdf *= pdfBounce;
            w = wi;

            cosThetaW = glm::dot(w, hitInfo.normal);

            // Escape Check
            if (cosThetaW < 0.0f && z == hitInfo.mat.z)
            {
                break;
            }
        }

        return glm::vec4(0.0f);
    }

    // DIRECT LIGHTING BRDF
    glm::vec4 DirectBxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 wi)
    {
        // PERFECT SMOOTH CHECK (Dirac Delta)
        if (hitInfo.mat.roughness == 0.0f)
        {
            return glm::vec4(0.0f);
        }

        // VARIABLES
        float pi = 3.14159265359;

        float nDotW0 = glm::dot(hitInfo.normal, w0);
        float nDotWi = glm::dot(hitInfo.normal, wi);

        // Early Exit
        if (nDotW0 <= 0.0f || nDotWi <= 0.0f)
        {
            return glm::vec4(0.0f);
        }

        glm::vec3 wh = glm::normalize(w0 + wi); // half-way vector

        // IDEAL SPECULAR
        // Fresnel Schlick Approx (Calculate Reflection Based On Incident Angle)
        glm::vec4 F0 = glm::vec4(0.04f);
        if (hitInfo.mat.metallic)
        {
            F0 = glm::mix(F0, ray.radiance, hitInfo.mat.metallic); // mix the fresnel reflection with the base color based on how metallic object is
        }

        // ROUGH SPECULAR (Direction Diffuse)

        // Trowbridge Reitz (Normal Distr Function)
        float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
        float alphaSqr = alpha * alpha;
        float k = (alphaSqr + 1.0f) / 8.0f;
        float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
        float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

        // Rough Specular BRDF
        glm::vec4 Fr = FrsRflct(wh, w0, F0);
        glm::vec4 roughSpec = (D * G * Fr) / (4.0f * nDotW0 * nDotWi);

        // Ideal Diffuse
        glm::vec4 kD = glm::vec4(1.0f) - Fr;
        kD *= (1.0f - hitInfo.mat.metallic);
        glm::vec4 idealDiffuse = (kD * ray.radiance) / pi;
        // glm::vec3 idealDiffuse = hitInfo.mat.albedo / pi;

        // PBR BRDF RESULT (Direct Lighting)
        glm::vec4 directLighting = idealDiffuse + roughSpec;

        return directLighting;
    }

    glm::vec4 BxDF(Ray ray, HitInfo hitInfo, glm::vec3 w0, glm::vec3 &wi, float &pdf)
    {
        // Variables
        float ior = hitInfo.mat.ior;
        glm::vec3 normal = hitInfo.normal;
        glm::vec4 albedo = ray.radiance;
        float nDotw0 = dot(hitInfo.normal, w0);
        glm::vec4 Fr;
        glm::vec4 F0 = glm::vec4(0.04f);

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
                float cosTheta;

                // CORRECTED SCHLICK APPROXIMATION
                F0 = glm::vec4(glm::pow((ior - 1) / (ior + 1), 2.0f));

                // Variables
                Fr = FrsRflct(n, w0, F0);
                float uc = RandFloat();
                float pt = 1.0f - Fr.r;

                // DISCRETE SAMPLING
                // Perfect Specular Reflection
                if (uc < (Fr.r / (Fr.r + pt)))
                {
                    wi = glm::reflect(-w0, n);
                    pdf = 1.0f;
                    cosTheta = glm::max(glm::abs(glm::dot(n, wi)), 0.001f);
                    return glm::vec4(1.0f);
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

                    pdf = 1.0f;
                    cosTheta = glm::max(glm::abs(glm::dot(n, wi)), 0.001f);
                    return glm::vec4(1.0f);
                }
            }
            // Smooth Conductor
            else if (hitInfo.mat.metallic)
            {
                // CORRECTED SCHLICK APPROXIMATION
                // F0 = (glm::pow(ior - 1.0f, 2.0f) + (albedo * albedo)) / (glm::pow(ior + 1.0f, 2.0f) + (albedo * albedo));
                F0 = albedo;

                wi = glm::reflect(-w0, normal);

                // BSDF CALCULATION
                // return Fr / glm::abs(nDotw0);
                Fr = FrsRflct(normal, w0, F0);

                pdf = 1.0f;
                float cosTheta = glm::max(glm::abs(glm::dot(normal, wi)), 0.001f);
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
            pdf = wi.z / pi;
            pdf = glm::max(pdf, 0.001f);

            // Convert wi To World Space
            // Axis Calculation
            glm::vec3 normUp = (abs(hitInfo.normal.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
            glm::vec3 orthoU = glm::normalize(glm::cross(normUp, hitInfo.normal));
            glm::vec3 orthoUp = glm::cross(orthoU, hitInfo.normal);

            // Rotation Matrix
            glm::mat3 localSpace = glm::mat3(orthoU, orthoUp, hitInfo.normal);

            // Rotate Into World Space
            wi = glm::normalize(localSpace * wi);

            // Trowbridge Reitz (Normal Distr Function)
            glm::vec3 wh = glm::normalize(0.5f * (w0 + wi));      // half way vector
            float alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
            float alphaSqr = alpha * alpha;
            float k = (alphaSqr + 1.0f) / 8.0f;
            float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
            float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution
            float h;

            // ROUGH CONDUCTOR
            if (hitInfo.mat.metallic)
            {
                // F0 = (glm::pow(ior - 1.0f, 2.0f) + (albedo * albedo)) / (glm::pow(ior + 1.0f, 2.0f) + (albedo * albedo));
                F0 = albedo;

                // GGX SAMPLING
                alpha = glm::max(hitInfo.mat.roughness, 0.01f); // roughness parameter
                alphaSqr = alpha * alpha;

                // Random Point In Uniform Disk
                float phi = 2.0f * pi * Xi0;
                float cosTheta = glm::sqrt((1.0f - Xi1) / (1.0f + (alphaSqr - 1.0f) * Xi1));
                float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));

                wh.x = sinTheta * glm::cos(phi); // random point on x
                wh.y = sinTheta * glm::sin(phi); // random point on y
                wh.z = cosTheta;

                // Local To World Transform
                wh = glm::normalize(orthoU * wh.x + orthoUp * wh.y + hitInfo.normal * wh.z);

                wi = glm::reflect(-w0, wh); // final outgoing vector (bounce direction)

                // Normal Check
                if (glm::dot(wi, hitInfo.normal) <= 0.0f)
                {
                    return glm::vec4(0.0f);
                }

                float k = (alphaSqr + 1.0f) / 8.0f;
                float D = DistrFunc(alphaSqr, hitInfo.normal, wh); // how aligned surface normal is with microfacet normals
                float G = GeomFunc(k, hitInfo.normal, w0, wi);     // how much masking, shadowing, interreflection due to facet distribution

                float nDotw0 = glm::max(dot(hitInfo.normal, w0), 0.001f);
                float nDotwi = glm::max(dot(hitInfo.normal, wi), 0.001f);
                float nDotwh = glm::max(dot(hitInfo.normal, wh), 0.001f);
                float w0Dotwh = glm::max(dot(w0, wh), 0.001f);

                // PDF
                pdf = (D * nDotwh) / (4.0f * w0Dotwh);
                pdf = glm::max(pdf, 0.001f);

                // BRDF Calculations
                glm::vec4 Fr = FrsRflct(wh, w0, F0);
                glm::vec4 roughSpec = (D * G * Fr) / (4.0f * nDotw0 * nDotwi);

                return roughSpec;
            }

            // ROUGH DIELECTRIC
            else if (hitInfo.mat.ior > 1.0f)
            {
                // CORRECTED SCHLICK APPROXIMATION
                F0 = glm::vec4(glm::pow((ior - 1.0f) / (ior + 1.0f), 2.0f));

                // Random Numbers
                float xiPhi = RandFloat();
                float xiTheta = RandFloat();
                float xi0 = RandFloat();

                // GGX SAMPLING
                float alpha = glm::max(hitInfo.mat.roughness, 0.01f);
                float alphaSqr = alpha * alpha;

                // Random Point In Uniform Disk
                float phi = 2.0f * pi * xiPhi;
                float cosTheta = glm::sqrt((1.0f - xiTheta) / (1.0f + (alphaSqr - 1.0f) * xiTheta));
                float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));

                wh.x = sinTheta * glm::cos(phi);
                wh.y = sinTheta * glm::sin(phi);
                wh.z = cosTheta;

                // Local To World Transform
                wh = glm::normalize(orthoU * wh.x + orthoUp * wh.y + hitInfo.normal * wh.z);

                if (glm::dot(wh, w0) < 0.0f)
                {
                    wh = -wh;
                }

                // Inside / Outside Check
                float etaI = 1.0f;
                float etaT = hitInfo.mat.ior;
                bool outside = glm::dot(w0, normal) > 0.0f;
                glm::vec3 n = outside ? normal : -normal;
                float eta = outside ? (etaI / etaT) : (etaT / etaI);

                // Fresnel Reflection
                float Fr = FrsRflct(wh, w0, F0).r;

                // REFLECT CHANCE
                if (xi0 <= Fr)
                {
                    // Reflect
                    wi = glm::reflect(-w0, wh);

                    if (glm::dot(wi, hitInfo.normal) <= 0.0f)
                        return glm::vec4(0.0f);

                    float k = (alphaSqr + 1.0f) / 8.0f;
                    float D_new = DistrFunc(alphaSqr, hitInfo.normal, wh);
                    float G_new = GeomFunc(k, hitInfo.normal, w0, wi);

                    float nDotw0 = glm::max(dot(hitInfo.normal, w0), 0.001f);
                    float nDotwi = glm::max(dot(hitInfo.normal, wi), 0.001f);
                    float nDotwh = glm::max(dot(hitInfo.normal, wh), 0.001f);
                    float w0Dotwh = glm::max(dot(w0, wh), 0.001f);

                    // PDF
                    pdf = Fr * ((D_new * nDotwh) / (4.0f * w0Dotwh));
                    pdf = glm::max(pdf, 0.001f);

                    // Return BRDF
                    return (D_new * G_new * glm::vec4(Fr) * nDotwi) / (4.0f * nDotwi);
                }
                // Transmit (Refract)
                else
                {
                    wi = glm::refract(-w0, wh, eta);

                    // TIR Check
                    if (glm::length(wi) == 0.0f)
                    {
                        return glm::vec4(0.0f);
                    }

                    float k = (alphaSqr + 1.0f) / 8.0f;
                    float D_new = DistrFunc(alphaSqr, hitInfo.normal, wh);
                    float G_new = GeomFunc(k, hitInfo.normal, w0, wi);

                    float nDotw0 = glm::abs(glm::dot(hitInfo.normal, w0));
                    float nDotwi = glm::abs(glm::dot(hitInfo.normal, wi));
                    float nDotwh = glm::abs(glm::dot(hitInfo.normal, wh));

                    // USE ABSOLUTE VALUES FOR MICROFACET CHECKS
                    float w0Dotwh = glm::abs(glm::dot(w0, wh));
                    float wiDotwh = glm::abs(glm::dot(wi, wh));

                    // PDF
                    float denom = glm::pow(glm::dot(wi, wh) + eta * glm::dot(w0, wh), 2.0f);
                    denom = glm::max(denom, 0.0001f); // Safety clamp

                    pdf = (1.0f - Fr) * (D_new * nDotwh * wiDotwh) / denom;
                    pdf = glm::max(pdf, 0.001f);

                    // BTDF
                    float btdf = (D_new * G_new * w0Dotwh * wiDotwh) / (nDotw0 * nDotwi * denom);
                    float nDotwiT = glm::abs(glm::dot(hitInfo.normal, wi));
                    return glm::vec4(btdf) * (1.0f - Fr) * albedo;
                }
            }

            // ROUGH DIFFUSE
            else
            {
                float nDotwi = glm::max(dot(hitInfo.normal, wi), 0.001f);
                float nDotw0 = glm::max(dot(hitInfo.normal, w0), 0.001f);
                glm::vec4 Fr = FrsRflct(wh, w0, F0);
                glm::vec4 roughSpec = (D * G * Fr) / (4.0f * nDotw0 * nDotwi);

                // Diffuse Lobe (Lambertian)
                glm::vec4 kD = (1.0f - Fr) * (1.0f - hitInfo.mat.metallic); // diffuse ratio
                glm::vec4 diffuse = (kD * albedo) / pi;

                // PBR BRDF RESULT (Direct Lighting)
                // return (roughSpec + diffuse) * pi;
                return diffuse;
            }
        }

        // Error Detection
        return glm::vec4(0.0f);
    }

    //
    // RGB TO SPECTRAL
    //

    // HERO SAMPLING
    glm::vec4 RandLambda()
    {
        glm::vec4 lambda;

        // Random Number Generator Setup
        thread_local std::random_device r;                     // generate seed with hardware entropy
        thread_local std::mt19937 gen(r());                    // generate normal distribution
        std::uniform_real_distribution<float> distr(0.0, 1.0); // number range

        // Create Range For Light Spectrum
        float specMin = 380.0;
        float specMax = 780.0;
        float range = specMax - specMin;

        // Generate Hero Sample (Initial Random Number)
        float hero = specMin + (distr(gen) * range);

        // Grab Lambda Values Based On Hero
        for (int i = 0; i < 4; i++)
        {
            float offset = i * (range / 4.0f);
            float val = hero + offset;

            // Check For Bounds
            if (val > specMax)
            {
                val -= range; // warp back to start
            }

            lambda[i] = val;
        }

        return lambda;
    }

    // Spectral Unsampling (Generate Reflectance Curve (Grab Power / Radiance))
    float ReflectanceCurve(glm::vec3 rgbVal, float lambda)
    {
        // Return Color Field Based On Wavelength
        if (lambda < 490.0f)
            return rgbVal.b;
        else if (lambda < 580.0f)
            return rgbVal.g;
        else
            return rgbVal.r;
    }

    // CIE Weights
    CIE_Table getWeights(float lambda)
    {
        // Clamp Lamda (Out of Bounds)
        if (lambda < 380.0f || lambda > 780.0f)
        {
            return {0.0f, 0.0f, 0.0f};
        }

        // Convert To Integer
        int index = static_cast<int>(lambda - 380.0f);

        return cieTable[index];
    }
}