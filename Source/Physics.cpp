/*
  ==============================================================================
    Physics.cpp
    Elements - Fresnel calculations and spectral interactions
    Ported from Python prototype
  ==============================================================================
*/

#include "Physics.h"
#include <cmath>
#include <algorithm>

// ==============================================================================
// STATIC DATA - Materials
// ==============================================================================

/**
 * Los 8 materiales del prototipo Python.
 *
 * 'static' significa que esta variable solo es visible en este archivo.
 * Se inicializa una sola vez cuando el programa carga.
 */
static std::array<Material, NUM_MATERIALS> s_materials = {{
    // Diamond - Near-perfect uniform transmission → ALL harmonics → crystalline
    Material(
        "Diamond",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.96f, 0.97f, 0.98f, 0.98f, 0.97f, 0.97f, 0.96f, 0.95f }},
        "#E8F4FF", 2.42f
    ),
    // Water - High transmission with slight red falloff
    Material(
        "Water",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.95f, 0.97f, 0.98f, 0.96f, 0.92f, 0.85f, 0.75f, 0.60f }},
        "#50C8E8", 1.33f
    ),
    // Amber - Low blue, high red → warm bass tones
    Material(
        "Amber",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.20f, 0.30f, 0.45f, 0.60f, 0.75f, 0.85f, 0.90f, 0.92f }},
        "#FFBF00", 1.55f
    ),
    // Ruby - Very low blue/green, very high red → deep bass
    Material(
        "Ruby",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.10f, 0.10f, 0.12f, 0.15f, 0.45f, 0.80f, 0.92f, 0.95f }},
        "#E0115F", 1.77f
    ),
    // Gold - Absorbs blue, transmits red → LOW harmonics → warm, mellow
    Material(
        "Gold",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.15f, 0.20f, 0.35f, 0.55f, 0.78f, 0.90f, 0.95f, 0.97f }},
        "#FFD700", 0.47f
    ),
    // Emerald - Bell curve on green → harmonics 4-8 → nasal, vocal
    Material(
        "Emerald",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.15f, 0.40f, 0.75f, 0.92f, 0.75f, 0.40f, 0.18f, 0.12f }},
        "#50C878", 1.57f
    ),
    // Amethyst - High violet/blue, blocks red → harmonics 9-20 → brilliant
    Material(
        "Amethyst",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.92f, 0.85f, 0.70f, 0.45f, 0.25f, 0.15f, 0.10f, 0.08f }},
        "#9966CC", 1.54f
    ),
    // Sapphire - Transmits blue, blocks red → HIGH harmonics → cold
    Material(
        "Sapphire",
        {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
        {{ 0.92f, 0.88f, 0.75f, 0.50f, 0.25f, 0.12f, 0.08f, 0.05f }},
        "#0F52BA", 1.77f
    )
}};

// ==============================================================================
// STATIC DATA - Light Sources
// ==============================================================================

/**
 * Helper para generar distribución Gaussiana.
 *
 * En Python usabas: 0.3 + 0.7 * np.exp(-((w - 650)**2) / (2 * 80**2))
 * En C++ lo hacemos igual pero con std::exp().
 */
static float gaussianIntensity(float wavelength, float center, float sigma, float base, float peak)
{
    float diff = wavelength - center;
    return base + peak * std::exp(-(diff * diff) / (2.0f * sigma * sigma));
}

/**
 * Genera el array de wavelengths estándar (380-780nm, 50 puntos).
 */
std::array<float, NUM_WAVELENGTHS> generateWavelengths()
{
    std::array<float, NUM_WAVELENGTHS> wavelengths;
    float step = (WAVELENGTH_MAX - WAVELENGTH_MIN) / (NUM_WAVELENGTHS - 1);
    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
    {
        wavelengths[i] = WAVELENGTH_MIN + i * step;
    }
    return wavelengths;
}

/**
 * Inicializa las fuentes de luz.
 *
 * Esta función se llama una vez para crear los datos estáticos.
 * Usamos una función porque necesitamos calcular las intensidades.
 */
static std::array<LightSource, NUM_LIGHT_SOURCES> createLightSources()
{
    auto wavelengths = generateWavelengths();
    std::array<LightSource, NUM_LIGHT_SOURCES> sources;

    // Sunset - Peak at 650nm (warm orange-red)
    {
        std::array<float, NUM_WAVELENGTHS> intensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        {
            intensity[i] = gaussianIntensity(wavelengths[i], 650.0f, 80.0f, 0.3f, 0.7f);
        }
        sources[0] = LightSource("Sunset", wavelengths, intensity, "#FF6B35");
    }

    // Daylight - Peak at 550nm (neutral white-yellow)
    {
        std::array<float, NUM_WAVELENGTHS> intensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        {
            intensity[i] = gaussianIntensity(wavelengths[i], 550.0f, 120.0f, 0.5f, 0.5f);
        }
        sources[1] = LightSource("Daylight", wavelengths, intensity, "#FFD93D");
    }

    // LED Cool - Peak at 470nm (cool blue-white)
    {
        std::array<float, NUM_WAVELENGTHS> intensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        {
            intensity[i] = gaussianIntensity(wavelengths[i], 470.0f, 90.0f, 0.4f, 0.6f);
        }
        sources[2] = LightSource("LED Cool", wavelengths, intensity, "#6BCF7F");
    }

    return sources;
}

