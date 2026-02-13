/*
  ==============================================================================
    Physics.h
    Elements - Fresnel calculations and spectral interactions
    Ported from Python prototype
  ==============================================================================
*/

#pragma once

#include <array>
#include <vector>
#include <string>
#include <cmath>

// ==============================================================================
// CONSTANTS
// ==============================================================================

// Number of wavelength samples in our spectrum (380nm to 780nm)
constexpr int NUM_WAVELENGTHS = 50;

// Wavelength range (visible light spectrum in nanometers)
constexpr float WAVELENGTH_MIN = 380.0f;
constexpr float WAVELENGTH_MAX = 780.0f;

// Number of materials and light sources
constexpr int NUM_MATERIALS = 10;
constexpr int NUM_LIGHT_SOURCES = 3;

// ==============================================================================
// STRUCTS
// ==============================================================================

/**
 * Material with optical transmission properties.
 *
 * En C++, 'struct' es similar a 'class' pero con miembros públicos por defecto.
 * Usamos struct para datos simples sin comportamiento complejo.
 */
struct Material
{
    std::string name;
    std::array<float, 8> wavelengths;      // 8 puntos de muestreo (nm)
    std::array<float, 8> transmission;     // Transmisión 0-1 en cada punto
    std::string color;                      // Hex color para UI
    float refractiveIndex = 1.5f;           // Index of refraction (IOR)

    // Constructor por defecto (necesario para arrays)
    Material() = default;

    // Constructor con parámetros
    Material(const std::string& n,
             const std::array<float, 8>& wl,
             const std::array<float, 8>& tr,
             const std::string& col,
             float ior = 1.5f)
        : name(n), wavelengths(wl), transmission(tr), color(col), refractiveIndex(ior) {}
};

/**
 * Light source with spectral power distribution.
 *
 * std::array<float, N> es un array de tamaño fijo conocido en compilación.
 * Más seguro y eficiente que arrays C tradicionales (float[]).
 */
struct LightSource
{
    std::string name;
    std::array<float, NUM_WAVELENGTHS> wavelengths;  // 50 puntos
    std::array<float, NUM_WAVELENGTHS> intensity;    // Intensidad 0-1
    std::string color;                                // Hex color para UI

    LightSource() = default;

    LightSource(const std::string& n,
                const std::array<float, NUM_WAVELENGTHS>& wl,
                const std::array<float, NUM_WAVELENGTHS>& inten,
                const std::string& col)
        : name(n), wavelengths(wl), intensity(inten), color(col) {}
};

/**
 * 3D Light position for 3-point lighting system.
 *
 * Usamos una struct simple con 3 floats para el vector 3D.
 * Más ligero que usar una clase Vector3D completa.
 */
struct Vec3
{
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // Normalizar el vector (hacerlo de longitud 1)
    Vec3 normalized() const
    {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0.0f)
            return Vec3(x/len, y/len, z/len);
        return *this;
    }

    // Producto punto (dot product)
    float dot(const Vec3& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }
};

/**
 * Light position info for 3-point lighting.
 */
struct LightPosition
{
    std::string name;
    Vec3 position;       // Dirección normalizada hacia la luz
    float intensity;     // Factor de intensidad 0-1

    LightPosition() : intensity(0) {}
    LightPosition(const std::string& n, const Vec3& pos, float inten)
        : name(n), position(pos.normalized()), intensity(inten) {}
};

/**
 * Object rotation in 3D space (Euler angles in degrees).
 * DEPRECATED: Use RotationMatrix for gimbal-lock-free rotation.
 */
struct Rotation3D
{
    float x, y, z;  // Rotación en grados

    Rotation3D() : x(0), y(0), z(0) {}
    Rotation3D(float rx, float ry, float rz) : x(rx), y(ry), z(rz) {}
};

/**
 * 3x3 Rotation matrix for gimbal-lock-free rotation.
 * Stored in row-major order: m[row][col] = data[row*3 + col]
 */
struct RotationMatrix
{
    float data[9];  // 3x3 matrix in row-major order

    RotationMatrix()
    {
        // Initialize to identity
        data[0] = 1; data[1] = 0; data[2] = 0;
        data[3] = 0; data[4] = 1; data[5] = 0;
        data[6] = 0; data[7] = 0; data[8] = 1;
    }

    // Set from column-major 4x4 OpenGL matrix (extracts upper-left 3x3)
    void setFromColumnMajor4x4(const float* m4x4)
    {
        // OpenGL column-major: element [col][row] = m4x4[col*4 + row]
        // We want row-major: element [row][col] = data[row*3 + col]
        data[0] = m4x4[0];  data[1] = m4x4[4];  data[2] = m4x4[8];   // Row 0
        data[3] = m4x4[1];  data[4] = m4x4[5];  data[5] = m4x4[9];   // Row 1
        data[6] = m4x4[2];  data[7] = m4x4[6];  data[8] = m4x4[10];  // Row 2
    }

    // Apply rotation to a vector: result = M * v
    Vec3 apply(const Vec3& v) const
    {
        return Vec3(
            data[0] * v.x + data[1] * v.y + data[2] * v.z,
            data[3] * v.x + data[4] * v.y + data[5] * v.z,
            data[6] * v.x + data[7] * v.y + data[8] * v.z
        );
    }
};

/**
 * Geometry types for Fresnel calculations.
 *
 * 'enum class' es un enum con tipado fuerte (C++11).
 * Evita colisiones de nombres y requiere prefijo: Geometry::Cube
 */
enum class Geometry
{
    Cube,
    Sphere,
    Torus,
    Dodecahedron
};

// ==============================================================================
// FUNCTION DECLARATIONS
// ==============================================================================

// --- Rotation & Light Angle ---

