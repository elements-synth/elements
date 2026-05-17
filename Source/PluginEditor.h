/*
  ==============================================================================
    PluginEditor.h
    Elements - Complete Plugin GUI with 3D Viewport
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Shaders.h"
#include "ElementsUI.h"

// ==============================================================================
// VIEWPORT 3D - Renders geometry with OpenGL
// ==============================================================================

class Viewport3D : public juce::Component,
                   public juce::OpenGLRenderer,
                   public juce::Timer
{
public:
    Viewport3D(ElementsAudioProcessor& p);
    ~Viewport3D() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Mouse interaction for rotation gizmo
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void setGeometry(Geometry geom) { currentGeometry = geom; }
    void setMaterialColour(juce::Colour c) { materialColour = c; }
    void resetRotation();
    void setRotationFromEuler(float x, float y, float z);

private:
    ElementsAudioProcessor& processor;
    juce::OpenGLContext openGLContext;

    Geometry currentGeometry = Geometry::Sphere;
    juce::Colour materialColour{0xFFE8F4FF};

    // Mouse rotation state
    enum class DragAxis { None, X, Y, Z };
    bool isDragging = false;
    DragAxis lockedAxis = DragAxis::None;
    DragAxis hoveredAxis = DragAxis::None;
    juce::Point<float> lastMousePos;
    float dragSensitivity = 0.5f;

    DragAxis hitTestGizmo(juce::Point<float> mousePos);

    // Accumulated rotation matrix (column-major, OpenGL format)
    // This stores the full object orientation so each drag rotation
    // is relative to the current orientation (local-space rotation).
    float rotationMatrix[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    void applyIncrementalRotation(float angleDeg, float axisX, float axisY, float axisZ);
    void resetRotationMatrix();
    void extractEulerAngles(float& outX, float& outY, float& outZ) const;

    // Cumulative rotation values for UI display (independent of matrix extraction)
    // These accumulate continuously without gimbal lock artifacts
    float cumulativeRotX = 0.0f;
    float cumulativeRotY = 0.0f;
    float cumulativeRotZ = 0.0f;

    // Camera orbit + zoom
    float cameraTilt = 25.0f;    // degrees, vertical orbit
    float cameraRotY = -30.0f;   // degrees, horizontal orbit
    float cameraDist = 7.0f;     // distance from origin
    bool isOrbiting = false;
    juce::Point<float> lastOrbitPos;

    // Thickness for shader uniform
    float currentThickness = 1.0f;

    // Dirty flag for optimization - only repaint when needed
    bool needsRepaint = true;
    int lastMaterial = -1;
    Geometry lastGeometry = Geometry::Cube;
    int lastRotVersion = 0;
    int rotVersion = 0;
    bool lastLightEnabled[3] = {false, false, false};
    int lastLightSource[3] = {0, 0, 0};
    float lastLightIntensity[3] = {0.5f, 0.5f, 0.5f};

    // Light positions for visualization
    struct Vec3 { float x, y, z; };
    Vec3 keyLightPosition{2.5f, 2.2f, 2.5f};
    Vec3 fillLightPosition{-1.6f, 0.9f, 1.2f};
    Vec3 rimLightPosition{0.0f, 2.0f, -2.5f};

    // Legacy fixed-function helpers (grid, axes, gizmo, lights)
    void drawGrid(float size, int divisions);
    void drawAxes(float length);
    void drawRotationGizmo(float radius);
    void drawLightIndicators();

    void setupLighting();

    // === PBR Shader Pipeline ===
    struct PBRVertex {
        float position[3];
        float normal[3];
    };

    struct PBRMaterialProps {
        float metallic;
        float roughness;
        float ior;
        float transparency;
        float sssStrength;
        float sssRadius;
        float absorptionColor[3];
        bool  hasChromism;
        float albedoSecondary[3];
        float bandingStrength;
        float bandingFrequency;
    };

    // PBR material properties — metal, rough, ior, transp, sssStr, sssRad, absRGB, chromism, secRGB, bandStr, bandFreq
    static constexpr PBRMaterialProps pbrMaterials[NUM_MATERIALS] = {
        { 0.00f, 0.05f, 2.42f,  0.95f, 0.15f, 0.3f, {0.97f, 0.98f, 1.00f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Diamond
        { 0.00f, 0.10f, 1.33f,  0.90f, 0.05f, 0.5f, {0.70f, 0.85f, 0.95f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Water
        { 0.00f, 0.30f, 1.55f,  0.70f, 0.40f, 0.4f, {1.00f, 0.75f, 0.20f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Amber
        { 0.05f, 0.15f, 1.77f,  0.60f, 0.50f, 0.3f, {0.90f, 0.10f, 0.25f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Ruby
        { 1.00f, 0.20f, 0.47f,  0.00f, 0.00f, 0.0f, {1.00f, 0.84f, 0.00f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Gold
        { 0.05f, 0.20f, 1.57f,  0.55f, 0.35f, 0.3f, {0.20f, 0.80f, 0.40f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Emerald
        { 0.05f, 0.25f, 1.54f,  0.65f, 0.35f, 0.3f, {0.60f, 0.30f, 0.80f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Amethyst
        { 0.05f, 0.10f, 1.77f,  0.60f, 0.40f, 0.3f, {0.10f, 0.30f, 0.85f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Sapphire
        { 1.00f, 0.20f, 0.46f,  0.00f, 0.00f, 0.0f, {0.97f, 0.57f, 0.32f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Copper
        { 0.00f, 0.03f, 1.50f,  0.00f, 0.00f, 0.0f, {0.02f, 0.02f, 0.02f}, false, {0.0f, 0.0f, 0.0f}, 0.0f, 10.0f },  // Obsidian
        { 0.00f, 0.10f, 1.745f, 0.68f, 0.30f, 0.3f, {0.28f, 0.60f, 0.35f}, true,  {0.80f, 0.26f, 0.22f}, 0.0f, 10.0f }, // Alexandrite
        { 0.00f, 0.28f, 1.85f,  0.00f, 0.00f, 0.0f, {0.15f, 0.55f, 0.22f}, false, {0.0f, 0.0f, 0.0f}, 0.55f, 14.0f },  // Malachite
        { 0.00f, 0.05f, 1.636f, 0.75f, 0.00f, 0.0f, {0.50f, 0.40f, 0.80f}, true,  {0.65f, 0.28f, 0.65f}, 0.0f, 10.0f }, // Neodymium — Nd³⁺ glass, violet↔red chromism
    };

    // Shader program
    std::unique_ptr<juce::OpenGLShaderProgram> pbrShader;
    bool shaderReady = false;

    // VBO/VAO per geometry
    GLuint cubeVBO = 0, sphereVBO = 0, torusVBO = 0, dodecaVBO = 0;
    int cubeVertexCount = 0, sphereVertexCount = 0, torusVertexCount = 0, dodecaVertexCount = 0;

    // Vertex data generation
    std::vector<PBRVertex> generateCubeVertices(float size);
    std::vector<PBRVertex> generateSphereVertices(float radius, int segments);
    std::vector<PBRVertex> generateTorusVertices(float majorR, float minorR, int segments);
    std::vector<PBRVertex> generateDodecahedronVertices(float radius);

    void createVBOs();
    void destroyVBOs();
    bool compileShader();
    void renderGeometryPBR();

    // Displaced sphere (simplex noise deformation)
    std::vector<PBRVertex> baseSphereVerts;
    GLuint displacedSphereVBO = 0;
    bool displacedVBODirty = true;
    float lastDeformAmount = 0.0f;
    float lastDeformFrequency = 2.0f;
    float noiseTimeOffset = 0.0f;
    void updateDisplacedSphere(float amount, float frequency);

    // Environment map (equirectangular HDR loaded as GL_TEXTURE_2D)
    GLuint envTexture = 0;
    int envWidth = 0, envHeight = 0;
    void createEnvironmentMap();
    void destroyEnvironmentMap();

    // Matrix helpers for shader uniforms
    void buildProjectionMatrix(float* out, float fovDeg, float aspect, float near, float far);
    void buildViewMatrix(float* out, float tiltDeg, float rotYDeg, float dist);
    void getCameraPosition(float* outPos, float tiltDeg, float rotYDeg, float dist);
};

// ==============================================================================
// SPECTRUM DISPLAY
// ==============================================================================

class SpectrumDisplay : public juce::Component,
                        public juce::Timer
{
public:
    SpectrumDisplay(ElementsAudioProcessor& p);
    ~SpectrumDisplay() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    ElementsAudioProcessor& processor;
    juce::Colour wavelengthToColour(float wavelength);
};

// ==============================================================================
// OSCILLOSCOPE DISPLAY
// ==============================================================================

class OscilloscopeDisplay : public juce::Component,
                            public juce::Timer
{
public:
    OscilloscopeDisplay(ElementsAudioProcessor& p, bool isOscB = false);
    ~OscilloscopeDisplay() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    void pushSample(float sample);
    void setWaveformColour(juce::Colour c) { waveformColour = c; }

private:
    ElementsAudioProcessor& processor;
    bool useOscB = false;
    std::array<float, 512> waveformBuffer{};
    int writePosition = 0;
    juce::Colour waveformColour { 0xFF4A90E2 };
};

// ==============================================================================
// ADSR DISPLAY - Envelope curve visualization
// ==============================================================================

class ADSRDisplay : public juce::Component,
                    public juce::Timer
{
public:
    ADSRDisplay(ElementsAudioProcessor& p, bool isFilterEnv = false)
        : processor(p), filterMode(isFilterEnv) { startTimerHz(30); }

    void paint(juce::Graphics& g) override;
    void timerCallback() override { repaint(); }
    void setEnvelopeColour(juce::Colour c) { envelopeColour = c; }

private:
    ElementsAudioProcessor& processor;
    bool filterMode = false;
    juce::Colour envelopeColour { 0xFF4A90E2 };
};

// ==============================================================================
// ELEMENTS LOGO - PNG from BinaryData
// ==============================================================================

class ElementsLogo : public juce::Component
{
public:
    ElementsLogo()
    {
        logoImage = juce::ImageFileFormat::loadFrom(
            BinaryData::elementslogo_png,
            static_cast<size_t>(BinaryData::elementslogo_pngSize));
    }

    void paint(juce::Graphics& g) override
    {
        if (logoImage.isValid())
        {
            g.drawImage(logoImage, getLocalBounds().toFloat(),
                        juce::RectanglePlacement::yMid);
        }
    }

private:
    juce::Image logoImage;
};

// ==============================================================================
// HELP OVERLAY - Modal help panel with tabbed content
// ==============================================================================

namespace HelpContent
{
    inline juce::String about()
    {
        return
            "ELEMENTS\n"
            "Spectral Wavetable Synthesizer\n"
            "\n"
            "Elements generates audio by simulating how light interacts\n"
            "with physical materials. Each material has unique optical\n"
            "properties that translate into distinct harmonic content.\n"
            "\n"
            "The synthesis engine uses real physics data:\n"
            "  - Refractive indices determine harmonic structure\n"
            "  - Light absorption curves shape the frequency spectrum\n"
            "  - Material thickness controls spectral filtering\n"
            "\n"
            "Version 0.9.4 (Beta)";
    }

    inline juce::String materials()
    {
        return
            "MATERIALS\n"
            "\n"
            "Each material produces different timbres based on its\n"
            "optical transmission curve (see Science tab).\n"
            "\n"
            "--- MATERIAL MIXING ---\n"
            "\n"
            "Elements runs two independent oscillators (A and B),\n"
            "each shaped by its own material spectrum. The BLEND\n"
            "mode defines how they interact at the sample level.\n"
            "\n"
            "MIX: dry/wet between pure A (0) and the blend result.\n"
            "DETUNE: pitch offset for oscillator B (±100 cents).\n"
            "DEPTH: modulation depth — used by AM and FM only.\n"
            "MUTE A: audition oscillator B in isolation.\n"
            "\n"
            "RING MOD\n"
            "Multiplies A and B sample-by-sample. Produces sum and\n"
            "difference frequencies — sidebands that are not part\n"
            "of either source spectrum. Inharmonic, metallic, and\n"
            "highly dependent on both material choices. Strongest\n"
            "at MIX=1 with contrasting materials.\n"
            "\n"
            "AM\n"
            "B modulates the amplitude of A. At DEPTH=0, pure A\n"
            "passes through unchanged. As DEPTH rises, amplitude\n"
            "sidebands appear alongside the original carrier — you\n"
            "hear A enriched with new harmonics. Warmer and more\n"
            "controlled than Ring Mod; the original timbre stays\n"
            "recognisable.\n"
            "\n"
            "XOR\n"
            "Takes the absolute difference |A-B|, preserving the\n"
            "sign of A. Highlights spectral disagreement between\n"
            "materials — regions where they diverge most become\n"
            "loudest. Subtractive and hollow; pairing materials\n"
            "with contrasting curves (e.g. Ruby + Diamond) produces\n"
            "the most harmonic complexity.\n"
            "\n"
            "FM\n"
            "B modulates the phase of A before wavetable readout.\n"
            "DEPTH controls the FM index. Low DEPTH: subtle pitch\n"
            "drift and shimmer. High DEPTH: rich inharmonic spectra\n"
            "typical of DX-style FM synthesis, now coloured by\n"
            "both material curves. The most spectrally complex mode.\n"
            "\n"
            "--- GEMS ---\n"
            "\n"
            "{#ffa8d8f0}DIAMOND\n"
            "Highly refractive (n=2.42). Near-flat transmission\n"
            "across the visible spectrum. Bright, crystalline sound\n"
            "with strong upper harmonics. Sharp, clear transients.\n"
            "\n"
            "{#ffe84b6a}RUBY\n"
            "Cr\u00b3\u207a absorbs at 413nm + 550nm. Blue-violet window\n"
            "and red transmission above 620nm. Rich harmonic\n"
            "content — two spectral clusters, warm saturated tone.\n"
            "\n"
            "{#ff4ecb8d}EMERALD\n"
            "Cr\u00b3\u207a in beryl: absorption at 430nm + 610nm opens a\n"
            "green window (480-560nm). Balanced, focused spectrum.\n"
            "\n"
            "{#ffb57bee}AMETHYST\n"
            "[FeO\u2084]\u2070 absorbs at 545nm, transmitting violet + red.\n"
            "Bimodal harmonic structure with mid-range dip.\n"
            "\n"
            "{#ff5b9ef5}SAPPHIRE\n"
            "Fe\u00b2\u207a-Ti\u2074\u207a charge transfer: broad absorption at 570nm.\n"
            "Transmits blue only. Note: produces no sound under\n"
            "Sunset light — use Daylight or LED Cool.\n"
            "\n"
            "{#ff5b8a64}ALEXANDRITE\n"
            "Cr\u00b3\u207a dual-peak: green (490-570nm) + red (640-780nm)\n"
            "windows. Two harmonic clusters. Colour changes with\n"
            "light source — the alexandrite effect.\n"
            "\n"
            "--- MINERALS ---\n"
            "\n"
            "{#fff5b942}AMBER\n"
            "Organic polymer: UV cutoff at ~440nm, smooth warm\n"
            "sigmoid. Mid-high emphasis. Gentle, organic character.\n"
            "\n"
            "{#ff2e7d52}MALACHITE\n"
            "Cu\u00b2\u207a LMCT cuts blue, d-d band cuts red, leaving a\n"
            "green window (490-560nm). Focused, mineral timbre.\n"
            "\n"
            "{#ff6a7a8a}OBSIDIAN\n"
            "Volcanic glass: featureless, dark, monotonically rising\n"
            "toward red/NIR. Deep, subdued, minimal harmonics.\n"
            "\n"
            "--- METALS ---\n"
            "\n"
            "{#ffd4a843}GOLD\n"
            "Complex IOR (n=0.47 k=2.83). Interband edge at ~500nm\n"
            "absorbs blue/UV. Warm metallic shimmer, yellow-red\n"
            "harmonic weighting.\n"
            "\n"
            "{#ffcf7e46}COPPER\n"
            "Complex IOR (n=0.46 k=2.83). Deeper interband edge\n"
            "at ~600nm. Absorbs blue-green, emphasises orange-red.\n"
            "Smooth metallic resonance.\n"
            "\n"
            "--- SPECIAL ---\n"
            "\n"
            "{#ff7ec8e3}WATER\n"
            "Direct Beer-Lambert from measured data (1m path).\n"
            "Flat in visible, steep OH overtone drop above 700nm.\n"
            "Soft, fluid timbre with strong high-harmonic roll-off.\n"
            "\n"
            "{#ff9070c8}NEODYMIUM\n"
            "Nd\u00b3\u207a rare-earth glass: 6 narrow f-f absorption bands\n"
            "(432/522/583/625/677/741nm). Comb-filter character.\n"
            "Harmonically rich, otherworldly, complex spectrum.";
    }

    inline juce::String science()
    {
        return
            "SPECTRAL SCIENCE\n"
            "\n"
            "Each material curve shows optical transmission (0-1)\n"
            "across the visible spectrum (380-780nm, 32 samples).\n"
            "These curves directly shape the harmonic spectrum.\n"
            "\n"
            "--- GEMS ---\n"
            "\n"
            "{#ffa8d8f0}DIAMOND  n=2.42  Sellmeier (1923)\n"
            "Near-flat visible transmission. Slight UV scatter\n"
            "loss at 380nm (Rayleigh). No absorbing chromophores.\n"
            "\n"
            "{#ffe84b6a}RUBY  n=1.77  PMC9330567 (Rossman 2022)\n"
            "Cr\u00b3\u207a in corundum: absorbs at 413nm + 550nm, transmits\n"
            "blue-violet window (455-480nm) + red above 620nm.\n"
            "\n"
            "{#ff4ecb8d}EMERALD  n=1.57  Wood & Nassau (1968)\n"
            "Cr\u00b3\u207a in beryl: weaker crystal field than ruby shifts\n"
            "absorption to 430nm + 610nm, opening the green window.\n"
            "\n"
            "{#ffb57bee}AMETHYST  n=1.54  PMC7483767 (Hatipoglu 2020)\n"
            "[FeO\u2084]\u2070 centre absorbs at 545nm. Transmits violet + red,\n"
            "producing the characteristic purple colour.\n"
            "\n"
            "{#ff5b9ef5}SAPPHIRE  n=1.77  GIA 2020 (Dubinsky et al.)\n"
            "Fe\u00b2\u207a-Ti\u2074\u207a intervalence charge transfer: broad absorption\n"
            "centred at 570nm. Fe\u00b3\u207a adds dips at 393nm + 450nm.\n"
            "\n"
            "{#ff5b8a64}ALEXANDRITE  n=1.75  PMC7145866 (Taran 2020)\n"
            "Cr\u00b3\u207a in chrysoberyl: two transmission peaks (green +\n"
            "red) create the alexandrite colour-change effect.\n"
            "\n"
            "--- MINERALS ---\n"
            "\n"
            "{#fff5b942}AMBER  n=1.54  PMC12196071 (Wolfe 2025)\n"
            "Polycyclic aromatics create a UV cutoff at ~440nm.\n"
            "No sharp visible bands. Smooth warm sigmoid.\n"
            "\n"
            "{#ff2e7d52}MALACHITE  n=1.85  USGS splib07 / K-M\n"
            "Cu\u00b2\u207a LMCT below 450nm + d-d band above 700nm leave\n"
            "a narrow green window. Converted via Kubelka-Munk.\n"
            "\n"
            "{#ff6a7a8a}OBSIDIAN  n=1.50  Icarus 2021 (Cloutis et al.)\n"
            "Volcanic glass: featureless monotonic rise UV-to-NIR.\n"
            "Fe\u00b2\u207a + magnetite nanoparticles cause broadband absorption.\n"
            "\n"
            "--- METALS ---\n"
            "\n"
            "{#ffd4a843}GOLD  n=0.47 k=2.83  Palik (1985)\n"
            "Interband transition at ~500nm. Absorbs blue/UV,\n"
            "reflects yellow/red. Complex IOR metallic path.\n"
            "\n"
            "{#ffcf7e46}COPPER  n=0.46 k=2.83  Palik (1985)\n"
            "Deeper interband edge at ~600nm vs gold. Absorbs\n"
            "blue-green, reflects orange-red. Metallic path.\n"
            "\n"
            "--- RARE EARTH ---\n"
            "\n"
            "{#ff9070c8}NEODYMIUM  n=1.64  Kaminskii (1981)\n"
            "Nd\u00b3\u207a f-f transitions from \u2074I\u2089/\u2082 ground state: 6 narrow\n"
            "absorption bands (432/522/583/625/677/741nm).\n"
            "32-point grid required to resolve band structure.\n"
            "\n"
            "--- WATER ---\n"
            "\n"
            "{#ff7ec8e3}WATER  n=1.33  Pope & Fry (1997)\n"
            "Direct Beer-Lambert from measured absorption data\n"
            "(1m path). OH overtone causes steep drop above 700nm.";
    }

    inline juce::String geometry()
    {
        return
            "GEOMETRY\n"
            "\n"
            "The 3D shape affects how light interacts with the material:\n"
            "\n"
            "CUBE\n"
            "Sharp edges create distinct spectral peaks.\n"
            "Uniform light distribution. Clear, focused harmonics.\n"
            "\n"
            "SPHERE\n"
            "Smooth surface produces gradual spectral changes.\n"
            "Even light diffusion. Softer, rounder timbre.\n"
            "\n"
            "TORUS\n"
            "Complex internal reflections. Variable harmonic\n"
            "emphasis. Rich, evolving spectrum.\n"
            "\n"
            "DODECAHEDRON\n"
            "Multiple facets create complex interactions.\n"
            "Dense harmonic content. Intricate spectral texture.";
    }

    inline juce::String lights()
    {
        return
            "LIGHT SOURCES\n"
            "\n"
            "Each light type has a unique color temperature:\n"
            "\n"
            "{#ffE07830}SUNSET (2000K)\n"
            "Warm, orange-red spectrum. Emphasizes lower harmonics.\n"
            "Soft, vintage character.\n"
            "\n"
            "{#ffD4A843}DAYLIGHT (5500K)\n"
            "Neutral, balanced spectrum. Full harmonic range.\n"
            "Natural, clear sound.\n"
            "\n"
            "{#ff7EC8E3}LED COOL (6500K)\n"
            "Blue-white spectrum. Emphasizes upper harmonics.\n"
            "Bright, modern character.\n"
            "\n"
            "You can enable up to 3 lights simultaneously.\n"
            "The spectrum combines their contributions.\n"
            "\n"
            "INTENSITY\n"
            "Each light has an intensity slider (0.0 - 1.0).\n"
            "Intensity controls how much each light contributes\n"
            "to the spectrum and to the 3D viewport lighting.\n"
            "\n"
            "  0.5 = equilibrium (default, no pitch effect)\n"
            "  > 0.5 = brighter light, pitch shifts up\n"
            "  < 0.5 = dimmer light, pitch shifts down\n"
            "\n"
            "Pitch modulation range: +/- 2 semitones.\n"
            "The effect is cumulative: more lights at high\n"
            "intensity push pitch further up. Double-click\n"
            "the slider to reset to 0.5.";
    }

    inline juce::String viewport()
    {
        return
            "3D VIEWPORT\n"
            "\n"
            "CAMERA\n"
            "  Right-click + drag: Orbit camera around object\n"
            "  Scroll wheel: Zoom in / out\n"
            "\n"
            "ROTATION GIZMO\n"
            "  Drag the colored rings to rotate the object:\n"
            "  Red ring (X), Green ring (Y), Blue ring (Z)\n"
            "  Rotation affects the Fresnel angle between light\n"
            "  and surface, changing the timbre.\n"
            "\n"
            "ROTATION FIELDS\n"
            "  Type exact rotation values (0-360) in the X/Y/Z\n"
            "  fields on the left side. Click Reset to return\n"
            "  to default orientation.\n"
            "\n"
            "LIGHT INDICATORS\n"
            "  The 3 light bulbs show position and color of\n"
            "  each active light. Bulb brightness reflects the\n"
            "  intensity slider value.\n"
            "\n"
            "THICKNESS\n"
            "  Top-right slider. Controls material depth in\n"
            "  the light path (Beer-Lambert absorption).";
    }

    inline juce::String controls()
    {
        return
            "CONTROLS REFERENCE\n"
            "\n"
            "FILTER\n"
            "  Cutoff: Low-pass filter frequency (20Hz - 20kHz)\n"
            "  Reso: Filter resonance (emphasis at cutoff)\n"
            "  Env Amt: Modulation depth from Filter Envelope\n"
            "\n"
            "FILTER ENVELOPE\n"
            "  Attack: Time to reach peak brightness\n"
            "  Decay: Time to decay to sustain level\n"
            "  Sustain: Held brightness level\n"
            "  Release: Fade-out time after note off\n"
            "\n"
            "AMP ENVELOPE\n"
            "  Attack: Volume fade-in time\n"
            "  Decay: Time to reach sustain level\n"
            "  Sustain: Held volume level\n"
            "  Release: Volume fade-out time\n"
            "\n"
            "  Mode selector (Classic / Physical):\n"
            "\n"
            "  Classic: Standard ADSR controlled by knobs.\n"
            "\n"
            "  Physical: ADSR values are derived from optics.\n"
            "  The envelope shape changes automatically based\n"
            "  on material, thickness, and light intensity:\n"
            "    Attack  = light intensity (brighter = faster)\n"
            "    Decay   = thickness x absorption (thick = slow)\n"
            "    Sustain = refractive index (Diamond high, Water low)\n"
            "    Release = IOR x thickness (dense = long release)\n"
            "  In Physical mode the ADSR knobs are overridden.\n"
            "\n"
            "OUTPUT\n"
            "  Volume: Master output level\n"
            "\n"
            "THICKNESS\n"
            "  Material depth in the light path.\n"
            "  Affects spectral filtering and absorption.";
    }
}

class HelpOverlay : public juce::Component
{
public:
    std::function<void()> onClose;

    HelpOverlay()
    {
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(true);

        tabs = { "About", "Materials", "Science", "Geometry", "Lights", "Viewport", "Controls" };
        content = {
            HelpContent::about(),
            HelpContent::materials(),
            HelpContent::science(),
            HelpContent::geometry(),
            HelpContent::lights(),
            HelpContent::viewport(),
            HelpContent::controls()
        };

        spectraImage = juce::ImageFileFormat::loadFrom(
            BinaryData::elements_spectra_panel_png,
            static_cast<size_t>(BinaryData::elements_spectra_panel_pngSize));
    }

    void paint(juce::Graphics& g) override
    {
        // Semi-transparent dark backdrop
        g.fillAll(juce::Colour(0xF00D1117));

        recalcLayout();

        // Panel background
        g.setColour(ElementsColors::bg1);
        g.fillRoundedRectangle(fullPanelBounds.toFloat(), 6.0f);
        g.setColour(ElementsColors::border);
        g.drawRoundedRectangle(fullPanelBounds.toFloat(), 6.0f, 1.0f);

        // Tab bar
        int tabW = tabBarBounds.getWidth() / static_cast<int>(tabs.size());

        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
        {
            auto tabRect = juce::Rectangle<int>(tabBarBounds.getX() + i * tabW,
                                                 tabBarBounds.getY(), tabW, tabBarBounds.getHeight());

            if (i == activeTab)
            {
                g.setColour(ElementsColors::bg3);
                g.fillRoundedRectangle(tabRect.toFloat().reduced(2, 2), 3.0f);
                g.setColour(juce::Colour(0xFFa8d8f0));
            }
            else
            {
                g.setColour(ElementsColors::mid);
            }

            g.setFont(juce::Font(12.5f, juce::Font::bold));
            g.drawText(tabs[static_cast<size_t>(i)].toUpperCase(), tabRect, juce::Justification::centred);
        }

        // Tab bar bottom border
        g.setColour(ElementsColors::border);
        g.drawHorizontalLine(tabBarBounds.getBottom(),
                             static_cast<float>(fullPanelBounds.getX()),
                             static_cast<float>(fullPanelBounds.getRight()));

        // Content area (below tab bar, with padding)
        auto contentArea = juce::Rectangle<int>(fullPanelBounds.getX(),
                                                 tabBarBounds.getBottom(),
                                                 fullPanelBounds.getWidth(),
                                                 fullPanelBounds.getBottom() - tabBarBounds.getBottom())
                               .reduced(32, 24);

        // Science tab: image scrolls with content as the first item
        static const int scienceTabIndex = 2;
        int scienceImgOffset = 0;
        if (activeTab == scienceTabIndex && spectraImage.isValid())
        {
            float imgAspect = static_cast<float>(spectraImage.getWidth()) / spectraImage.getHeight();
            int imgW = contentArea.getWidth();
            int imgH = juce::roundToInt(imgW / imgAspect);
            scienceImgOffset = imgH + 8;

            int imgY = contentArea.getY() - scrollOffset;
            if (imgY + imgH > contentArea.getY() && imgY < contentArea.getBottom())
            {
                g.saveState();
                g.reduceClipRegion(contentArea);
                g.drawImage(spectraImage,
                            juce::Rectangle<float>((float)contentArea.getX(), (float)imgY, (float)imgW, (float)imgH),
                            juce::RectanglePlacement::fillDestination);
                g.restoreState();
            }
        }

        drawFormattedContent(g, contentArea, content[static_cast<size_t>(activeTab)], scienceImgOffset);

        // Close button (top-right of panel)
        g.setColour(ElementsColors::mid);
        g.setFont(juce::Font(22.0f));
        g.drawText(juce::String::charToString(0x00D7), closeBounds, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();

        // Close button
        if (closeBounds.contains(pos))
        {
            dismiss();
            return;
        }

        // Tab clicks
        if (tabBarBounds.contains(pos))
        {
            int tabW = tabBarBounds.getWidth() / static_cast<int>(tabs.size());
            int clickedTab = (pos.x - tabBarBounds.getX()) / tabW;
            if (clickedTab >= 0 && clickedTab < static_cast<int>(tabs.size()))
            {
                activeTab = clickedTab;
                scrollOffset = 0;
                repaint();
            }
            return;
        }

        // Click outside panel = close
        if (!fullPanelBounds.contains(pos))
            dismiss();
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        scrollOffset -= static_cast<int>(wheel.deltaY * 120.0f);
        scrollOffset = juce::jmax(0, scrollOffset);
        repaint();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

    void dismiss()
    {
        setVisible(false);
        if (onClose) onClose();
    }

private:
    void drawFormattedContent(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text, int topOffset = 0)
    {
        auto lines = juce::StringArray::fromLines(text);
        float lineH = 22.0f;
        float totalH = lines.size() * lineH + static_cast<float>(topOffset);
        float visibleH = static_cast<float>(area.getHeight());
        int maxScroll = juce::jmax(0, static_cast<int>(totalH - visibleH));
        scrollOffset = juce::jmin(scrollOffset, maxScroll);

        // Reserve space for scrollbar if content overflows
        int scrollBarW = (totalH > visibleH) ? 6 : 0;
        auto textArea = area.withTrimmedRight(scrollBarW + 4);

        float y = static_cast<float>(area.getY()) - scrollOffset + static_cast<float>(topOffset);
        float x = static_cast<float>(textArea.getX());
        float w = static_cast<float>(textArea.getWidth());

        // Clip to content area
        g.saveState();
        g.reduceClipRegion(area);

        for (auto& line : lines)
        {
            if (y + lineH > area.getY() - lineH && y < area.getBottom() + lineH)
            {
                // Check for color tag: {#AARRGGBB}TEXT
                juce::String displayLine = line;
                juce::Colour titleColour(0xFFa8d8f0);
                bool hasColorTag = line.startsWith("{#") && line.indexOf("}") == 10;

                if (hasColorTag)
                {
                    auto hex = line.substring(2, 10);
                    titleColour = juce::Colour(static_cast<juce::uint32>(hex.getHexValue64()));
                    displayLine = line.substring(11);
                }

                bool isTitle = displayLine.isNotEmpty()
                               && displayLine == displayLine.toUpperCase()
                               && !displayLine.startsWith(" ");

                if (isTitle)
                {
                    g.setFont(juce::Font(15.0f, juce::Font::bold));
                    g.setColour(titleColour);
                    g.drawText(displayLine, static_cast<int>(x), static_cast<int>(y), static_cast<int>(w),
                               static_cast<int>(lineH), juce::Justification::centredLeft);
                }
                else
                {
                    g.setFont(juce::Font(13.0f));
                    g.setColour(ElementsColors::text);
                    g.drawText(displayLine, static_cast<int>(x), static_cast<int>(y), static_cast<int>(w),
                               static_cast<int>(lineH), juce::Justification::centredLeft);
                }
            }
            y += lineH;
        }

        // Scrollbar
        if (totalH > visibleH)
        {
            float trackX = static_cast<float>(area.getRight() - scrollBarW);
            float trackY = static_cast<float>(area.getY());
            float trackH = visibleH;

            // Track
            g.setColour(ElementsColors::bg3.withAlpha(0.4f));
            g.fillRoundedRectangle(trackX, trackY, static_cast<float>(scrollBarW), trackH, 3.0f);

            // Thumb
            float thumbRatio = visibleH / totalH;
            float thumbH = juce::jmax(20.0f, trackH * thumbRatio);
            float scrollRange = trackH - thumbH;
            float thumbY = trackY + (maxScroll > 0 ? scrollRange * (static_cast<float>(scrollOffset) / maxScroll) : 0.0f);

            g.setColour(ElementsColors::mid.withAlpha(0.6f));
            g.fillRoundedRectangle(trackX, thumbY, static_cast<float>(scrollBarW), thumbH, 3.0f);
        }

        g.restoreState();
    }

    void recalcLayout()
    {
        auto area = getLocalBounds();
        int panelW = juce::jmin(680, area.getWidth() - 40);
        int panelH = juce::jmin(520, area.getHeight() - 40);
        fullPanelBounds = juce::Rectangle<int>(0, 0, panelW, panelH).withCentre(area.getCentre());
        tabBarBounds = juce::Rectangle<int>(fullPanelBounds.getX(), fullPanelBounds.getY(),
                                             fullPanelBounds.getWidth(), 36);
        closeBounds = juce::Rectangle<int>(fullPanelBounds.getRight() - 34,
                                            fullPanelBounds.getY() + 2, 32, 32);
    }

    std::vector<juce::String> tabs;
    std::vector<juce::String> content;
    juce::Image spectraImage;
    int activeTab = 0;
    int scrollOffset = 0;
    juce::Rectangle<int> fullPanelBounds;
    juce::Rectangle<int> tabBarBounds;
    juce::Rectangle<int> closeBounds;
};

// ==============================================================================
// VIEWPORT ACCORDION - Collapsible overlay panels (GEO / MAT)
// ==============================================================================

class AccordionPanel : public juce::Component
{
public:
    AccordionPanel() { setInterceptsMouseClicks(true, true); }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xA20D1117));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colour(0x20FFFFFF));
        g.drawRect(getLocalBounds().toFloat(), 1.0f);
    }
};

class ViewportAccordion : public juce::Component
{
public:
    static constexpr int kHeaderH = 28;
    static constexpr int kGeoPanH = 108;
    static constexpr int kMatPanH = 224;

    std::function<void()> onLayoutChanged;
    AccordionPanel geoPanel, matPanel;

    ViewportAccordion()
    {
        setInterceptsMouseClicks(true, true);
        addAndMakeVisible(geoPanel);  geoPanel.setVisible(false);
        addAndMakeVisible(matPanel);  matPanel.setVisible(false);
    }

    bool isGeoOpen() const { return open[0]; }
    bool isMatOpen() const { return open[1]; }

    int getNeededHeight() const
    {
        int maxPH = 0;
        if (open[0]) maxPH = juce::jmax(maxPH, kGeoPanH);
        if (open[1]) maxPH = juce::jmax(maxPH, kMatPanH);
        return kHeaderH + maxPH;
    }

    bool hitTest(int x, int y) override
    {
        if (y < kHeaderH) return true;
        int half = getWidth() / 2;
        if (x < half  && open[0] && y < kHeaderH + kGeoPanH) return true;
        if (x >= half && open[1] && y < kHeaderH + kMatPanH) return true;
        return false;
    }

    void paint(juce::Graphics& g) override
    {
        int w = getWidth();
        int half = w / 2;

        g.setColour(juce::Colour(0xA20D1117));
        g.fillRect(0, 0, w, kHeaderH);

        const char* labels[] = { "GEOMETRIES", "MATERIALS" };
        for (int i = 0; i < 2; ++i)
        {
            int x = i * half;
            if (open[i])
            {
                g.setColour(juce::Colour(0x18a8d8f0));
                g.fillRect(x + 1, 1, half - 2, kHeaderH - 2);
            }
            if (i > 0)
            {
                g.setColour(juce::Colour(0x28FFFFFF));
                g.drawVerticalLine(x, 2.0f, (float)(kHeaderH - 2));
            }
            juce::String arrow = open[i]
                ? juce::String::charToString(0x25BC)   // ▼ open
                : juce::String::charToString(0x25B6);  // ▶ closed
            g.setColour(open[i] ? juce::Colour(0xFFa8d8f0) : juce::Colour(0xFF5A6A78));
            g.setFont(juce::Font(11.5f, juce::Font::bold));
            g.drawText(arrow + "  " + labels[i], x + 14, 0, half - 28, kHeaderH,
                       juce::Justification::centredLeft);
        }
        g.setColour(juce::Colour(0x30FFFFFF));
        g.drawHorizontalLine(kHeaderH, 0.0f, (float)w);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.y >= kHeaderH) return;
        int idx = e.x / (getWidth() / 2);
        if (idx < 0 || idx > 1) return;
        open[idx] = !open[idx];
        (idx == 0 ? geoPanel : matPanel).setVisible(open[idx]);
        resized();
        repaint();
        if (onLayoutChanged) onLayoutChanged();
    }

    void resized() override
    {
        int half = getWidth() / 2;
        geoPanel.setBounds(0,    kHeaderH, half, open[0] ? kGeoPanH : 0);
        matPanel.setBounds(half, kHeaderH, half, open[1] ? kMatPanH : 0);
    }

private:
    bool open[2] = { false, false };
};

// ==============================================================================
// SPLASH OVERLAY - Startup branding screen
// ==============================================================================

class SplashOverlay : public juce::Component,
                      private juce::Timer
{
public:
    std::function<void()> onClose;

    SplashOverlay()
    {
        setInterceptsMouseClicks(true, false);
        logoImage = juce::ImageFileFormat::loadFrom(
            BinaryData::elementslogo_png,
            static_cast<size_t>(BinaryData::elementslogo_pngSize));

        // Start 2-second delay before fade begins
        startTimer(2000);
    }

    void paint(juce::Graphics& g) override
    {
        g.setOpacity(currentAlpha);

        // Dark backdrop
        g.setColour(juce::Colour(0xFF0D1117));
        g.fillRect(getLocalBounds());

        auto area = getLocalBounds();
        auto cx = area.getCentreX();
        auto cy = area.getCentreY();

        // Logo (centered)
        if (logoImage.isValid())
        {
            int logoW = 220;
            int logoH = static_cast<int>(logoW * (static_cast<float>(logoImage.getHeight()) / logoImage.getWidth()));
            g.drawImage(logoImage,
                        cx - logoW / 2, cy - logoH / 2 - 20, logoW, logoH,
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
        }

        // Version text
        juce::String versionStr = "v" + juce::String(JucePlugin_VersionString);
        g.setFont(juce::Font(14.0f));
        g.setColour(ElementsColors::mid.withAlpha(currentAlpha));
        int versionW = g.getCurrentFont().getStringWidth(versionStr);

        // BETA badge
        juce::String betaStr = "BETA";
        auto boldFont = juce::Font(11.0f, juce::Font::bold);
        int betaW = boldFont.getStringWidth(betaStr);
        int badgePad = 8;
        int badgeH = 18;
        int gap = 8;

        int totalW = versionW + gap + betaW + badgePad * 2;
        int startX = cx - totalW / 2;
        int textY = cy + 24;

        // Draw version
        g.setFont(juce::Font(14.0f));
        g.setColour(ElementsColors::mid.withAlpha(currentAlpha));
        g.drawText(versionStr, startX, textY, versionW, 20, juce::Justification::centredLeft);

        // Draw BETA badge
        int badgeX = startX + versionW + gap;
        auto badgeColour = MaterialAccents::diamond;
        g.setColour(badgeColour.withAlpha(currentAlpha * 0.15f));
        g.fillRoundedRectangle(static_cast<float>(badgeX), static_cast<float>(textY + 1),
                                static_cast<float>(betaW + badgePad * 2), static_cast<float>(badgeH), 4.0f);
        g.setColour(badgeColour.withAlpha(currentAlpha));
        g.drawRoundedRectangle(static_cast<float>(badgeX), static_cast<float>(textY + 1),
                                static_cast<float>(betaW + badgePad * 2), static_cast<float>(badgeH), 4.0f, 1.0f);
        g.setFont(boldFont);
        g.drawText(betaStr, badgeX + badgePad, textY + 1, betaW, badgeH, juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        if (!fading)
        {
            // First callback: 3s elapsed, start fading
            fading = true;
            startTimerHz(30);  // Switch to 30Hz for smooth fade
            return;
        }

        // Fade out over ~500ms (30Hz * ~15 frames)
        currentAlpha -= 1.0f / 15.0f;
        if (currentAlpha <= 0.0f)
        {
            currentAlpha = 0.0f;
            stopTimer();
            setVisible(false);
            if (onClose) onClose();
        }
        repaint();
    }

    juce::Image logoImage;
    float currentAlpha = 1.0f;
    bool fading = false;
};

// ==============================================================================
// PIANO ROLL - Visual keyboard
// ==============================================================================

class PianoRoll : public juce::Component,
                  public juce::Timer
{
public:
    PianoRoll(ElementsAudioProcessor& p);
    ~PianoRoll() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setHighlightColour(juce::Colour c) { highlightColour = c; }

private:
    ElementsAudioProcessor& processor;

    int startOctave = 2;
    int numOctaves = 5;
    int currentNote = -1;

    int getNoteFromPosition(juce::Point<int> pos);
    juce::Rectangle<int> getKeyBounds(int note, bool isBlack);
    bool isBlackKey(int note);

    std::array<bool, 128> activeNotes{};
    juce::Colour highlightColour { 0xFF4A90E2 };
};

// ==============================================================================
// LIGHT PANEL - Single light control
// ==============================================================================

class LightPanel : public juce::Component,
                   public juce::Button::Listener,
                   public juce::ComboBox::Listener
{
public:
    LightPanel(ElementsAudioProcessor& p, int lightIndex, const juce::String& name);
    ~LightPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* combo) override;

    void setEnabled(bool enabled);
    bool isLightEnabled() const { return enableButton.getToggleState(); }

    juce::Slider& getIntensitySlider() { return intensitySlider; }

private:
    ElementsAudioProcessor& processor;
    int lightIndex;
    juce::String lightName;

    juce::ToggleButton enableButton;
    juce::ComboBox sourceCombo;
    juce::Label intensityLabel;
    juce::Slider intensitySlider;
};

// ==============================================================================
// CUSTOM LOOK AND FEEL
// ==============================================================================

class ElementsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ElementsLookAndFeel();

    void setAccent(juce::Colour c) { currentAccent = c; }
    juce::Colour getAccent() const { return currentAccent; }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Color lookup for popup menu items
    juce::Colour getColourForItemText(const juce::String& text) const;

    // Force JetBrains Mono for all fonts
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

    void loadKnobFramesFromBinaryData();
    bool hasKnobFrames() const { return ! knobFramesOriginal.empty(); }

private:
    void rebuildTintedFrames();

    juce::Colour currentAccent { MaterialAccents::diamond };
    juce::Typeface::Ptr jbmRegular;
    juce::Typeface::Ptr jbmBold;

    // Filmstrip knob frames (loaded from disk for testing)
    std::vector<juce::Image> knobFramesOriginal;  // untinted source
    std::vector<juce::Image> knobFrames;           // tinted for current accent
};

// ==============================================================================
// MAIN EDITOR
// ==============================================================================

class ElementsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::Slider::Listener,
                                     public juce::Button::Listener,
                                     public juce::ComboBox::Listener,
                                     public juce::Label::Listener,
                                     public juce::Timer
{
public:
    explicit ElementsAudioProcessorEditor(ElementsAudioProcessor&);
    ~ElementsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* combo) override;
    void labelTextChanged(juce::Label* label) override;

private:
    ElementsAudioProcessor& audioProcessor;
    ElementsLookAndFeel lookAndFeel;

    // === TOOLBAR: Logo + Geometry + Material dropdowns + Help ===
    ElementsLogo elementsLogo;
    juce::TextButton helpButton{"?"};
    HelpOverlay helpOverlay;
    SplashOverlay splashOverlay;

    // Preset system
    juce::Label presetLabel;
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton{"SAVE"};
    juce::TextButton deletePresetButton{"DEL"};
    void savePreset();
    void deletePreset();
    void loadPreset(const juce::File& file);
    void refreshPresetList();
    void updateDepthEnabled(int blendMode);  // gray out DEPTH when not AM/FM
    juce::File getPresetsDir() const;
    juce::File currentPresetFile;
    juce::ComboBox geoCombo, matCombo, matBCombo, blendModeCombo;
    juce::Label geoLabel, matLabel, matBLabel, blendModeLabel;

    // === LEFT COLUMN: Viewport + Accordion overlay + Lights ===
    Viewport3D viewport3D;
    ViewportAccordion accordion;

    // Lights (kept as LightPanel for now, will become LightsBar in Phase 3)
    juce::Label lightsLabel;
    std::unique_ptr<LightPanel> keyLightPanel;
    std::unique_ptr<LightPanel> fillLightPanel;
    std::unique_ptr<LightPanel> rimLightPanel;

    // Thickness
    juce::Label thicknessLabel;
    juce::Slider thicknessSlider;

    // Deform / Wavefolding
    juce::Label deformLabel;
    juce::Slider deformSlider;

    // Rotation (floating inside viewport)
    juce::Label rotXLabel, rotYLabel, rotZLabel;
    juce::Label rotXValue, rotYValue, rotZValue;
    juce::TextButton resetRotationButton{"Reset"};

    // === BOTTOM: Piano ===
    PianoRoll pianoRoll;

    // === RIGHT COLUMN: Visualizers + Controls ===
    juce::Label spectrumLabel, oscilloscopeLabel, oscilloscopeBLabel;
    SpectrumDisplay spectrumDisplay;
    OscilloscopeDisplay oscilloscopeDisplay;
    OscilloscopeDisplay oscilloscopeDisplayB;

    // Mix/blend slider (in viewport overlay, second row)
    juce::Label mixLabel;
    juce::Slider mixSlider;
    juce::Label detuneLabel;
    juce::Slider detuneSlider;
    juce::Label modDepthLabel;
    juce::Slider modDepthSlider;
    juce::TextButton oscAMuteButton{"MUTE A"};
    juce::DrawableButton swapMaterialsButton{"swap", juce::DrawableButton::ImageFitted};
    ADSRDisplay adsrDisplay;
    ADSRDisplay filterAdsrDisplay;

    // Filter
    juce::Label filterLabel;
    juce::ToggleButton filterBypassButton{"ON"};
    juce::Slider filterCutoffSlider, filterResonanceSlider;
    juce::Label filterCutoffLabel, filterResonanceLabel;
    juce::ComboBox filterTypeCombo;

    // Filter Envelope
    juce::Label filterEnvLabel;
    juce::Slider filterAttackSlider, filterDecaySlider, filterSustainSlider, filterReleaseSlider;
    juce::Label fAttackLabel, fDecayLabel, fSustainLabel, fReleaseLabel;
    juce::Slider filterEnvAmountSlider;
    juce::Label filterEnvAmountLabel;

    // Amplitude Envelope
    juce::Label envelopeLabel;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    // Envelope mode (Classic/Physical)
    juce::ComboBox envModeCombo;

    // Volume
    juce::Label volumeLabel;
    juce::Slider volumeSlider;

    // APVTS attachments (MUST be declared AFTER all sliders/combos so they are
    // destroyed FIRST, before the widgets they reference)
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> resonanceAttachment;
    std::unique_ptr<ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<SliderAttachment> filterAttackAttachment;
    std::unique_ptr<SliderAttachment> filterDecayAttachment;
    std::unique_ptr<SliderAttachment> filterSustainAttachment;
    std::unique_ptr<SliderAttachment> filterReleaseAttachment;
    std::unique_ptr<SliderAttachment> filterEnvAmountAttachment;
    std::unique_ptr<SliderAttachment> ampAttackAttachment;
    std::unique_ptr<SliderAttachment> ampDecayAttachment;
    std::unique_ptr<SliderAttachment> ampSustainAttachment;
    std::unique_ptr<SliderAttachment> ampReleaseAttachment;
    std::unique_ptr<SliderAttachment> thicknessAttachment;
    std::unique_ptr<SliderAttachment> deformAttachment;
    std::unique_ptr<SliderAttachment> keyIntensityAttachment;
    std::unique_ptr<SliderAttachment> fillIntensityAttachment;
    std::unique_ptr<SliderAttachment> rimIntensityAttachment;
    std::unique_ptr<ComboBoxAttachment> envModeAttachment;
    std::unique_ptr<SliderAttachment> volumeAttachment;
    std::unique_ptr<SliderAttachment> mixAmountAttachment;
    std::unique_ptr<SliderAttachment> oscBDetuneAttachment;
    std::unique_ptr<SliderAttachment> modDepthAttachment;

    // === Helpers ===
    void setupRotarySlider(juce::Slider& slider, double min, double max, double def);
    void setupLabel(juce::Label& label, const juce::String& text, float fontSize, bool bold = false);

    // Material data (NUM_MATERIALS = 13 from Physics.h)
    const juce::String materialNames[NUM_MATERIALS] = {
        "Diamond", "Water", "Amber", "Ruby", "Gold", "Emerald", "Amethyst", "Sapphire",
        "Copper", "Obsidian", "Alexandrite", "Malachite", "Neodymium"
    };
    const juce::Colour materialColours[NUM_MATERIALS] = {
        juce::Colour(0xFFE8F4FF), juce::Colour(0xFF50C8E8), juce::Colour(0xFFFFBF00),
        juce::Colour(0xFFE0115F), juce::Colour(0xFFFFD700), juce::Colour(0xFF50C878),
        juce::Colour(0xFF9966CC), juce::Colour(0xFF0F52BA),
        juce::Colour(0xFFB87333), juce::Colour(0xFF1C1C1C),
        juce::Colour(0xFF5B8A64), juce::Colour(0xFF2E7D52), juce::Colour(0xFF9070C8)
    };

    // Section frame rectangles (right column, for paint())
    std::vector<juce::Rectangle<int>> sectionFrames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElementsAudioProcessorEditor)
};