static std::array<LightSource, NUM_LIGHT_SOURCES> s_lightSources = createLightSources();

// ==============================================================================
// STATIC DATA - Light Positions (3-Point Lighting)
// ==============================================================================

static std::array<LightPosition, 3> s_lightPositions = {{
    // Key Light - Front-right, above (primary)
    LightPosition("Key Light", Vec3(0.5f, 0.7f, 0.5f), 1.0f),
    // Fill Light - Left side, slightly above (secondary, softer)
    LightPosition("Fill Light", Vec3(-0.6f, 0.3f, 0.4f), 0.5f),
    // Rim Light - Behind and slightly above (back light for edge definition)
    LightPosition("Rim Light", Vec3(0.0f, 0.2f, -0.8f), 0.7f)
}};

// ==============================================================================
// DATA ACCESS FUNCTIONS
// ==============================================================================

/**
 * Usamos "Meyer's Singleton" pattern para evitar el
 * "static initialization order fiasco".
 *
 * Las variables static locales se inicializan la primera
 * vez que se llama a la función, garantizando orden correcto.
 */
const std::array<Material, NUM_MATERIALS>& getMaterials()
{
    // Static local - inicializado en primera llamada (thread-safe en C++11+)
    static std::array<Material, NUM_MATERIALS> materials = {{
        Material("Diamond",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.96f, 0.97f, 0.98f, 0.98f, 0.97f, 0.97f, 0.96f, 0.95f }},
                 "#E8F4FF", 2.42f),
        Material("Water",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.95f, 0.97f, 0.98f, 0.96f, 0.92f, 0.85f, 0.75f, 0.60f }},
                 "#50C8E8", 1.33f),
        Material("Amber",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.20f, 0.30f, 0.45f, 0.60f, 0.75f, 0.85f, 0.90f, 0.92f }},
                 "#FFBF00", 1.55f),
        Material("Ruby",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.10f, 0.10f, 0.12f, 0.15f, 0.45f, 0.80f, 0.92f, 0.95f }},
                 "#E0115F", 1.77f),
        Material("Gold",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.15f, 0.20f, 0.35f, 0.55f, 0.78f, 0.90f, 0.95f, 0.97f }},
                 "#FFD700", 0.47f),
        Material("Emerald",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.15f, 0.40f, 0.75f, 0.92f, 0.75f, 0.40f, 0.18f, 0.12f }},
                 "#50C878", 1.57f),
        Material("Amethyst",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.92f, 0.85f, 0.70f, 0.45f, 0.25f, 0.15f, 0.10f, 0.08f }},
                 "#9966CC", 1.54f),
        Material("Sapphire",
                 {{ 380.0f, 450.0f, 500.0f, 550.0f, 600.0f, 650.0f, 700.0f, 780.0f }},
                 {{ 0.92f, 0.88f, 0.75f, 0.50f, 0.25f, 0.12f, 0.08f, 0.05f }},
                 "#0F52BA", 1.77f)
    }};
    return materials;
}

const std::array<LightSource, NUM_LIGHT_SOURCES>& getLightSources()
{
    static std::array<LightSource, NUM_LIGHT_SOURCES> sources = []() {
        auto wavelengths = generateWavelengths();
        std::array<LightSource, NUM_LIGHT_SOURCES> s;

        // Sunset
        std::array<float, NUM_WAVELENGTHS> sunsetIntensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
            sunsetIntensity[i] = gaussianIntensity(wavelengths[i], 650.0f, 80.0f, 0.3f, 0.7f);
        s[0] = LightSource("Sunset", wavelengths, sunsetIntensity, "#FF6B35");

        // Daylight
        std::array<float, NUM_WAVELENGTHS> daylightIntensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
            daylightIntensity[i] = gaussianIntensity(wavelengths[i], 550.0f, 120.0f, 0.5f, 0.5f);
        s[1] = LightSource("Daylight", wavelengths, daylightIntensity, "#FFD93D");

        // LED Cool
        std::array<float, NUM_WAVELENGTHS> ledIntensity;
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
            ledIntensity[i] = gaussianIntensity(wavelengths[i], 470.0f, 90.0f, 0.4f, 0.6f);
        s[2] = LightSource("LED Cool", wavelengths, ledIntensity, "#6BCF7F");

        return s;
    }();
    return sources;
}

