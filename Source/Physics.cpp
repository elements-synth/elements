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

float calculateAverageTransmission(const Material& material)
{
    float sum = 0.0f;
    for (int i = 0; i < material.numSamples; ++i)
        sum += material.transmission[static_cast<size_t>(i)];
    return sum / static_cast<float>(material.numSamples);
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
    // Material(name, wavelengths, transmission/reflectance, color, n, k, metallicFactor)
    // n   = real part of complex IOR
    // k   = extinction coefficient (imaginary part): 0 = dielectric, >0 = metal
    // metallic = 0 → pure dielectric path (Beer-Lambert + real Fresnel)
    //            1 → pure metallic path (complex Fresnel, no Beer-Lambert)
    // 32-point wavelength grid shared by all materials (380–780nm, ~12.9nm steps)
    // Simple materials upsampled from 16-pt source data via linear interpolation.
    // Complex materials hand-designed at 32 pts (band positions from literature).
    // See science.md for sources and methodology.
    static std::array<Material, NUM_MATERIALS> materials = {{
        // Pure dielectrics: k=0, metallic=0
        // Diamond — Sellmeier (1923), k≈0, near-flat visible
        Material("Diamond",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.920f, 0.930f, 0.939f, 0.945f, 0.950f, 0.954f, 0.959f, 0.964f,
                   0.969f, 0.970f, 0.970f, 0.970f, 0.970f, 0.970f, 0.970f, 0.967f,
                   0.962f, 0.960f, 0.960f, 0.960f, 0.960f, 0.958f, 0.953f, 0.950f,
                   0.950f, 0.950f, 0.950f, 0.950f, 0.945f, 0.940f, 0.940f, 0.940f },
                 "#E8F4FF", 2.42f, 0.0f, 0.0f),
        // Water — Pope & Fry (1997), 1m Beer-Lambert
        Material("Water",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.989f, 0.992f, 0.995f, 0.995f, 0.995f, 0.993f, 0.991f, 0.989f,
                   0.987f, 0.978f, 0.967f, 0.960f, 0.956f, 0.948f, 0.938f, 0.917f,
                   0.888f, 0.844f, 0.789f, 0.749f, 0.730f, 0.705f, 0.670f, 0.629f,
                   0.576f, 0.496f, 0.328f, 0.182f, 0.126f, 0.072f, 0.045f, 0.018f },
                 "#50C8E8", 1.33f, 0.0f, 0.0f),
        // Amber — PMC12196071, UV cutoff ~440nm
        Material("Amber",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.010f, 0.020f, 0.029f, 0.053f, 0.078f, 0.142f, 0.204f, 0.316f,
                   0.441f, 0.549f, 0.649f, 0.713f, 0.761f, 0.804f, 0.842f, 0.871f,
                   0.891f, 0.908f, 0.922f, 0.934f, 0.943f, 0.952f, 0.957f, 0.960f,
                   0.960f, 0.961f, 0.966f, 0.970f, 0.970f, 0.970f, 0.970f, 0.970f },
                 "#FFBF00", 1.55f, 0.0f, 0.0f),
        // Ruby — PMC9330567, Cr³⁺ absorption bands 413/550nm
        Material("Ruby",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.02f, 0.02f, 0.03f, 0.06f, 0.10f, 0.12f, 0.13f, 0.10f,
                   0.08f, 0.05f, 0.03f, 0.02f, 0.02f, 0.02f, 0.04f, 0.08f,
                   0.15f, 0.30f, 0.55f, 0.75f, 0.85f, 0.90f, 0.93f, 0.95f,
                   0.97f, 0.96f, 0.97f, 0.97f, 0.97f, 0.97f, 0.98f, 0.98f },
                 "#E0115F", 1.77f, 0.01f, 0.0f),
        // Metals: k>>0, metallic=1.0 — transmission[] repurposed as spectral reflectance weights
        // Gold — Palik (1985), complex IOR: absorbs blue/UV, reflects yellow/red
        Material("Gold",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.010f, 0.010f, 0.010f, 0.015f, 0.020f, 0.024f, 0.029f, 0.034f,
                   0.039f, 0.068f, 0.108f, 0.273f, 0.495f, 0.645f, 0.751f, 0.830f,
                   0.885f, 0.920f, 0.940f, 0.954f, 0.963f, 0.970f, 0.970f, 0.971f,
                   0.976f, 0.980f, 0.980f, 0.980f, 0.980f, 0.980f, 0.980f, 0.980f },
                 "#FFD700", 0.47f, 2.83f, 1.0f),
        // Emerald — Wood & Nassau (1968), Cr³⁺ absorption 430/610nm
        Material("Emerald",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.04f, 0.07f, 0.09f, 0.07f, 0.04f, 0.12f, 0.28f, 0.48f,
                   0.65f, 0.78f, 0.87f, 0.90f, 0.89f, 0.88f, 0.82f, 0.70f,
                   0.50f, 0.28f, 0.14f, 0.08f, 0.06f, 0.06f, 0.06f, 0.05f,
                   0.04f, 0.04f, 0.03f, 0.03f, 0.03f, 0.02f, 0.02f, 0.02f },
                 "#50C878", 1.57f, 0.0f, 0.0f),
        // Amethyst — PMC7483767, [FeO₄]⁰ charge transfer ~545nm
        Material("Amethyst",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.880f, 0.861f, 0.841f, 0.812f, 0.782f, 0.736f, 0.691f, 0.595f,
                   0.484f, 0.363f, 0.238f, 0.163f, 0.110f, 0.105f, 0.129f, 0.162f,
                   0.202f, 0.246f, 0.294f, 0.342f, 0.387f, 0.432f, 0.472f, 0.507f,
                   0.531f, 0.547f, 0.532f, 0.519f, 0.509f, 0.499f, 0.490f, 0.480f },
                 "#9966CC", 1.54f, 0.0f, 0.0f),
        // Sapphire — GIA 2020, Fe²⁺–Ti⁴⁺ IVCT ~570nm
        Material("Sapphire",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.88f, 0.82f, 0.85f, 0.86f, 0.83f, 0.78f, 0.75f, 0.68f,
                   0.58f, 0.46f, 0.34f, 0.24f, 0.16f, 0.11f, 0.08f, 0.07f,
                   0.07f, 0.06f, 0.05f, 0.04f, 0.03f, 0.03f, 0.02f, 0.02f,
                   0.02f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f },
                 "#0F52BA", 1.77f, 0.0f, 0.0f),
        // Copper — Palik (1985), complex IOR, deep red character
        Material("Copper",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.014f,
                   0.019f, 0.020f, 0.020f, 0.023f, 0.028f, 0.036f, 0.046f, 0.055f,
                   0.065f, 0.111f, 0.189f, 0.274f, 0.363f, 0.461f, 0.561f, 0.663f,
                   0.769f, 0.853f, 0.868f, 0.882f, 0.902f, 0.921f, 0.936f, 0.950f },
                 "#B87333", 0.46f, 2.83f, 1.0f),
        // Obsidian — Icarus 2021 / USGS, volcanic glass, featureless rise toward IR
        Material("Obsidian",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.010f, 0.014f,
                   0.019f, 0.020f, 0.020f, 0.023f, 0.028f, 0.036f, 0.046f, 0.055f,
                   0.065f, 0.083f, 0.107f, 0.138f, 0.173f, 0.218f, 0.278f, 0.344f,
                   0.421f, 0.493f, 0.551f, 0.603f, 0.638f, 0.672f, 0.696f, 0.720f },
                 "#1C1C1C", 1.50f, 0.0f, 0.0f),
        // Alexandrite — PMC7145866, Cr³⁺ dual-peak: green (440-570nm) + red (640-780nm)
        Material("Alexandrite",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.07f, 0.09f, 0.10f, 0.08f, 0.05f, 0.08f, 0.22f, 0.45f,
                   0.62f, 0.72f, 0.74f, 0.70f, 0.60f, 0.42f, 0.25f, 0.16f,
                   0.20f, 0.32f, 0.46f, 0.58f, 0.68f, 0.76f, 0.82f, 0.86f,
                   0.88f, 0.88f, 0.88f, 0.87f, 0.88f, 0.88f, 0.89f, 0.90f },
                 "#5B8A64", 1.745f, 0.0f, 0.0f),
        // Malachite — USGS splib07 K-M, Cu²⁺ LMCT+d-d, green window
        Material("Malachite",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.08f, 0.12f, 0.18f, 0.25f, 0.34f, 0.45f, 0.56f, 0.66f,
                   0.74f, 0.81f, 0.86f, 0.89f, 0.90f, 0.88f, 0.84f, 0.78f,
                   0.70f, 0.62f, 0.53f, 0.45f, 0.38f, 0.32f, 0.27f, 0.22f,
                   0.19f, 0.16f, 0.14f, 0.13f, 0.12f, 0.12f, 0.11f, 0.11f },
                 "#2E7D52", 1.85f, 0.0f, 0.0f),
        // Neodymium — Nd:glass laser lit., f-f transitions from 4I9/2 ground state
        // Bands: ~432nm (4G11/2), ~522nm (4G9/2), ~583nm (4G5/2), ~625nm (2H9/2), ~677nm (4F7/2), ~741nm (4F5/2)
        Material("Neodymium",
                 { 380.0f, 393.0f, 406.0f, 419.0f, 432.0f, 445.0f, 457.0f, 470.0f,
                   483.0f, 496.0f, 509.0f, 522.0f, 535.0f, 548.0f, 561.0f, 574.0f,
                   587.0f, 600.0f, 613.0f, 626.0f, 638.0f, 651.0f, 664.0f, 677.0f,
                   690.0f, 703.0f, 716.0f, 728.0f, 741.0f, 754.0f, 767.0f, 780.0f },
                 { 0.72f, 0.78f, 0.82f, 0.85f, 0.55f, 0.80f, 0.86f, 0.82f,
                   0.75f, 0.85f, 0.78f, 0.18f, 0.72f, 0.86f, 0.88f, 0.30f,
                   0.06f, 0.52f, 0.78f, 0.38f, 0.72f, 0.85f, 0.78f, 0.30f,
                   0.68f, 0.82f, 0.84f, 0.72f, 0.38f, 0.76f, 0.86f, 0.88f },
                 "#9070C8", 1.636f, 0.0f, 0.0f)
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
        //   Blue (wlPos=0): decayRate = 6.0 → drops very fast with angle
        //   Red  (wlPos=1): decayRate = 1.0 → resists angle changes
        // Higher multiplier = more dramatic timbre shifts with rotation
        float decayRate = 1.0f + 5.0f * (1.0f - wlPos);
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
    const int n = material.numSamples;

    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
    {
        float wl = targetWavelengths[i];

        // Find surrounding points in material data
        int idx = 0;
        while (idx < n - 2 && matWL[idx + 1] < wl)
            ++idx;

        // Linear interpolation
        if (wl <= matWL[0])
            output[i] = matTR[0];
        else if (wl >= matWL[n - 1])
            output[i] = matTR[n - 1];
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
    // Interpolate material spectral curve (transmission for dielectrics,
    // reflectance for metals) to the 50-point light wavelength grid
    std::array<float, NUM_WAVELENGTHS> materialCurve;
    interpolateMaterial(material, light.wavelengths, materialCurve);

    float metallic = material.metallicFactor;

    // ==========================================================================
    // DIELECTRIC PATH  (metallicFactor = 0)
    // Uses real-valued IOR, Beer-Lambert transmission model.
    // Fresnel output is a transmission factor (1 = all passes, 0 = all reflected).
    // Strong angle-dependent spectral shaping (blues drop first with angle).
    // ==========================================================================
    std::array<float, NUM_WAVELENGTHS> dielectricOut{};
    if (metallic < 0.999f)
    {
        std::array<float, NUM_WAVELENGTHS> fresnelCurve;
        // Pass the material's actual IOR (previously hard-coded to 1.5f — bug fix)
        calculateFresnelSpectral(angleDeg, light.wavelengths, fresnelCurve,
                                 material.refractiveIndex);
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
            dielectricOut[i] = clamp(light.intensity[i] * materialCurve[i] * fresnelCurve[i],
                                     0.0f, 1.0f);
    }

    // ==========================================================================
    // METALLIC PATH  (metallicFactor = 1)
    // Uses complex IOR (n + ik). F0 computed from both n and k.
    // Schlick approximation from the correct metallic F0.
    // materialCurve[] is now the spectral reflectance (not transmission).
    // Angle-spectral shaping is ~10x weaker than dielectric — metals barely
    // change colour with viewing angle.
    // ==========================================================================
    std::array<float, NUM_WAVELENGTHS> metallicOut{};
    if (metallic > 0.001f)
    {
        float n = material.refractiveIndex;
        float k = material.extinctionCoeff;

        // Complex Fresnel F0: R = ((n-1)² + k²) / ((n+1)² + k²)
        float F0 = ((n - 1.0f) * (n - 1.0f) + k * k)
                 / ((n + 1.0f) * (n + 1.0f) + k * k);

        // Schlick angle-dependent reflectance
        float cosI = std::cos(degToRad(clamp(angleDeg, 0.0f, 89.0f)));
        float schlick = F0 + (1.0f - F0) * std::pow(1.0f - cosI, 5.0f);

        // Very subtle spectral modulation with angle: metals stay their colour
        // at all angles (0.5–1.5 decay vs 1–6 for dielectrics)
        float angleFactor = clamp(angleDeg / 90.0f, 0.0f, 1.0f);
        for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        {
            float wlPos = (light.wavelengths[i] - 380.0f) / 400.0f;
            float decayRate = 0.5f + 1.0f * (1.0f - wlPos);  // 0.5–1.5, much flatter than dielectric
            float spectralMod = std::pow(1.0f - angleFactor * 0.3f, decayRate);  // 0.3 caps the angle effect
            metallicOut[i] = clamp(light.intensity[i] * materialCurve[i] * schlick * spectralMod,
                                   0.0f, 1.0f);
        }
    }

    // ==========================================================================
    // BLEND: lerp between paths based on metallicFactor
    // Pure dielectric (0) or pure metal (1) takes the respective path fully.
    // Mixed materials (e.g. dielectric+metallic blend) get a weighted sum.
    // ==========================================================================
    for (int i = 0; i < NUM_WAVELENGTHS; ++i)
        output[i] = clamp(lerp(dielectricOut[i], metallicOut[i], metallic), 0.0f, 1.0f);
}

