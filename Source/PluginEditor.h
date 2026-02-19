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

private:
    ElementsAudioProcessor& processor;

    int startOctave = 2;
    int numOctaves = 5;
    int currentNote = -1;

    int getNoteFromPosition(juce::Point<int> pos);
    juce::Rectangle<int> getKeyBounds(int note, bool isBlack);
    bool isBlackKey(int note);

    std::array<bool, 128> activeNotes{};
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

private:
    ElementsAudioProcessor& processor;
    int lightIndex;
    juce::String lightName;

    juce::ToggleButton enableButton;
    juce::ComboBox sourceCombo;
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

private:
    juce::Colour currentAccent { MaterialAccents::diamond };
    juce::Typeface::Ptr jbmRegular;
    juce::Typeface::Ptr jbmBold;
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

    // === TOOLBAR: Geometry + Material dropdowns ===
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

    // APVTS attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> thicknessAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> resonanceAttachment;
    std::unique_ptr<ComboBoxAttachment> filterTypeAttachment;

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

    // Volume
    juce::Label volumeLabel;
    juce::Slider volumeSlider;

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