const LightPosition& getLightPosition(int index)
{
    static std::array<LightPosition, 3> positions = {{
        LightPosition("Key Light", Vec3(0.5f, 0.7f, 0.5f), 1.0f),
        LightPosition("Fill Light", Vec3(-0.6f, 0.3f, 0.4f), 0.5f),
        LightPosition("Rim Light", Vec3(0.0f, 0.2f, -0.8f), 0.7f)
    }};
    index = std::max(0, std::min(index, 2));
    return positions[static_cast<size_t>(index)];
}

// ==============================================================================
// ROTATION & LIGHT ANGLE
// ==============================================================================

/**
 * Aplica rotación 3D a un vector usando matrices de rotación.
 *
 * En Python usabas matrices numpy. En C++ lo hacemos manualmente
 * para evitar dependencias y mantener el código ligero.
 * Orden de rotación: X -> Y -> Z (igual que en Python).
 */
Vec3 applyRotation(const Vec3& v, const Rotation3D& rotation)
{
    // Convert to radians
    float rx = degToRad(rotation.x);
    float ry = degToRad(rotation.y);
    float rz = degToRad(rotation.z);

    // Precompute sin/cos
    float cx = std::cos(rx), sx = std::sin(rx);
    float cy = std::cos(ry), sy = std::sin(ry);
    float cz = std::cos(rz), sz = std::sin(rz);

    // Apply X rotation
    float y1 = cx * v.y - sx * v.z;
    float z1 = sx * v.y + cx * v.z;

    // Apply Y rotation
    float x2 = cy * v.x + sy * z1;
    float z2 = -sy * v.x + cy * z1;

    // Apply Z rotation
    float x3 = cz * x2 - sz * y1;
    float y3 = sz * x2 + cz * y1;

    return Vec3(x3, y3, z2);
}

float calculateLightAngle(const Vec3& lightPosition, const Rotation3D& objectRotation)
{
    // DEPRECATED: This function uses Euler angles which have gimbal lock.
    // Use calculateLightAngleFromMatrix instead.
    Vec3 normal(0.0f, 0.0f, 1.0f);
    Vec3 rotatedNormal = applyRotation(normal, objectRotation);
    float cosAngle = clamp(lightPosition.dot(rotatedNormal), -1.0f, 1.0f);
    float angleRad = std::acos(cosAngle);
    return angleRad * 180.0f / 3.14159265359f;
}

float calculateLightAngleFromMatrix(const Vec3& lightPosition, const RotationMatrix& rotMatrix)
{
    // Default implementation for cube - finds best facing face
    // For other geometries, use calculateLightAngleForGeometryFromMatrix

    static const Vec3 faceNormals[6] = {
        Vec3( 0.0f,  0.0f,  1.0f),  // +Z (front)
        Vec3( 0.0f,  0.0f, -1.0f),  // -Z (back)
        Vec3( 1.0f,  0.0f,  0.0f),  // +X (right)
        Vec3(-1.0f,  0.0f,  0.0f),  // -X (left)
        Vec3( 0.0f,  1.0f,  0.0f),  // +Y (top)
        Vec3( 0.0f, -1.0f,  0.0f)   // -Y (bottom)
    };

    float bestAngle = 90.0f;  // Default to grazing angle if no face sees light

    for (int i = 0; i < 6; ++i)
    {
        Vec3 rotatedNormal = rotMatrix.apply(faceNormals[i]);
        float cosAngle = lightPosition.dot(rotatedNormal);

        // Only consider faces that SEE the light (cosAngle > 0 means angle < 90°)
        if (cosAngle > 0.0f)
        {
            float angleRad = std::acos(clamp(cosAngle, -1.0f, 1.0f));
            float angleDeg = angleRad * 180.0f / 3.14159265359f;

            if (angleDeg < bestAngle)
            {
                bestAngle = angleDeg;
            }
        }
    }

    return bestAngle;
}