// ==============================================================================
// MULTI-FACE SPECTRUM CALCULATION
// ==============================================================================

void calculateSpectrumMultiFace(const Material& material,
                                const LightSource& light,
                                const Vec3& lightPosition,
                                const RotationMatrix& rotMatrix,
                                Geometry geometry,
                                std::array<float, NUM_WAVELENGTHS>& output,
                                float deformAmount,
                                float deformFrequency,
                                float noiseTimeOffset)
{
    std::fill(output.begin(), output.end(), 0.0f);

    // Sphere: when undeformed, rotation doesn't change light interaction.
    // A perfect sphere always presents the same curved surface to the light.
    // When deformed (deformAmount > 0), noise bumps tilt surface normals,
    // making Fresnel angles vary with rotation — just like a faceted geometry.
    // We crossfade between the two paths to avoid volume jumps at the threshold.
    if (geometry == Geometry::Sphere)
    {
        // Always compute the undeformed baseline spectrum
        std::array<float, NUM_WAVELENGTHS> undeformed{};
        calculateSpectrum(material, light, 10.0f, undeformed);

        if (deformAmount <= 0.001f)
        {
            output = undeformed;
            return;
        }

        // Sample 12 uniformly-distributed directions (icosahedron vertices)
        // and compute displaced normals from simplex noise
        static const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
        static const float icosaLen = std::sqrt(1.0f + phi * phi);
        static const Vec3 icosaDirs[12] = {
            Vec3( 0.0f,  1.0f / icosaLen,  phi / icosaLen),
            Vec3( 0.0f, -1.0f / icosaLen,  phi / icosaLen),
            Vec3( 0.0f,  1.0f / icosaLen, -phi / icosaLen),
            Vec3( 0.0f, -1.0f / icosaLen, -phi / icosaLen),
            Vec3( 1.0f / icosaLen,  phi / icosaLen,  0.0f),
            Vec3(-1.0f / icosaLen,  phi / icosaLen,  0.0f),
            Vec3( 1.0f / icosaLen, -phi / icosaLen,  0.0f),
            Vec3(-1.0f / icosaLen, -phi / icosaLen,  0.0f),
            Vec3( phi / icosaLen,  0.0f,  1.0f / icosaLen),
            Vec3(-phi / icosaLen,  0.0f,  1.0f / icosaLen),
            Vec3( phi / icosaLen,  0.0f, -1.0f / icosaLen),
            Vec3(-phi / icosaLen,  0.0f, -1.0f / icosaLen)
        };

        // Compute displaced normals via noise gradient (finite differences)
        Vec3 deformedNormals[12];
        constexpr float eps = 0.01f;
        const float freq = deformFrequency;
        const float scale = deformAmount * 0.3f;

        for (int i = 0; i < 12; ++i)
        {
            const Vec3& p = icosaDirs[i];
            float px = p.x * freq + noiseTimeOffset, py = p.y * freq, pz = p.z * freq;

            float dndx = (simplex3D(px + eps, py, pz) - simplex3D(px - eps, py, pz)) / (2.0f * eps);
            float dndy = (simplex3D(px, py + eps, pz) - simplex3D(px, py - eps, pz)) / (2.0f * eps);
            float dndz = (simplex3D(px, py, pz + eps) - simplex3D(px, py, pz - eps)) / (2.0f * eps);
            Vec3 grad(dndx, dndy, dndz);

            float radialComp = grad.dot(p);
            Vec3 gradTan(grad.x - p.x * radialComp,
                         grad.y - p.y * radialComp,
                         grad.z - p.z * radialComp);

            Vec3 displaced(p.x - gradTan.x * scale,
                           p.y - gradTan.y * scale,
                           p.z - gradTan.z * scale);
            deformedNormals[i] = displaced.normalized();
        }

        // Multi-face Fresnel with deformed normals
        std::array<float, NUM_WAVELENGTHS> deformedOutput{};
        std::array<float, NUM_WAVELENGTHS> materialCurve;
        interpolateMaterial(material, light.wavelengths, materialCurve);

        std::array<float, NUM_WAVELENGTHS> fresnelCurve;
        float totalWeight = 0.0f;

        for (int face = 0; face < 12; ++face)
        {
            Vec3 rotatedNormal = rotMatrix.apply(deformedNormals[face]);
            float cosAngle = lightPosition.dot(rotatedNormal);
            if (cosAngle <= 0.0f)
                continue;

            float angleDeg = std::acos(clamp(cosAngle, -1.0f, 1.0f)) * 180.0f / 3.14159265359f;
            float weight = cosAngle * cosAngle;

            calculateFresnelSpectral(angleDeg, light.wavelengths, fresnelCurve, material.refractiveIndex);

            for (int w = 0; w < NUM_WAVELENGTHS; ++w)
                deformedOutput[w] += weight * light.intensity[w] * materialCurve[w] * fresnelCurve[w];

            totalWeight += weight;
        }

        if (totalWeight > 0.0f)
        {
            for (int w = 0; w < NUM_WAVELENGTHS; ++w)
                deformedOutput[w] /= totalWeight;
        }

        // Crossfade: smooth transition between undeformed and deformed spectra.
        // At deformAmount=0 → 100% undeformed, at >=0.15 → 100% deformed.
        constexpr float blendRange = 0.15f;
        float blend = std::min(deformAmount / blendRange, 1.0f);

        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
            output[w] = undeformed[w] * (1.0f - blend) + deformedOutput[w] * blend;

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
        // Steeper weighting = dominant face contributes more = more timbral variation
        // Cube: pow(4) for dramatic 6-position transitions
        // Torus/Dodecahedron: pow(2) for smoother but still noticeable contrast
        float weight;
        if (geometry == Geometry::Cube)
            weight = cosAngle * cosAngle * cosAngle * cosAngle;  // pow(cosAngle, 4)
        else
            weight = cosAngle * cosAngle;  // pow(cosAngle, 2)

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

// ==============================================================================
// 3D SIMPLEX NOISE
// ==============================================================================

namespace {

static const int snPerm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

static float snGrad3(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

} // anonymous namespace

float simplex3D(float x, float y, float z)
{
    constexpr float F3 = 1.0f / 3.0f;
    constexpr float G3 = 1.0f / 6.0f;

    float s = (x + y + z) * F3;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    int k = static_cast<int>(std::floor(z + s));

    float t = (i + j + k) * G3;
    float x0 = x - (i - t);
    float y0 = y - (j - t);
    float z0 = z - (k - t);

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0)      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else               { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if (y0 < z0)       { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else               { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - i1 + G3, y1 = y0 - j1 + G3, z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2*G3, y2 = y0 - j2 + 2*G3, z2 = z0 - k2 + 2*G3;
    float x3 = x0 - 1 + 3*G3, y3 = y0 - 1 + 3*G3, z3 = z0 - 1 + 3*G3;

    int ii = i & 255, jj = j & 255, kk = k & 255;

    float n = 0.0f;
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 > 0) { t0 *= t0; n += t0 * t0 * snGrad3(snPerm[ii + snPerm[jj + snPerm[kk]]], x0, y0, z0); }
    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 > 0) { t1 *= t1; n += t1 * t1 * snGrad3(snPerm[ii+i1 + snPerm[jj+j1 + snPerm[kk+k1]]], x1, y1, z1); }
    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 > 0) { t2 *= t2; n += t2 * t2 * snGrad3(snPerm[ii+i2 + snPerm[jj+j2 + snPerm[kk+k2]]], x2, y2, z2); }
    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 > 0) { t3 *= t3; n += t3 * t3 * snGrad3(snPerm[ii+1 + snPerm[jj+1 + snPerm[kk+1]]], x3, y3, z3); }

    return 32.0f * n;
}