/**
 * Calculate rotation matrix and apply to a vector.
 * Returns the rotated vector.
 */
Vec3 applyRotation(const Vec3& v, const Rotation3D& rotation);

/**
 * Calculate the angle of incidence for a light on a rotated object.
 * Returns angle in degrees (0 = perpendicular, 90 = grazing).
 * DEPRECATED: Use calculateLightAngleFromMatrix for gimbal-lock-free calculation.
 */
float calculateLightAngle(const Vec3& lightPosition, const Rotation3D& objectRotation);

/**
 * Calculate light angle using rotation matrix directly (no gimbal lock).
 * Returns angle in degrees (0 = perpendicular to light, 180 = facing away).
 */
float calculateLightAngleFromMatrix(const Vec3& lightPosition, const RotationMatrix& rotMatrix);

/**
 * Calculate geometry-aware light angle using rotation matrix.
 * For curved geometries (sphere, torus), samples multiple normals.
 * This is the primary function to use for all light angle calculations.
 */
float calculateLightAngleForGeometryFromMatrix(const Vec3& lightPosition,
                                                const RotationMatrix& rotMatrix,
                                                Geometry geometry);

/**
 * Calculate geometry-aware light angle using multiple surface normals.
 * For curved geometries, this samples multiple normals and averages.
 * DEPRECATED: Use calculateLightAngleForGeometryFromMatrix instead.
 */
float calculateLightAngleForGeometry(const Vec3& lightPosition,
                                      const Rotation3D& objectRotation,
                                      Geometry geometry);

// --- Fresnel Calculations ---

/**
 * Calculate Fresnel transmission for a single refractive index.
 * Used for display purposes (shows average Fresnel effect).
 */
float calculateFresnelFactor(float angleDeg, float refractiveIndex = 1.5f);

/**
 * Calculate wavelength-dependent Fresnel transmission.
 * Output array must have NUM_WAVELENGTHS elements.
 */
void calculateFresnelSpectral(float angleDeg,
                              const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                              std::array<float, NUM_WAVELENGTHS>& output,
                              float baseIndex = 1.5f);

// --- Geometry-specific Fresnel ---

void calculateFresnelCube(float angleDeg,
                          const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                          std::array<float, NUM_WAVELENGTHS>& output,
                          float baseIndex = 1.5f);

void calculateFresnelSphere(float angleDeg,
                            const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                            std::array<float, NUM_WAVELENGTHS>& output,
                            float baseIndex = 1.5f,
                            int numSamples = 32);

void calculateFresnelTorus(float angleDeg,
                           const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                           std::array<float, NUM_WAVELENGTHS>& output,
                           float baseIndex = 1.5f,
                           int numSamples = 16);

/**
 * Dispatch Fresnel calculation based on geometry type.
 */
void calculateGeometryFresnel(Geometry geometry,
                              float angleDeg,
                              const std::array<float, NUM_WAVELENGTHS>& wavelengths,
                              std::array<float, NUM_WAVELENGTHS>& output,
                              float baseIndex = 1.5f);

// --- Spectrum Calculation ---

/**
 * Calculate resulting spectrum from material + light + Fresnel interaction.
 * This is the main function for single-light spectrum calculation.
 */
void calculateSpectrum(const Material& material,
                       const LightSource& light,
                       float angleDeg,
                       std::array<float, NUM_WAVELENGTHS>& output);

/**
 * Calculate spectrum by summing Fresnel contributions from ALL visible faces.
 * This replaces the single-angle approach which missed variation when one face
 * dominated (e.g. top face always winning during Y rotation).
 *
 * For each face that sees the light (angle < 90°):
 *   1. Calculate its Fresnel spectrum at that angle
 *   2. Weight by how directly it faces the light (cosAngle)
 *   3. Sum all weighted contributions
 *
 * Result: rotation ALWAYS produces spectral variation because different faces
 * contribute different amounts at different angles.
 */
void calculateSpectrumMultiFace(const Material& material,
                                const LightSource& light,
                                const Vec3& lightPosition,
                                const RotationMatrix& rotMatrix,
                                Geometry geometry,
                                std::array<float, NUM_WAVELENGTHS>& output);

// --- Utility Functions ---

/**
 * Linear interpolation between two values.
 */
inline float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

/**
 * Clamp value between min and max.
 *
 * 'inline' sugiere al compilador insertar el código directamente
 * en lugar de hacer una llamada a función. Más rápido para funciones pequeñas.
 */
inline float clamp(float value, float minVal, float maxVal)
{
    return std::max(minVal, std::min(maxVal, value));
}

/**
 * Convert degrees to radians.
 */
inline float degToRad(float degrees)
{
    return degrees * 3.14159265359f / 180.0f;
}

/**
 * Interpolate material transmission to match light wavelengths.
 * Material has 8 sample points, output has NUM_WAVELENGTHS points.
 */
void interpolateMaterial(const Material& material,
                         const std::array<float, NUM_WAVELENGTHS>& targetWavelengths,
                         std::array<float, NUM_WAVELENGTHS>& output);

// ==============================================================================
// GLOBAL DATA ACCESS
// ==============================================================================

/**
 * Get all predefined materials.
 *
 * Retorna una referencia constante al array estático.
 * 'const' = no se puede modificar, '&' = referencia (no copia).
 */
const std::array<Material, NUM_MATERIALS>& getMaterials();

/**
 * Get all predefined light sources.
 */
const std::array<LightSource, NUM_LIGHT_SOURCES>& getLightSources();

/**
 * Get light position by key: 0=key, 1=fill, 2=rim
 */
const LightPosition& getLightPosition(int index);

/**
 * Generate standard wavelength array (380-780nm, 50 points).
 */
std::array<float, NUM_WAVELENGTHS> generateWavelengths();