float calculateLightAngleForGeometryFromMatrix(const Vec3& lightPosition,
                                                const RotationMatrix& rotMatrix,
                                                Geometry geometry)
{
    // ==========================================================================
    // GEOMETRY-SPECIFIC ANGLE CALCULATION
    // ==========================================================================

    if (geometry == Geometry::Sphere)
    {
        // SPHERE: Rotation doesn't change light interaction!
        // A sphere always presents the same curved surface to the light.
        // The point facing the light is always at angle ≈ 0°.
        // We return a small constant angle representing the average response.
        // The timbre comes entirely from the MATERIAL, not rotation.
        return 10.0f;
    }

    if (geometry == Geometry::Torus)
    {
        // TORUS: Has partial rotational symmetry.
        // Sample multiple points for smoother transitions than cube.
        // Use 12 sample normals (more than cube's 6) for smoother response.

        static const Vec3 torusNormals[12] = {
            // Cardinal directions
            Vec3( 0.0f,  0.0f,  1.0f),  // +Z
            Vec3( 0.0f,  0.0f, -1.0f),  // -Z
            Vec3( 1.0f,  0.0f,  0.0f),  // +X
            Vec3(-1.0f,  0.0f,  0.0f),  // -X
            Vec3( 0.0f,  1.0f,  0.0f),  // +Y
            Vec3( 0.0f, -1.0f,  0.0f),  // -Y
            // Diagonal directions (45° between cardinals)
            Vec3( 0.707f,  0.0f,  0.707f),   // +X+Z
            Vec3(-0.707f,  0.0f,  0.707f),   // -X+Z
            Vec3( 0.707f,  0.0f, -0.707f),   // +X-Z
            Vec3(-0.707f,  0.0f, -0.707f),   // -X-Z
            Vec3( 0.0f,  0.707f,  0.707f),   // +Y+Z
            Vec3( 0.0f, -0.707f,  0.707f)    // -Y+Z
        };

        float totalAngle = 0.0f;
        float totalWeight = 0.0f;

        for (int i = 0; i < 12; ++i)
        {
            Vec3 rotatedNormal = rotMatrix.apply(torusNormals[i]);
            float cosAngle = lightPosition.dot(rotatedNormal);

            // Only faces that see the light (angle < 90°)
            if (cosAngle > 0.0f)
            {
                float angleRad = std::acos(clamp(cosAngle, -1.0f, 1.0f));
                float angleDeg = angleRad * 180.0f / 3.14159265359f;

                // Weight by how directly the face sees the light
                float weight = cosAngle * cosAngle;  // Square for more contrast
                totalAngle += angleDeg * weight;
                totalWeight += weight;
            }
        }

        if (totalWeight > 0.0f)
        {
            return totalAngle / totalWeight;
        }
        return 85.0f;  // Almost grazing if no face sees light well
    }

    if (geometry == Geometry::Dodecahedron)
    {
        // DODECAHEDRON: 12 pentagonal faces with uniformly distributed normals.
        // More faces than cube = smoother transitions, richer spectral movement.
        static const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;  // Golden ratio ≈ 1.618
        static const float invPhi = 1.0f / phi;
        // 12 face normals of a regular dodecahedron (normalized)
        static const Vec3 dodecaNormals[12] = {
            Vec3( 0.0f,  1.0f,  phi).normalized(),
            Vec3( 0.0f, -1.0f,  phi).normalized(),
            Vec3( 0.0f,  1.0f, -phi).normalized(),
            Vec3( 0.0f, -1.0f, -phi).normalized(),
            Vec3( 1.0f,  phi,  0.0f).normalized(),
            Vec3(-1.0f,  phi,  0.0f).normalized(),
            Vec3( 1.0f, -phi,  0.0f).normalized(),
            Vec3(-1.0f, -phi,  0.0f).normalized(),
            Vec3( phi,  0.0f,  1.0f).normalized(),
            Vec3(-phi,  0.0f,  1.0f).normalized(),
            Vec3( phi,  0.0f, -1.0f).normalized(),
            Vec3(-phi,  0.0f, -1.0f).normalized()
        };

        float totalAngle = 0.0f;
        float totalWeight = 0.0f;

        for (int i = 0; i < 12; ++i)
        {
            Vec3 rotatedNormal = rotMatrix.apply(dodecaNormals[i]);
            float cosAngle = lightPosition.dot(rotatedNormal);

            if (cosAngle > 0.0f)
            {
                float angleRad = std::acos(clamp(cosAngle, -1.0f, 1.0f));
                float angleDeg = angleRad * 180.0f / 3.14159265359f;

                float weight = cosAngle * cosAngle;
                totalAngle += angleDeg * weight;
                totalWeight += weight;
            }
        }

        if (totalWeight > 0.0f)
            return totalAngle / totalWeight;
        return 85.0f;
    }

    // CUBE: Discrete faces, jumpy transitions (physically correct)
    // Find the face that best sees the light and use its angle.
    return calculateLightAngleFromMatrix(lightPosition, rotMatrix);
}

