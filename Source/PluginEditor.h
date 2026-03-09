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
        float ior;               // Index of refraction
        float transparency;      // 0=opaque, 1=fully transparent
        float sssStrength;       // Subsurface scattering intensity
        float sssRadius;         // SSS spread
        float absorptionColor[3]; // RGB tint (from transmission spectrum)
    };

    // PBR material properties indexed by material enum (same order as materialNames)
    //                          metal  rough  ior    transp  sssStr sssRad  absorption R,G,B
    static constexpr PBRMaterialProps pbrMaterials[NUM_MATERIALS] = {
        { 0.00f, 0.05f, 2.42f, 0.95f, 0.15f, 0.3f, {0.97f, 0.98f, 1.00f} },  // Diamond
        { 0.00f, 0.10f, 1.33f, 0.90f, 0.05f, 0.5f, {0.70f, 0.85f, 0.95f} },  // Water
        { 0.00f, 0.30f, 1.55f, 0.70f, 0.40f, 0.4f, {1.00f, 0.75f, 0.20f} },  // Amber
        { 0.05f, 0.15f, 1.77f, 0.60f, 0.50f, 0.3f, {0.90f, 0.10f, 0.25f} },  // Ruby
        { 1.00f, 0.20f, 0.47f, 0.00f, 0.00f, 0.0f, {1.00f, 0.84f, 0.00f} },  // Gold
        { 0.05f, 0.20f, 1.57f, 0.55f, 0.35f, 0.3f, {0.20f, 0.80f, 0.40f} },  // Emerald
        { 0.05f, 0.25f, 1.54f, 0.65f, 0.35f, 0.3f, {0.60f, 0.30f, 0.80f} },  // Amethyst
        { 0.05f, 0.10f, 1.77f, 0.60f, 0.40f, 0.3f, {0.10f, 0.30f, 0.85f} },  // Sapphire
        { 1.00f, 0.20f, 0.46f, 0.00f, 0.00f, 0.0f, {0.97f, 0.57f, 0.32f} },  // Copper  — metallic, orange-tinted like gold
        { 0.00f, 0.03f, 1.50f, 0.00f, 0.00f, 0.0f, {0.02f, 0.02f, 0.02f} },  // Obsidian — black glass, glossy specular
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
    OscilloscopeDisplay(ElementsAudioProcessor& p);
    ~OscilloscopeDisplay() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    void pushSample(float sample);
    void setWaveformColour(juce::Colour c) { waveformColour = c; }

private:
    ElementsAudioProcessor& processor;
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
            "Version 1.0 (Beta)";
    }

    inline juce::String materials()
    {
        return
            "MATERIALS\n"
            "\n"
            "Each material produces different timbres based on its\n"
            "optical properties:\n"
            "\n"
            "{#ffa8d8f0}DIAMOND\n"
            "Highly refractive (n=2.42). Bright, crystalline sound\n"
            "with strong upper harmonics. Sharp, clear transients.\n"
            "\n"
            "{#ff7ec8e3}WATER\n"
            "Low refraction (n=1.33). Soft, fluid timbre.\n"
            "Smooth harmonic roll-off.\n"
            "\n"
            "{#fff5b942}AMBER\n"
            "Warm, organic character. Mid-range emphasis.\n"
            "Gentle high-frequency absorption.\n"
            "\n"
            "{#ffe84b6a}RUBY\n"
            "Rich harmonic content. Warm, saturated tone.\n"
            "Strong fundamental.\n"
            "\n"
            "{#ffd4a843}GOLD\n"
            "Soft, metallic shimmer. Gentle spectral peaks.\n"
            "Warm overall character.\n"
            "\n"
            "{#ff4ecb8d}EMERALD\n"
            "Balanced spectrum. Clear, focused sound.\n"
            "Moderate brightness.\n"
            "\n"
            "{#ffb57bee}AMETHYST\n"
            "Complex harmonic structure.\n"
            "Slight mid-range emphasis.\n"
            "\n"
            "{#ff5b9ef5}SAPPHIRE\n"
            "Clear, focused tone. Strong upper-mid presence.\n"
            "Note: Sapphire only transmits blue wavelengths.\n"
            "It will not produce sound with Sunset light.\n"
            "Use Daylight or LED Cool for best results.\n"
            "\n"
            "{#ffcf7e46}COPPER\n"
            "Metallic, warm resonance. Orange-tinted harmonics.\n"
            "Smooth mid-range with soft high-end roll-off.\n"
            "\n"
            "{#ff6a7a8a}OBSIDIAN\n"
            "Dark, glassy character. Subdued harmonics with\n"
            "glossy specular highlights. Deep, minimal tone.";
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

        tabs = { "About", "Materials", "Geometry", "Lights", "Viewport", "Controls" };
        content = {
            HelpContent::about(),
            HelpContent::materials(),
            HelpContent::geometry(),
            HelpContent::lights(),
            HelpContent::viewport(),
            HelpContent::controls()
        };
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
        drawFormattedContent(g, contentArea, content[static_cast<size_t>(activeTab)]);

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
    void drawFormattedContent(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text)
    {
        auto lines = juce::StringArray::fromLines(text);
        float lineH = 22.0f;
        float totalH = lines.size() * lineH;
        float visibleH = static_cast<float>(area.getHeight());
        int maxScroll = juce::jmax(0, static_cast<int>(totalH - visibleH));
        scrollOffset = juce::jmin(scrollOffset, maxScroll);

        // Reserve space for scrollbar if content overflows
        int scrollBarW = (totalH > visibleH) ? 6 : 0;
        auto textArea = area.withTrimmedRight(scrollBarW + 4);

        float y = static_cast<float>(area.getY()) - scrollOffset;
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
    int activeTab = 0;
    int scrollOffset = 0;
    juce::Rectangle<int> fullPanelBounds;
    juce::Rectangle<int> tabBarBounds;
    juce::Rectangle<int> closeBounds;
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
    juce::ComboBox geoCombo, matCombo;
    juce::Label geoLabel, matLabel;

    // === LEFT COLUMN: Viewport + Lights ===
    Viewport3D viewport3D;

    // Lights (kept as LightPanel for now, will become LightsBar in Phase 3)
    juce::Label lightsLabel;
    std::unique_ptr<LightPanel> keyLightPanel;
    std::unique_ptr<LightPanel> fillLightPanel;
    std::unique_ptr<LightPanel> rimLightPanel;

    // Thickness
    juce::Label thicknessLabel;
    juce::Slider thicknessSlider;

    // Rotation (floating inside viewport)
    juce::Label rotXLabel, rotYLabel, rotZLabel;
    juce::Label rotXValue, rotYValue, rotZValue;
    juce::TextButton resetRotationButton{"Reset"};

    // === BOTTOM: Piano ===
    PianoRoll pianoRoll;

    // === RIGHT COLUMN: Visualizers + Controls ===
    juce::Label spectrumLabel, oscilloscopeLabel;
    SpectrumDisplay spectrumDisplay;
    OscilloscopeDisplay oscilloscopeDisplay;
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
    std::unique_ptr<SliderAttachment> keyIntensityAttachment;
    std::unique_ptr<SliderAttachment> fillIntensityAttachment;
    std::unique_ptr<SliderAttachment> rimIntensityAttachment;
    std::unique_ptr<ComboBoxAttachment> envModeAttachment;

    // === Helpers ===
    void setupRotarySlider(juce::Slider& slider, double min, double max, double def);
    void setupLabel(juce::Label& label, const juce::String& text, float fontSize, bool bold = false);

    // Material data
    static constexpr int NUM_MATERIALS = 10;
    const juce::String materialNames[NUM_MATERIALS] = {
        "Diamond", "Water", "Amber", "Ruby", "Gold", "Emerald", "Amethyst", "Sapphire",
        "Copper", "Obsidian"
    };
    const juce::Colour materialColours[NUM_MATERIALS] = {
        juce::Colour(0xFFE8F4FF), juce::Colour(0xFF50C8E8), juce::Colour(0xFFFFBF00),
        juce::Colour(0xFFE0115F), juce::Colour(0xFFFFD700), juce::Colour(0xFF50C878),
        juce::Colour(0xFF9966CC), juce::Colour(0xFF0F52BA),
        juce::Colour(0xFFB87333), juce::Colour(0xFF1C1C1C)
    };

    // Section frame rectangles (right column, for paint())
    std::vector<juce::Rectangle<int>> sectionFrames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElementsAudioProcessorEditor)
};