float calculateLightAngleForGeometry(const Vec3& lightPosition,
                                      const Rotation3D& objectRotation,
                                      Geometry geometry)
{
    // For cube: single front-facing normal
    if (geometry == Geometry::Cube)
    {
        return calculateLightAngle(lightPosition, objectRotation);
    }

    // For sphere and torus: sample multiple normals to capture Z rotation
    // We use 6 face normals (like a cube's faces) and weight-average them
    static const Vec3 sampleNormals[6] = {
        Vec3( 0.0f,  0.0f,  1.0f),  // Front  (+Z)
        Vec3( 0.0f,  0.0f, -1.0f),  // Back   (-Z)
        Vec3( 1.0f,  0.0f,  0.0f),  // Right  (+X)
        Vec3(-1.0f,  0.0f,  0.0f),  // Left   (-X)
        Vec3( 0.0f,  1.0f,  0.0f),  // Top    (+Y)
        Vec3( 0.0f, -1.0f,  0.0f)   // Bottom (-Y)
    };

    float totalAngle = 0.0f;
    float totalWeight = 0.0f;

    for (int i = 0; i < 6; ++i)
    {
        Vec3 rotatedNormal = applyRotation(sampleNormals[i], objectRotation);
        float cosAngle = lightPosition.dot(rotatedNormal);
        cosAngle = clamp(cosAngle, -1.0f, 1.0f);

        // Weight by how much this face "sees" the light (positive cos = facing light)
        // Only consider faces that face toward the light
        if (cosAngle > 0.0f)
        {
            float angleRad = std::acos(cosAngle);
            float angleDeg = angleRad * 180.0f / 3.14159265359f;

            // Weight by cosAngle (faces more directly facing light contribute more)
            float weight = cosAngle;

            // For torus, give more weight to side faces (X axis) to capture Z rotation
            if (geometry == Geometry::Torus && (i == 2 || i == 3))
            {
                weight *= 1.5f;
            }

            totalAngle += angleDeg * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0f)
    {
        return totalAngle / totalWeight;
    }

    // All faces facing away from light → return 90° (grazing angle)
    return 90.0f;
}

// ==============================================================================
// FRESNEL CALCULATIONS
// ==============================================================================

float calculateFresnelFactor(float angleDeg, float refractiveIndex)
{
    // Beyond 90° we're looking at the backside - minimal transmission
    if (angleDeg >= 90.0f)
        return 0.01f;

    float angleRad = degToRad(angleDeg);
    float cosI = std::cos(angleRad);
    float sinI = std::sin(angleRad);

    float sinT = sinI / refractiveIndex;
    if (sinT >= 1.0f)
    {
        // Schlick fallback for metals/extreme IOR where exact equations hit TIR
        float F0 = ((1.0f - refractiveIndex) / (1.0f + refractiveIndex));
        F0 = F0 * F0;
        float reflectance = F0 + (1.0f - F0) * std::pow(1.0f - cosI, 5.0f);
        return clamp(1.0f - reflectance, 0.0f, 1.0f);
    }

    float cosT = std::sqrt(1.0f - sinT * sinT);

    // Fresnel equations for s and p polarization
    float rs = (cosI - refractiveIndex * cosT) / (cosI + refractiveIndex * cosT);
    float rp = (refractiveIndex * cosI - cosT) / (refractiveIndex * cosI + cosT);

    float reflectance = 0.5f * (rs * rs + rp * rp);
    return clamp(1.0f - reflectance, 0.0f, 1.0f);
}

void calculateFresnelSpectral(float angleDeg,
                              const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                              std::array<float, NUM_WAVELENGTHS>& output,
                              float baseIndex)
{
    // Beyond 90° we're looking at the backside of the object - minimal transmission
    if (angleDeg >= 90.0f)
    {
        // Return very low transmission (almost silence) for backside
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        {
            output[i] = 0.02f;
        }
        return;
    }

    // Calculate base Fresnel transmission (uniform)
    float baseFresnel;
    float angleRad = degToRad(angleDeg);
    float cosI = std::cos(angleRad);
    float sinI = std::sin(angleRad);
    float sinT = sinI / baseIndex;

    if (sinT >= 1.0f)
    {
        // Schlick fallback for metals/extreme IOR where exact equations hit TIR
        float F0 = ((1.0f - baseIndex) / (1.0f + baseIndex));
        F0 = F0 * F0;
        float reflectance = F0 + (1.0f - F0) * std::pow(1.0f - cosI, 5.0f);
        baseFresnel = clamp(1.0f - reflectance, 0.0f, 1.0f);
    }
    else
    {
        float cosT = std::sqrt(1.0f - sinT * sinT);
        float rs = (cosI - baseIndex * cosT) / (cosI + baseIndex * cosT);
        float rp = (baseIndex * cosI - cosT) / (baseIndex * cosI + cosT);
        float reflectance = 0.5f * (rs * rs + rp * rp);
        baseFresnel = 1.0f - reflectance;
    }

    // ==========================================================================
    // SPECTRAL SHAPING based on angle
    //
    // Per-wavelength exponential decay: blue wavelengths (high harmonics)
    // decay MUCH faster with angle than red wavelengths (low harmonics).
    //
    // At 0° (direct):  full spectrum, bright, all harmonics
    // At 30° (mild):   highs noticeably reduced, warmer character
    // At 45° (mid):    dramatic blue/red ratio (8x), clear timbre shift
    // At 70°+ (grazing): mostly lows, dark/muffled, very different timbre
    //
    // Volume variation is handled separately by spectralAmplitudeTarget
    // in SynthEngine, NOT here — keeps timbre and volume decoupled.
    // ==========================================================================

    float angleFactor = clamp(angleDeg / 90.0f, 0.0f, 1.0f);

    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
    {
        // Wavelength position: 0 at blue/violet (380nm), 1 at red (780nm)
        float wlPos = (wavelengths[i] - 380.0f) / (780.0f - 380.0f);

        // Per-wavelength exponential decay rate:
        //   Blue (wlPos=0): decayRate = 4.0 → drops fast with angle
        //   Red  (wlPos=1): decayRate = 1.0 → resists angle changes
        // This creates strong spectral SHAPE changes at moderate angles
        float decayRate = 1.0f + 3.0f * (1.0f - wlPos);
        float spectralMod = std::pow(1.0f - angleFactor, decayRate);

        // Final transmission: Fresnel base * spectral shaping (NO amplitude factor)
        float transmission = baseFresnel * spectralMod;

        output[i] = clamp(transmission, 0.0f, 1.0f);
    }
}

// ==============================================================================
// GEOMETRY-SPECIFIC FRESNEL
// ==============================================================================

void calculateFresnelCube(float angleDeg,
                          const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                          std::array<float, NUM_WAVELENGTHS>& output,
                          float baseIndex)
{
    // Cube: single flat face, direct Fresnel
    calculateFresnelSpectral(angleDeg, wavelengths, output, baseIndex);
}

void calculateFresnelSphere(float angleDeg,
                            const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                            std::array<float, NUM_WAVELENGTHS>& output,
                            float baseIndex,
                            int numSamples)
{
    // Initialize output to zero
    std::fill(output.begin(), output.end(), 0.0f);

    // Temporary array for each sample
    std::array<float, NUM_WAVELENGTHS> sampleFresnel;

    float totalWeight = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Sample angle across hemisphere (0 to 89 degrees)
        float sampleAngle = 89.0f * i / (numSamples - 1);

        // Weight: sin(θ)·cos(θ) for projected area
        float thetaRad = degToRad(sampleAngle);
        float weight = std::sin(thetaRad) * std::cos(thetaRad);

        // Rim boost for grazing angles
        float rimBoost = 1.0f + 2.0f * std::pow(sampleAngle / 90.0f, 3.0f);
        weight *= rimBoost;

        // Effective angle offset by base rotation
        float effectiveAngle = clamp(sampleAngle + angleDeg * 0.5f, 0.0f, 89.0f);

        // Calculate Fresnel at this angle
        calculateFresnelSpectral(effectiveAngle, wavelengths, sampleFresnel, baseIndex);

        // Accumulate weighted contribution
        for (int j = 0; j < NUM_WAVELENGTHS; ++j)
        {
            output[j] += weight * sampleFresnel[j];
        }
        totalWeight += weight;
    }

    // Normalize
    if (totalWeight > 0.0f)
    {
        for (int j = 0; j < NUM_WAVELENGTHS; ++j)
        {
            output[j] = clamp(output[j] / totalWeight, 0.0f, 1.0f);
        }
    }
}

void calculateFresnelTorus(float angleDeg,
                           const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                           std::array<float, NUM_WAVELENGTHS>& output,
                           float baseIndex,
                           int numSamples)
{
    // Initialize output to zero
    std::fill(output.begin(), output.end(), 0.0f);

    constexpr float R = 0.4f;   // Major radius
    constexpr float r = 0.15f;  // Minor radius
    constexpr float PI = 3.14159265359f;

    // Pre-compute caustic boost for concave regions
    std::array<float, NUM_WAVELENGTHS> causticBoost;
    for (int j = 0; j < NUM_WAVELENGTHS; ++j)
    {
        float wlPos = (wavelengths[j] - 380.0f) / (780.0f - 380.0f);
        float diff = wlPos - 0.5f;
        causticBoost[j] = 1.0f + 0.3f * std::exp(-(diff * diff) / 0.08f);
    }

    // Temporary array for Fresnel calculations
    std::array<float, NUM_WAVELENGTHS> sampleFresnel;

    float totalWeight = 0.0f;

    // Sample over torus surface
    int poloidalSamples = numSamples;
    int toroidalSamples = numSamples / 2;

    for (int pi = 0; pi < poloidalSamples; ++pi)
    {
        float theta = 2.0f * PI * pi / poloidalSamples;  // Poloidal angle
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (int ti = 0; ti < toroidalSamples; ++ti)
        {
            float phi = PI * ti / toroidalSamples;  // Toroidal angle (0 to PI)
            float sinPhi = std::sin(phi);

            // Surface normal z-component determines local angle
            float nz = sinTheta;

            // Local angle of incidence
            float localAngle = std::abs(std::acos(clamp(nz, -1.0f, 1.0f))) * 180.0f / PI;
            float effectiveAngle = clamp(localAngle + angleDeg * 0.4f, 0.0f, 89.0f);

            // Area weight based on torus geometry
            float areaWeight = std::abs(R + r * cosTheta) * std::abs(sinPhi + 0.5f);

            // Classify surface region
            bool isOuter = cosTheta >= 0.0f;
            bool isConcave = cosTheta < -0.3f;

            if (isOuter)
            {
                // Outer convex surface
                calculateFresnelSpectral(effectiveAngle, wavelengths, sampleFresnel, baseIndex);

                for (int j = 0; j < NUM_WAVELENGTHS; ++j)
                {
                    output[j] += areaWeight * sampleFresnel[j];
                }
                totalWeight += areaWeight;
            }
            else if (isConcave)
            {
                // Inner concave surface (caustics)
                float concaveAngle = clamp(effectiveAngle * 0.5f, 0.0f, 89.0f);
                calculateFresnelSpectral(concaveAngle, wavelengths, sampleFresnel, baseIndex);

                float w = areaWeight * 0.6f;
                for (int j = 0; j < NUM_WAVELENGTHS; ++j)
                {
                    output[j] += w * clamp(sampleFresnel[j] * causticBoost[j], 0.0f, 1.0f);
                }
                totalWeight += w;
            }
        }
    }

    // Normalize
    if (totalWeight > 0.0f)
    {
        for (int j = 0; j < NUM_WAVELENGTHS; ++j)
        {
            output[j] = clamp(output[j] / totalWeight, 0.0f, 1.0f);
        }
    }
}

void calculateGeometryFresnel(Geometry geometry,
                              float angleDeg,
                              const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                              std::array<float, NUM_WAVELENGTHS>& output,
                              float baseIndex)
{
    switch (geometry)
    {
        case Geometry::Sphere:
            calculateFresnelSphere(angleDeg, wavelengths, output, baseIndex);
            break;
        case Geometry::Torus:
            calculateFresnelTorus(angleDeg, wavelengths, output, baseIndex);
            break;
        case Geometry::Dodecahedron:
            // Dodecahedron: flat faces like cube, direct Fresnel per face
            calculateFresnelCube(angleDeg, wavelengths, output, baseIndex);
            break;
        case Geometry::Cube:
        default:
            calculateFresnelCube(angleDeg, wavelengths, output, baseIndex);
            break;
    }
}

// ==============================================================================
// MATERIAL INTERPOLATION
// ==============================================================================

/**
 * Interpola los 8 puntos de transmisión del material a 50 puntos.
 *
 * Usamos interpolación lineal simple. En Python usabas scipy interp1d
 * con 'cubic', pero para audio la diferencia es mínima y esto es más rápido.
 */
void interpolateMaterial(const Material& material,
                         const std::array<float, NUM_WAVELENGTHS>& targetWavelengths,
                         std::array<float, NUM_WAVELENGTHS>& output)
{
    const auto& matWL = material.wavelengths;
    const auto& matTR = material.transmission;

    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
    {
        float wl = targetWavelengths[i];

        // Find surrounding points in material data
        int idx = 0;
        while (idx < 7 && matWL[idx + 1] < wl)
        {
            ++idx;
        }

        // Linear interpolation
        if (wl <= matWL[0])
        {
            output[i] = matTR[0];
        }
        else if (wl >= matWL[7])
        {
            output[i] = matTR[7];
        }
        else
        {
            float t = (wl - matWL[idx]) / (matWL[idx + 1] - matWL[idx]);
            output[i] = lerp(matTR[idx], matTR[idx + 1], t);
        }

        output[i] = clamp(output[i], 0.0f, 1.0f);
    }
}

// ==============================================================================
// SPECTRUM CALCULATION
// ==============================================================================

void calculateSpectrum(const Material& material,
                       const LightSource& light,
                       float angleDeg,
                       std::array<float, NUM_WAVELENGTHS>& output)
{
    // Interpolate material to light wavelengths
    std::array<float, NUM_WAVELENGTHS> materialCurve;
    interpolateMaterial(material, light.wavelengths, materialCurve);

    // Calculate Fresnel transmission
    std::array<float, NUM_WAVELENGTHS> fresnelCurve;
    calculateFresnelSpectral(angleDeg, light.wavelengths, fresnelCurve);

    // Combine: light * material * fresnel
    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
    {
        output[i] = light.intensity[i] * materialCurve[i] * fresnelCurve[i];
        output[i] = clamp(output[i], 0.0f, 1.0f);
    }
}

// ==============================================================================
// MULTI-FACE SPECTRUM CALCULATION
// ==============================================================================

void calculateSpectrumMultiFace(const Material& material,
                                const LightSource& light,
                                const Vec3& lightPosition,
                                const RotationMatrix& rotMatrix,
                                Geometry geometry,
                                std::array<float, NUM_WAVELENGTHS>& output)
{
    std::fill(output.begin(), output.end(), 0.0f);

    // Sphere: rotation doesn't change light interaction
    // A sphere always presents the same curved surface to the light.
    if (geometry == Geometry::Sphere)
    {
        calculateSpectrum(material, light, 10.0f, output);
        return;
    }

    // Define face normals based on geometry
    static const Vec3 cubeNormals[6] = {
        Vec3( 0.0f,  0.0f,  1.0f),   // +Z (front)
        Vec3( 0.0f,  0.0f, -1.0f),   // -Z (back)
        Vec3( 1.0f,  0.0f,  0.0f),   // +X (right)
        Vec3(-1.0f,  0.0f,  0.0f),   // -X (left)
        Vec3( 0.0f,  1.0f,  0.0f),   // +Y (top)
        Vec3( 0.0f, -1.0f,  0.0f)    // -Y (bottom)
    };

    static const Vec3 torusNormals[12] = {
        Vec3( 0.0f,  0.0f,  1.0f),
        Vec3( 0.0f,  0.0f, -1.0f),
        Vec3( 1.0f,  0.0f,  0.0f),
        Vec3(-1.0f,  0.0f,  0.0f),
        Vec3( 0.0f,  1.0f,  0.0f),
        Vec3( 0.0f, -1.0f,  0.0f),
        Vec3( 0.707f,  0.0f,  0.707f),
        Vec3(-0.707f,  0.0f,  0.707f),
        Vec3( 0.707f,  0.0f, -0.707f),
        Vec3(-0.707f,  0.0f, -0.707f),
        Vec3( 0.0f,  0.707f,  0.707f),
        Vec3( 0.0f, -0.707f,  0.707f)
    };

    // Dodecahedron: 12 pentagonal faces with uniformly distributed normals
    static const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    static const Vec3 dodecaNormals[12] = {
        Vec3( 0.0f,  1.0f,  phi).normalized(),
        Vec3( 0.0f, -1.0f,  phi).normalized(),
        Vec3( 0.0f,  1.0f, -phi).normalized(),
        Vec3( 0.0f, -1.0f, -phi).normalized(),
        Vec3( 1.0f,  phi,  0.0f).normalized(),
        Vec3(-1.0f,  phi,  0.0f).normalized(),
        Vec3( 1.0f, -phi,  0.0f).normalized(),
        Vec3(-1.0f, -phi,  0.0f).normalized(),
        Vec3( phi,  0.0f,  1.0f).normalized(),
        Vec3(-phi,  0.0f,  1.0f).normalized(),
        Vec3( phi,  0.0f, -1.0f).normalized(),
        Vec3(-phi,  0.0f, -1.0f).normalized()
    };

    const Vec3* normals;
    int numNormals;

    if (geometry == Geometry::Torus)
    {
        normals = torusNormals;
        numNormals = 12;
    }
    else if (geometry == Geometry::Dodecahedron)
    {
        normals = dodecaNormals;
        numNormals = 12;
    }
    else
    {
        normals = cubeNormals;
        numNormals = 6;
    }

    // Interpolate material transmission to light wavelengths
    std::array<float, NUM_WAVELENGTHS> materialCurve;
    interpolateMaterial(material, light.wavelengths, materialCurve);

    // Temp array for per-face Fresnel
    std::array<float, NUM_WAVELENGTHS> fresnelCurve;

    float totalWeight = 0.0f;

    for (int face = 0; face < numNormals; ++face)
    {
        // Rotate face normal by the object's rotation
        Vec3 rotatedNormal = rotMatrix.apply(normals[face]);

        // cosAngle > 0 means this face sees the light (angle < 90°)
        float cosAngle = lightPosition.dot(rotatedNormal);

        if (cosAngle <= 0.0f)
            continue;  // Back face, doesn't see light

        float angleDeg = std::acos(clamp(cosAngle, -1.0f, 1.0f)) * 180.0f / 3.14159265359f;

        // Weight by how directly this face sees the light
        float weight = cosAngle;

        // Calculate Fresnel spectrum at THIS face's specific angle
        // Each face gets a different Fresnel response based on its angle to light
        calculateFresnelSpectral(angleDeg, light.wavelengths, fresnelCurve, material.refractiveIndex);

        // Add weighted contribution: light * material * fresnel
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
        {
            output[w] += weight * light.intensity[w] * materialCurve[w] * fresnelCurve[w];
        }
        totalWeight += weight;
    }

    // Weighted average: preserves spectral SHAPE variation between rotations
    // Volume variation is handled separately by spectralAmplitude in SynthEngine
    if (totalWeight > 0.0f)
    {
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
        {
            output[w] /= totalWeight;
        }
    }
}
