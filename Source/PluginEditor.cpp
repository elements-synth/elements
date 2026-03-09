/*
  ==============================================================================
    PluginEditor.cpp
    Elements - Complete Plugin GUI Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// stb_image for loading HDR environment maps
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
#include "stb_image.h"

// ==============================================================================
// VIEWPORT 3D
// ==============================================================================

Viewport3D::Viewport3D(ElementsAudioProcessor& p) : processor(p)
{
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(false);  // Don't repaint automatically
    startTimerHz(30);  // Reduced from 60Hz
}

Viewport3D::~Viewport3D()
{
    stopTimer();
    openGLContext.detach();
}

void Viewport3D::newOpenGLContextCreated()
{
    createVBOs();
    createEnvironmentMap();
    shaderReady = compileShader();
    if (!shaderReady)
        DBG("PBR shader compilation failed — falling back to fixed-function");
}

void Viewport3D::applyIncrementalRotation(float angleDeg, float axisX, float axisY, float axisZ)
{
    // Build a rotation matrix for 'angleDeg' around (axisX, axisY, axisZ)
    // and PRE-multiply it with the current rotation matrix.
    // Pre-multiplying means the new rotation is applied in the object's LOCAL space.
    float a = angleDeg * 3.14159265359f / 180.0f;
    float c = std::cos(a), s = std::sin(a), t = 1.0f - c;

    // Rotation matrix R (column-major)
    float R[16] = {
        t*axisX*axisX + c,         t*axisX*axisY + s*axisZ,   t*axisX*axisZ - s*axisY,  0,
        t*axisX*axisY - s*axisZ,   t*axisY*axisY + c,         t*axisY*axisZ + s*axisX,  0,
        t*axisX*axisZ + s*axisY,   t*axisY*axisZ - s*axisX,   t*axisZ*axisZ + c,        0,
        0,                          0,                          0,                        1
    };

    // result = rotationMatrix * R (post-multiply for local-space rotation)
    // M * R means R is applied first (in object's local frame), then M.
    float result[16];
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
        {
            result[col * 4 + row] = 0;
            for (int k = 0; k < 4; ++k)
                result[col * 4 + row] += rotationMatrix[k * 4 + row] * R[col * 4 + k];
        }

    std::memcpy(rotationMatrix, result, sizeof(rotationMatrix));
    ++rotVersion;
}

void Viewport3D::resetRotationMatrix()
{
    float identity[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    std::memcpy(rotationMatrix, identity, sizeof(rotationMatrix));

    // Also reset cumulative rotation values
    cumulativeRotX = 0.0f;
    cumulativeRotY = 0.0f;
    cumulativeRotZ = 0.0f;

    ++rotVersion;
}

void Viewport3D::extractEulerAngles(float& outX, float& outY, float& outZ) const
{
    // Extract Euler angles (XYZ order) from the rotation matrix.
    // rotationMatrix is column-major: element [col][row] = rotationMatrix[col*4 + row]
    // M = Rx * Ry * Rz
    // m[2][0] = -sin(Y)
    float m00 = rotationMatrix[0], m10 = rotationMatrix[4], m20 = rotationMatrix[8];
    float m01 = rotationMatrix[1], m11 = rotationMatrix[5], m21 = rotationMatrix[9];
    float m02 = rotationMatrix[2], m12 = rotationMatrix[6], m22 = rotationMatrix[10];

    const float RAD2DEG = 180.0f / 3.14159265359f;

    float sy = -m20;
    if (sy > 0.999f) sy = 0.999f;
    if (sy < -0.999f) sy = -0.999f;

    outY = std::asin(sy) * RAD2DEG;
    float cosY = std::cos(std::asin(sy));

    if (cosY > 0.001f)
    {
        outX = std::atan2(m21, m22) * RAD2DEG;
        outZ = std::atan2(m10, m00) * RAD2DEG;
    }
    else
    {
        // Gimbal lock
        outX = std::atan2(-m12, m11) * RAD2DEG;
        outZ = 0.0f;
    }
}

void Viewport3D::resetRotation()
{
    resetRotationMatrix();

    // Write zero rotation to APVTS
    auto setZero = [&](juce::RangedAudioParameter* param) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(0.0f));
        param->endChangeGesture();
    };
    setZero(processor.getRotationXParam());
    setZero(processor.getRotationYParam());
    setZero(processor.getRotationZParam());
}

void Viewport3D::setRotationFromEuler(float xDeg, float yDeg, float zDeg)
{
    // Rebuild rotation matrix from Euler angles (XYZ order): M = Rx * Ry * Rz
    const float D2R = 3.14159265359f / 180.0f;
    float cx = std::cos(xDeg * D2R), sx = std::sin(xDeg * D2R);
    float cy = std::cos(yDeg * D2R), sy = std::sin(yDeg * D2R);
    float cz = std::cos(zDeg * D2R), sz = std::sin(zDeg * D2R);

    // Column-major
    rotationMatrix[0]  = cy*cz;
    rotationMatrix[1]  = cx*sz + sx*sy*cz;
    rotationMatrix[2]  = sx*sz - cx*sy*cz;
    rotationMatrix[3]  = 0;
    rotationMatrix[4]  = -cy*sz;
    rotationMatrix[5]  = cx*cz - sx*sy*sz;
    rotationMatrix[6]  = sx*cz + cx*sy*sz;
    rotationMatrix[7]  = 0;
    rotationMatrix[8]  = sy;
    rotationMatrix[9]  = -sx*cy;
    rotationMatrix[10] = cx*cy;
    rotationMatrix[11] = 0;
    rotationMatrix[12] = 0; rotationMatrix[13] = 0; rotationMatrix[14] = 0; rotationMatrix[15] = 1;

    // Set cumulative rotation values to match
    cumulativeRotX = xDeg;
    cumulativeRotY = yDeg;
    cumulativeRotZ = zDeg;

    ++rotVersion;
}

void Viewport3D::renderOpenGL()
{
    using namespace juce::gl;

    const float desktopScale = static_cast<float>(openGLContext.getRenderingScale());
    glViewport(0, 0,
               static_cast<GLsizei>(getWidth() * desktopScale),
               static_cast<GLsizei>(getHeight() * desktopScale));

    // Gray background
    glClearColor(0.22f, 0.22f, 0.24f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // === LEGACY FIXED-FUNCTION: projection + camera setup ===
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = getWidth() / static_cast<float>(std::max(1, getHeight()));
    float fov = 45.0f;
    float nearP = 0.1f;
    float farP = 100.0f;
    float top = nearP * std::tan(fov * 3.14159f / 360.0f);
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    glFrustum(left, right, bottom, top, nearP, farP);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDist);
    glRotatef(cameraTilt, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraRotY, 0.0f, 1.0f, 0.0f);

    // Setup legacy lighting (for light indicators)
    setupLighting();

    // === FIXED-FUNCTION: Grid + Axes ===
    glDisable(GL_LIGHTING);
    drawGrid(4.0f, 12);
    drawAxes(2.0f);

    // === PBR SHADER: Geometry ===
    currentGeometry = processor.getGeometry();

    if (shaderReady)
    {
        renderGeometryPBR();
    }
    else
    {
        // Fallback: legacy fixed-function geometry
        glEnable(GL_LIGHTING);
        glMultMatrixf(rotationMatrix);
        glColor3f(materialColour.getFloatRed(),
                  materialColour.getFloatGreen(),
                  materialColour.getFloatBlue());

        // Inline legacy draw (kept as fallback)
        {
            using namespace juce::gl;
            float s = 0.5f;
            if (currentGeometry == Geometry::Cube)
            {
                glBegin(GL_QUADS);
                glNormal3f(0,0,1);  glVertex3f(-s,-s,s); glVertex3f(s,-s,s); glVertex3f(s,s,s); glVertex3f(-s,s,s);
                glNormal3f(0,0,-1); glVertex3f(s,-s,-s); glVertex3f(-s,-s,-s); glVertex3f(-s,s,-s); glVertex3f(s,s,-s);
                glNormal3f(0,1,0);  glVertex3f(-s,s,s); glVertex3f(s,s,s); glVertex3f(s,s,-s); glVertex3f(-s,s,-s);
                glNormal3f(0,-1,0); glVertex3f(-s,-s,-s); glVertex3f(s,-s,-s); glVertex3f(s,-s,s); glVertex3f(-s,-s,s);
                glNormal3f(1,0,0);  glVertex3f(s,-s,s); glVertex3f(s,-s,-s); glVertex3f(s,s,-s); glVertex3f(s,s,s);
                glNormal3f(-1,0,0); glVertex3f(-s,-s,-s); glVertex3f(-s,-s,s); glVertex3f(-s,s,s); glVertex3f(-s,s,-s);
                glEnd();
            }
        }

        // Undo model matrix for the rest
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -cameraDist);
        glRotatef(cameraTilt, 1.0f, 0.0f, 0.0f);
        glRotatef(cameraRotY, 0.0f, 1.0f, 0.0f);
    }

    // === FIXED-FUNCTION: Light indicators (world space) ===
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glMultMatrixf(rotationMatrix);
    {
        glPushMatrix();
        float inv[16] = {
            rotationMatrix[0], rotationMatrix[4], rotationMatrix[8],  0,
            rotationMatrix[1], rotationMatrix[5], rotationMatrix[9],  0,
            rotationMatrix[2], rotationMatrix[6], rotationMatrix[10], 0,
            0,                  0,                  0,                 1
        };
        glMultMatrixf(inv);
        drawLightIndicators();
        glPopMatrix();
    }
    glPopMatrix();

    // === FIXED-FUNCTION: Rotation gizmo ===
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glMultMatrixf(rotationMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRotationGizmo(1.4f);
    glDisable(GL_BLEND);
    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
}

void Viewport3D::openGLContextClosing()
{
    destroyVBOs();
    destroyEnvironmentMap();
    pbrShader.reset();
    shaderReady = false;
}

void Viewport3D::paint(juce::Graphics& g)
{
    // Fallback if OpenGL not available
    if (!openGLContext.isAttached())
    {
        g.fillAll(juce::Colour(0xFF0D0D14));
        g.setColour(juce::Colours::white);
        g.drawText("OpenGL not available", getLocalBounds(), juce::Justification::centred);
    }

    // Viewport hints (Houdini-style)
    auto bounds = getLocalBounds();
    g.setColour(ElementsColors::text.withAlpha(0.35f));
    g.setFont(juce::Font(10.5f));
    int hintY = bounds.getBottom() - 82;
    g.drawText("RMB  Orbit    Scroll  Zoom", 12, hintY, 250, 14,
               juce::Justification::centredLeft);
}

void Viewport3D::resized()
{
}

void Viewport3D::timerCallback()
{
    // Sync viewport rotation from APVTS when not dragging (DAW automation)
    if (!isDragging)
    {
        float apvtsX = processor.getRotationX();
        float apvtsY = processor.getRotationY();
        float apvtsZ = processor.getRotationZ();

        // Compare wrapped cumulative values with APVTS values
        auto wrap360 = [](float v) {
            v = std::fmod(v, 360.0f);
            return v < 0.0f ? v + 360.0f : v;
        };

        constexpr float epsilon = 0.05f;
        if (std::abs(apvtsX - wrap360(cumulativeRotX)) > epsilon ||
            std::abs(apvtsY - wrap360(cumulativeRotY)) > epsilon ||
            std::abs(apvtsZ - wrap360(cumulativeRotZ)) > epsilon)
        {
            setRotationFromEuler(apvtsX, apvtsY, apvtsZ);
            needsRepaint = true;
        }
    }

    // Check if anything changed that requires a repaint
    int matIndex = processor.getMaterial();
    Geometry geom = processor.getGeometry();

    // Check light changes (enabled, source, intensity)
    bool lightsChanged = false;
    for (int i = 0; i < 3; ++i)
    {
        bool enabled = processor.isLightEnabled(i);
        int source = processor.getLightSource(i);
        float intensity = processor.getSynth().getLightIntensity(i);
        if (enabled != lastLightEnabled[i] || source != lastLightSource[i] ||
            std::abs(intensity - lastLightIntensity[i]) > 0.001f)
        {
            lastLightEnabled[i] = enabled;
            lastLightSource[i] = source;
            lastLightIntensity[i] = intensity;
            lightsChanged = true;
        }
    }

    // Sync thickness from processor (for shader uniform only)
    float newThickness = processor.getThickness();
    if (std::abs(newThickness - currentThickness) > 0.01f)
    {
        currentThickness = newThickness;
        needsRepaint = true;
    }

    bool changed = (matIndex != lastMaterial) ||
                   (geom != lastGeometry) ||
                   (rotVersion != lastRotVersion) ||
                   lightsChanged ||
                   isDragging ||
                   needsRepaint;

    if (changed)
    {
        lastMaterial = matIndex;
        lastGeometry = geom;
        lastRotVersion = rotVersion;
        needsRepaint = false;

        // Update material color
        const juce::Colour colours[] = {
            juce::Colour(0xFFE8F4FF), juce::Colour(0xFF50C8E8), juce::Colour(0xFFFFBF00),
            juce::Colour(0xFFE0115F), juce::Colour(0xFFFFD700), juce::Colour(0xFF50C878),
            juce::Colour(0xFF9966CC), juce::Colour(0xFF0F52BA),
            juce::Colour(0xFFB87333), juce::Colour(0xFF1C1C1C)
        };
        if (matIndex >= 0 && matIndex < NUM_MATERIALS)
            materialColour = colours[matIndex];

        repaint();
    }
}

Viewport3D::DragAxis Viewport3D::hitTestGizmo(juce::Point<float> mousePos)
{
    // Project 3D ring points to 2D screen space using the same transforms as renderOpenGL
    // (rotation matrix + camera), then find which ring is closest to the mouse cursor.

    const float PI = 3.14159265359f;
    const float DEG2RAD = PI / 180.0f;
    const float gizmoRadius = 1.4f;  // Must match drawRotationGizmo radius
    const int numSamples = 64;

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());

    // Rebuild the projection matrix parameters (same as renderOpenGL)
    float aspect = w / std::max(1.0f, h);
    float fov = 45.0f;
    float nearP = 0.1f;
    float topP = nearP * std::tan(fov * PI / 360.0f);
    float rightP = topP * aspect;

    // Camera angles (use current orbit state)
    float camTiltX = cameraTilt * DEG2RAD;
    float camRotY = cameraRotY * DEG2RAD;
    float cCX = std::cos(camTiltX), sCX = std::sin(camTiltX);
    float cCY = std::cos(camRotY), sCY = std::sin(camRotY);

    // Project a 3D world point to 2D screen coordinates
    auto project = [&](float px, float py, float pz) -> juce::Point<float> {
        // Apply object rotation matrix (column-major)
        float x = rotationMatrix[0]*px + rotationMatrix[4]*py + rotationMatrix[8]*pz;
        float y = rotationMatrix[1]*px + rotationMatrix[5]*py + rotationMatrix[9]*pz;
        float z = rotationMatrix[2]*px + rotationMatrix[6]*py + rotationMatrix[10]*pz;

        // Apply camera Y rotation
        float tx = x * cCY + z * sCY;
        float tz = -x * sCY + z * cCY;
        x = tx; z = tz;

        // Apply camera X rotation (tilt)
        float ty = y * cCX - z * sCX;
        tz = y * sCX + z * cCX;
        y = ty; z = tz;

        // Translate (camera distance)
        z -= cameraDist;

        // Perspective projection
        if (z >= -nearP) return {-10000.0f, -10000.0f};
        float sx = (x * nearP / (-z)) / rightP;
        float sy = (y * nearP / (-z)) / topP;

        return { (sx * 0.5f + 0.5f) * w, (-sy * 0.5f + 0.5f) * h };
    };

    // For each ring, find minimum distance from mouse to projected ring points
    float minDist[3] = {1e9f, 1e9f, 1e9f};

    for (int i = 0; i < numSamples; ++i)
    {
        float angle = 2.0f * PI * i / numSamples;
        float ca = std::cos(angle);
        float sa = std::sin(angle);

        // X ring: circle in YZ plane
        auto pX = project(0.0f, gizmoRadius * ca, gizmoRadius * sa);
        float dX = mousePos.getDistanceFrom(pX);
        if (dX < minDist[0]) minDist[0] = dX;

        // Y ring: circle in XZ plane
        auto pY = project(gizmoRadius * ca, 0.0f, gizmoRadius * sa);
        float dY = mousePos.getDistanceFrom(pY);
        if (dY < minDist[1]) minDist[1] = dY;

        // Z ring: circle in XY plane
        auto pZ = project(gizmoRadius * ca, gizmoRadius * sa, 0.0f);
        float dZ = mousePos.getDistanceFrom(pZ);
        if (dZ < minDist[2]) minDist[2] = dZ;
    }

    // Find closest ring
    float tolerance = std::min(w, h) * 0.03f;  // ~3% of viewport size
    int best = -1;
    float bestDist = tolerance;

    for (int i = 0; i < 3; ++i)
    {
        if (minDist[i] < bestDist)
        {
            bestDist = minDist[i];
            best = i;
        }
    }

    if (best == 0) return DragAxis::X;
    if (best == 1) return DragAxis::Y;
    if (best == 2) return DragAxis::Z;
    return DragAxis::None;
}

void Viewport3D::mouseDown(const juce::MouseEvent& e)
{
    // Right-click: orbit camera
    if (e.mods.isRightButtonDown())
    {
        isOrbiting = true;
        lastOrbitPos = e.position;
        return;
    }

    // Left-click: gizmo rotation
    lockedAxis = hitTestGizmo(e.position);

    if (lockedAxis != DragAxis::None)
    {
        isDragging = true;
        lastMousePos = e.position;

        // Notify DAW that rotation gesture is starting (for automation recording)
        processor.getRotationXParam()->beginChangeGesture();
        processor.getRotationYParam()->beginChangeGesture();
        processor.getRotationZParam()->beginChangeGesture();
    }
}

void Viewport3D::mouseDrag(const juce::MouseEvent& e)
{
    // Orbit camera
    if (isOrbiting)
    {
        float dx = e.position.x - lastOrbitPos.x;
        float dy = e.position.y - lastOrbitPos.y;
        cameraRotY += dx * 0.4f;
        cameraTilt += dy * 0.4f;
        cameraTilt = juce::jlimit(-89.0f, 89.0f, cameraTilt);
        lastOrbitPos = e.position;
        needsRepaint = true;
        return;
    }

    if (!isDragging || lockedAxis == DragAxis::None) return;

    float deltaX = e.position.x - lastMousePos.x;
    float deltaY = e.position.y - lastMousePos.y;

    // Calculate rotation delta for this drag
    float rotationDelta = 0.0f;

    // Apply incremental rotation around the selected LOCAL axis
    if (lockedAxis == DragAxis::X)
    {
        rotationDelta = deltaY * dragSensitivity;
        applyIncrementalRotation(rotationDelta, 1.0f, 0.0f, 0.0f);
        cumulativeRotX += rotationDelta;
    }
    else if (lockedAxis == DragAxis::Y)
    {
        rotationDelta = deltaX * dragSensitivity;
        applyIncrementalRotation(rotationDelta, 0.0f, 1.0f, 0.0f);
        cumulativeRotY += rotationDelta;
    }
    else if (lockedAxis == DragAxis::Z)
    {
        rotationDelta = deltaX * dragSensitivity;
        applyIncrementalRotation(rotationDelta, 0.0f, 0.0f, 1.0f);
        cumulativeRotZ += rotationDelta;
    }

    // Write Euler angles to APVTS (wrapping to 0-360 range for DAW automation)
    auto wrap360 = [](float v) {
        v = std::fmod(v, 360.0f);
        return v < 0.0f ? v + 360.0f : v;
    };

    auto* pX = processor.getRotationXParam();
    auto* pY = processor.getRotationYParam();
    auto* pZ = processor.getRotationZParam();
    pX->setValueNotifyingHost(pX->convertTo0to1(wrap360(cumulativeRotX)));
    pY->setValueNotifyingHost(pY->convertTo0to1(wrap360(cumulativeRotY)));
    pZ->setValueNotifyingHost(pZ->convertTo0to1(wrap360(cumulativeRotZ)));

    lastMousePos = e.position;
}

void Viewport3D::mouseUp(const juce::MouseEvent&)
{
    if (isOrbiting)
    {
        isOrbiting = false;
        return;
    }

    if (isDragging)
    {
        // End gesture for all rotation axes
        processor.getRotationXParam()->endChangeGesture();
        processor.getRotationYParam()->endChangeGesture();
        processor.getRotationZParam()->endChangeGesture();
    }
    isDragging = false;
    lockedAxis = DragAxis::None;
}

void Viewport3D::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    cameraDist -= wheel.deltaY * 2.0f;
    cameraDist = juce::jlimit(2.5f, 20.0f, cameraDist);
    needsRepaint = true;
}

void Viewport3D::mouseMove(const juce::MouseEvent& e)
{
    DragAxis newHover = hitTestGizmo(e.position);
    if (newHover != hoveredAxis)
    {
        hoveredAxis = newHover;
        needsRepaint = true;
    }
}

// ==============================================================================
// PBR SHADER PIPELINE
// ==============================================================================

std::vector<Viewport3D::PBRVertex> Viewport3D::generateCubeVertices(float size)
{
    std::vector<PBRVertex> verts;
    verts.reserve(36);

    float s = size / 2.0f;

    struct Face { float nx, ny, nz; float v[4][3]; };
    Face faces[6] = {
        { 0, 0, 1,   {{-s,-s,s},{s,-s,s},{s,s,s},{-s,s,s}} },
        { 0, 0,-1,   {{ s,-s,-s},{-s,-s,-s},{-s,s,-s},{s,s,-s}} },
        { 0, 1, 0,   {{-s,s,s},{s,s,s},{s,s,-s},{-s,s,-s}} },
        { 0,-1, 0,   {{-s,-s,-s},{s,-s,-s},{s,-s,s},{-s,-s,s}} },
        { 1, 0, 0,   {{ s,-s,s},{s,-s,-s},{s,s,-s},{s,s,s}} },
        {-1, 0, 0,   {{-s,-s,-s},{-s,-s,s},{-s,s,s},{-s,s,-s}} },
    };

    for (auto& f : faces)
    {
        int indices[] = {0,1,2, 0,2,3};
        for (int idx : indices) {
            PBRVertex v;
            v.position[0] = f.v[idx][0]; v.position[1] = f.v[idx][1]; v.position[2] = f.v[idx][2];
            v.normal[0] = f.nx; v.normal[1] = f.ny; v.normal[2] = f.nz;
            verts.push_back(v);
        }
    }
    return verts;
}

std::vector<Viewport3D::PBRVertex> Viewport3D::generateSphereVertices(float radius, int segments)
{
    const float PI = 3.14159265359f;
    std::vector<PBRVertex> verts;
    verts.reserve(segments * segments * 6);

    for (int i = 0; i < segments; ++i)
    {
        float lat0 = PI * (-0.5f + static_cast<float>(i) / segments);
        float lat1 = PI * (-0.5f + static_cast<float>(i + 1) / segments);
        float y0 = std::sin(lat0), y1 = std::sin(lat1);
        float r0 = std::cos(lat0), r1 = std::cos(lat1);

        for (int j = 0; j < segments; ++j)
        {
            float lng0 = 2 * PI * static_cast<float>(j) / segments;
            float lng1 = 2 * PI * static_cast<float>(j + 1) / segments;
            float x0 = std::cos(lng0), z0 = std::sin(lng0);
            float x1 = std::cos(lng1), z1 = std::sin(lng1);

            PBRVertex v00 = {{ radius*x0*r0, radius*y0, radius*z0*r0 }, { x0*r0, y0, z0*r0 }};
            PBRVertex v10 = {{ radius*x1*r0, radius*y0, radius*z1*r0 }, { x1*r0, y0, z1*r0 }};
            PBRVertex v11 = {{ radius*x1*r1, radius*y1, radius*z1*r1 }, { x1*r1, y1, z1*r1 }};
            PBRVertex v01 = {{ radius*x0*r1, radius*y1, radius*z0*r1 }, { x0*r1, y1, z0*r1 }};

            verts.push_back(v00); verts.push_back(v10); verts.push_back(v11);
            verts.push_back(v00); verts.push_back(v11); verts.push_back(v01);
        }
    }
    return verts;
}

std::vector<Viewport3D::PBRVertex> Viewport3D::generateTorusVertices(float majorR, float minorR, int segments)
{
    const float PI = 3.14159265359f;
    int rings = segments, sides = segments;
    std::vector<PBRVertex> verts;
    verts.reserve(rings * sides * 6);

    for (int i = 0; i < rings; ++i)
    {
        float theta0 = 2 * PI * i / rings;
        float theta1 = 2 * PI * (i + 1) / rings;

        for (int j = 0; j < sides; ++j)
        {
            float phi0 = 2 * PI * j / sides;
            float phi1 = 2 * PI * (j + 1) / sides;

            auto torusPoint = [&](float t, float p, PBRVertex& v) {
                float ct = std::cos(t), st = std::sin(t);
                float cp = std::cos(p), sp = std::sin(p);
                v.position[0] = (majorR + minorR * cp) * ct;
                v.position[1] = minorR * sp;
                v.position[2] = (majorR + minorR * cp) * st;
                v.normal[0] = cp * ct;
                v.normal[1] = sp;
                v.normal[2] = cp * st;
            };

            PBRVertex v00, v10, v11, v01;
            torusPoint(theta0, phi0, v00);
            torusPoint(theta1, phi0, v10);
            torusPoint(theta1, phi1, v11);
            torusPoint(theta0, phi1, v01);

            verts.push_back(v00); verts.push_back(v10); verts.push_back(v11);
            verts.push_back(v00); verts.push_back(v11); verts.push_back(v01);
        }
    }
    return verts;
}

std::vector<Viewport3D::PBRVertex> Viewport3D::generateDodecahedronVertices(float radius)
{
    // Regular dodecahedron: 12 pentagonal faces, 20 vertices
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    const float invPhi = 1.0f / phi;

    // 20 vertices of a regular dodecahedron
    float rawVerts[20][3] = {
        { 1,  1,  1}, { 1,  1, -1}, { 1, -1,  1}, { 1, -1, -1},       // 0-3: cube
        {-1,  1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1, -1, -1},       // 4-7: cube
        {0,  phi,  invPhi}, {0,  phi, -invPhi},                         // 8-9: YZ plane
        {0, -phi,  invPhi}, {0, -phi, -invPhi},                         // 10-11: YZ plane
        { invPhi, 0,  phi}, {-invPhi, 0,  phi},                         // 12-13: XZ plane
        { invPhi, 0, -phi}, {-invPhi, 0, -phi},                         // 14-15: XZ plane
        { phi,  invPhi, 0}, { phi, -invPhi, 0},                         // 16-17: XY plane
        {-phi,  invPhi, 0}, {-phi, -invPhi, 0}                          // 18-19: XY plane
    };

    // Normalize raw vertices to unit sphere
    float rawNorm[20][3];
    for (int i = 0; i < 20; ++i)
    {
        float len = std::sqrt(rawVerts[i][0]*rawVerts[i][0] + rawVerts[i][1]*rawVerts[i][1] + rawVerts[i][2]*rawVerts[i][2]);
        rawNorm[i][0] = rawVerts[i][0] / len;
        rawNorm[i][1] = rawVerts[i][1] / len;
        rawNorm[i][2] = rawVerts[i][2] / len;
    }

    // 12 face normals (icosahedron vertices = dodecahedron face centers)
    float faceNormals[12][3] = {
        { 0,  1,  phi}, { 0, -1,  phi}, { 0,  1, -phi}, { 0, -1, -phi},
        { 1,  phi,  0}, {-1,  phi,  0}, { 1, -phi,  0}, {-1, -phi,  0},
        { phi,  0,  1}, {-phi,  0,  1}, { phi,  0, -1}, {-phi,  0, -1}
    };
    for (auto& n : faceNormals)
    {
        float len = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        n[0] /= len; n[1] /= len; n[2] /= len;
    }

    // Scale raw vertices to radius
    float scaledVerts[20][3];
    for (int i = 0; i < 20; ++i)
    {
        scaledVerts[i][0] = rawNorm[i][0] * radius;
        scaledVerts[i][1] = rawNorm[i][1] * radius;
        scaledVerts[i][2] = rawNorm[i][2] * radius;
    }

    std::vector<PBRVertex> verts;
    verts.reserve(12 * 3 * 3);

    for (int f = 0; f < 12; ++f)
    {
        float nx = faceNormals[f][0];
        float ny = faceNormals[f][1];
        float nz = faceNormals[f][2];

        // Find 5 vertices closest to this face normal
        struct VertDot { int idx; float dot; };
        VertDot dots[20];
        for (int i = 0; i < 20; ++i)
        {
            dots[i].idx = i;
            dots[i].dot = rawNorm[i][0]*faceNormals[f][0] + rawNorm[i][1]*faceNormals[f][1] + rawNorm[i][2]*faceNormals[f][2];
        }
        for (int i = 0; i < 19; ++i)
            for (int j = i + 1; j < 20; ++j)
                if (dots[j].dot > dots[i].dot)
                    std::swap(dots[i], dots[j]);

        float fv[5][3];
        for (int i = 0; i < 5; ++i)
        {
            fv[i][0] = scaledVerts[dots[i].idx][0];
            fv[i][1] = scaledVerts[dots[i].idx][1];
            fv[i][2] = scaledVerts[dots[i].idx][2];
        }

        // Sort vertices by angle around face normal for correct winding
        float fnx = faceNormals[f][0], fny = faceNormals[f][1], fnz = faceNormals[f][2];
        float tx, ty, tz;
        if (std::abs(fnx) < 0.9f)
            { tx = 0; ty = -fnz; tz = fny; }
        else
            { tx = fnz; ty = 0; tz = -fnx; }
        float tlen = std::sqrt(tx*tx + ty*ty + tz*tz);
        tx /= tlen; ty /= tlen; tz /= tlen;
        float bx = fny*tz - fnz*ty, by = fnz*tx - fnx*tz, bz = fnx*ty - fny*tx;

        float angles[5];
        for (int i = 0; i < 5; ++i)
            angles[i] = std::atan2(fv[i][0]*bx + fv[i][1]*by + fv[i][2]*bz,
                                   fv[i][0]*tx + fv[i][1]*ty + fv[i][2]*tz);

        int order[5] = {0, 1, 2, 3, 4};
        for (int i = 0; i < 4; ++i)
            for (int j = i + 1; j < 5; ++j)
                if (angles[order[j]] < angles[order[i]])
                    std::swap(order[i], order[j]);

        for (int i = 1; i < 4; ++i)
        {
            PBRVertex v0 = {{ fv[order[0]][0], fv[order[0]][1], fv[order[0]][2] }, { nx, ny, nz }};
            PBRVertex v1 = {{ fv[order[i]][0], fv[order[i]][1], fv[order[i]][2] }, { nx, ny, nz }};
            PBRVertex v2 = {{ fv[order[i+1]][0], fv[order[i+1]][1], fv[order[i+1]][2] }, { nx, ny, nz }};
            verts.push_back(v0); verts.push_back(v1); verts.push_back(v2);
        }
    }

    return verts;
}

void Viewport3D::createVBOs()
{
    using namespace juce::gl;

    auto cubeVerts   = generateCubeVertices(1.0f);
    auto sphereVerts = generateSphereVertices(0.8f, 32);
    auto torusVerts  = generateTorusVertices(0.6f, 0.25f, 32);
    auto dodecaVerts = generateDodecahedronVertices(0.85f);

    cubeVertexCount   = static_cast<int>(cubeVerts.size());
    sphereVertexCount = static_cast<int>(sphereVerts.size());
    torusVertexCount  = static_cast<int>(torusVerts.size());
    dodecaVertexCount = static_cast<int>(dodecaVerts.size());

    auto uploadVBO = [](GLuint& vbo, const void* data, size_t bytes) {
        using namespace juce::gl;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(bytes),
                     data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    };

    uploadVBO(cubeVBO, cubeVerts.data(), cubeVerts.size() * sizeof(PBRVertex));
    uploadVBO(sphereVBO, sphereVerts.data(), sphereVerts.size() * sizeof(PBRVertex));
    uploadVBO(torusVBO, torusVerts.data(), torusVerts.size() * sizeof(PBRVertex));
    uploadVBO(dodecaVBO, dodecaVerts.data(), dodecaVerts.size() * sizeof(PBRVertex));
}

void Viewport3D::destroyVBOs()
{
    using namespace juce::gl;
    if (cubeVBO)   { glDeleteBuffers(1, &cubeVBO);   cubeVBO = 0; }
    if (sphereVBO) { glDeleteBuffers(1, &sphereVBO); sphereVBO = 0; }
    if (torusVBO)  { glDeleteBuffers(1, &torusVBO);  torusVBO = 0; }
    if (dodecaVBO) { glDeleteBuffers(1, &dodecaVBO); dodecaVBO = 0; }
}

void Viewport3D::createEnvironmentMap()
{
    using namespace juce::gl;

    // Locate HDR file next to the plugin binary or in Source folder
    juce::File hdrFile;

    // Try several paths: next to executable, Source folder, project root
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File candidates[] = {
        exeDir.getChildFile("studio_kontrast_03_2k.hdr"),
        exeDir.getParentDirectory().getChildFile("studio_kontrast_03_2k.hdr"),
        juce::File("/Users/matiasderose/Documents/JUCE_Projects/Elements/studio_kontrast_03_2k.hdr"),
        juce::File("/Users/matiasderose/Documents/JUCE_Projects/Elements/Source/studio_kontrast_03_2k.hdr")
    };

    for (auto& f : candidates)
    {
        if (f.existsAsFile())
        {
            hdrFile = f;
            break;
        }
    }

    if (!hdrFile.existsAsFile())
    {
        DBG("Environment HDR not found — reflections will be black");
        return;
    }

    // Load HDR with stb_image (returns float RGB)
    int w, h, channels;
    float* hdrData = stbi_loadf(hdrFile.getFullPathName().toRawUTF8(), &w, &h, &channels, 3);
    if (!hdrData)
    {
        DBG("Failed to load HDR: " + juce::String(stbi_failure_reason()));
        return;
    }

    DBG("Loaded HDR environment map: " + juce::String(w) + "x" + juce::String(h));

    envWidth = w;
    envHeight = h;

    // Upload as GL_TEXTURE_2D with float data
    // On macOS OpenGL, GL_RGB16F is well-supported
    glGenTextures(1, &envTexture);
    glBindTexture(GL_TEXTURE_2D, envTexture);

    // Try GL_RGB16F for HDR; fall back to GL_RGB if not available
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0,
                 GL_RGB, GL_FLOAT, hdrData);

    // Check for errors — if GL_RGB16F failed, fall back to LDR
    if (glGetError() != GL_NO_ERROR)
    {
        // Convert to 8-bit with basic tonemap
        std::vector<unsigned char> ldrData(w * h * 3);
        for (int i = 0; i < w * h * 3; ++i)
        {
            // Reinhard tonemap + gamma
            float v = hdrData[i] / (hdrData[i] + 1.0f);
            v = std::pow(v, 1.0f / 2.2f);
            ldrData[i] = static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, v * 255.0f)));
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, ldrData.data());
        DBG("Using LDR fallback for environment map");
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Wrap horizontally
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Clamp vertically

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(hdrData);
}

void Viewport3D::destroyEnvironmentMap()
{
    using namespace juce::gl;
    if (envTexture != 0)
    {
        glDeleteTextures(1, &envTexture);
        envTexture = 0;
    }
}

bool Viewport3D::compileShader()
{
    pbrShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);

    if (!pbrShader->addVertexShader(pbrVertexShader))
    {
        DBG("Vertex shader error: " + pbrShader->getLastError());
        pbrShader.reset();
        return false;
    }
    if (!pbrShader->addFragmentShader(pbrFragmentShader))
    {
        DBG("Fragment shader error: " + pbrShader->getLastError());
        pbrShader.reset();
        return false;
    }
    if (!pbrShader->link())
    {
        DBG("Shader link error: " + pbrShader->getLastError());
        pbrShader.reset();
        return false;
    }

    return true;
}

void Viewport3D::buildProjectionMatrix(float* out, float fovDeg, float aspect, float nearP, float farP)
{
    float top = nearP * std::tan(fovDeg * 3.14159265359f / 360.0f);
    float right = top * aspect;

    // Column-major perspective matrix
    std::memset(out, 0, 16 * sizeof(float));
    out[0]  = nearP / right;
    out[5]  = nearP / top;
    out[10] = -(farP + nearP) / (farP - nearP);
    out[11] = -1.0f;
    out[14] = -(2.0f * farP * nearP) / (farP - nearP);
}

void Viewport3D::buildViewMatrix(float* out, float tiltDeg, float rotYDeg, float dist)
{
    const float D2R = 3.14159265359f / 180.0f;
    float cx = std::cos(tiltDeg * D2R), sx = std::sin(tiltDeg * D2R);
    float cy = std::cos(rotYDeg * D2R), sy = std::sin(rotYDeg * D2R);

    // View = Translate(0,0,-dist) * RotX(tilt) * RotY(rotY)
    // Build column-major
    // RotY:
    // cy  0  sy  0
    // 0   1  0   0
    // -sy 0  cy  0
    // 0   0  0   1
    //
    // RotX:
    // 1   0   0   0
    // 0   cx -sx  0
    // 0   sx  cx  0
    // 0   0   0   1
    //
    // Translate:
    // 1  0  0  0
    // 0  1  0  0
    // 0  0  1  -dist
    // 0  0  0  1
    //
    // Result = T * Rx * Ry (column-major)
    out[0]  = cy;           out[1]  = sx*sy;        out[2]  = -cx*sy;       out[3]  = 0;
    out[4]  = 0;            out[5]  = cx;           out[6]  = sx;           out[7]  = 0;
    out[8]  = sy;           out[9]  = -sx*cy;       out[10] = cx*cy;        out[11] = 0;
    out[12] = 0;            out[13] = 0;            out[14] = -dist;        out[15] = 1;
}

void Viewport3D::getCameraPosition(float* outPos, float tiltDeg, float rotYDeg, float dist)
{
    // Camera is at (0,0,dist) in view space, transformed by inverse of view rotation
    const float D2R = 3.14159265359f / 180.0f;
    float cx = std::cos(tiltDeg * D2R), sx = std::sin(tiltDeg * D2R);
    float cy = std::cos(rotYDeg * D2R), sy = std::sin(rotYDeg * D2R);

    // Inverse rotation applied to (0, 0, dist):
    // InvRotY * InvRotX * (0, 0, dist)
    // InvRotX * (0,0,dist) = (0, -sx*dist, cx*dist)
    // InvRotY * that = (sy*cx*dist, -sx*dist, cy*cx*dist)
    outPos[0] =  sy * cx * dist;
    outPos[1] = -sx * dist;
    outPos[2] =  cy * cx * dist;
}

void Viewport3D::renderGeometryPBR()
{
    using namespace juce::gl;

    // Build matrices
    float aspect = getWidth() / static_cast<float>(std::max(1, getHeight()));
    float projMatrix[16], viewMatrix[16];
    buildProjectionMatrix(projMatrix, 45.0f, aspect, 0.1f, 100.0f);
    buildViewMatrix(viewMatrix, cameraTilt, cameraRotY, cameraDist);

    float cameraPos[3];
    getCameraPosition(cameraPos, cameraTilt, cameraRotY, cameraDist);

    // Normal matrix = transpose(inverse(modelMatrix))
    // For orthogonal rotation matrix, inverse = transpose, so normalMatrix = rotationMatrix (3x3)
    float normalMatrix[9] = {
        rotationMatrix[0], rotationMatrix[1], rotationMatrix[2],
        rotationMatrix[4], rotationMatrix[5], rotationMatrix[6],
        rotationMatrix[8], rotationMatrix[9], rotationMatrix[10]
    };

    // Light colors
    const float lightColors[3][3] = {
        {1.0f, 0.55f, 0.15f},   // Sunset
        {1.0f, 1.0f, 0.85f},    // Daylight
        {0.4f, 0.7f, 1.0f}      // LED Cool
    };
    const float lightPositions[3][3] = {
        { keyLightPosition.x,  keyLightPosition.y,  keyLightPosition.z },
        { fillLightPosition.x, fillLightPosition.y, fillLightPosition.z },
        { rimLightPosition.x,  rimLightPosition.y,  rimLightPosition.z }
    };

    // Get PBR material properties
    int matIdx = processor.getMaterial();
    if (matIdx < 0 || matIdx >= NUM_MATERIALS) matIdx = 0;
    const auto& mat = pbrMaterials[matIdx];
    float albedo[3] = {
        materialColour.getFloatRed(),
        materialColour.getFloatGreen(),
        materialColour.getFloatBlue()
    };

    // Enable alpha blending for transparent materials
    bool isTransparent = mat.transparency > 0.0f;
    if (isTransparent)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Bind environment map (equirectangular 2D texture) to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, envTexture);

    // === Activate shader ===
    pbrShader->use();

    // Set uniforms
    auto setMat4 = [&](const char* name, const float* m) {
        auto loc = glGetUniformLocation(pbrShader->getProgramID(), name);
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m);
    };
    auto setMat3 = [&](const char* name, const float* m) {
        auto loc = glGetUniformLocation(pbrShader->getProgramID(), name);
        if (loc >= 0) glUniformMatrix3fv(loc, 1, GL_FALSE, m);
    };
    auto setVec3 = [&](const char* name, float x, float y, float z) {
        auto loc = glGetUniformLocation(pbrShader->getProgramID(), name);
        if (loc >= 0) glUniform3f(loc, x, y, z);
    };
    auto setFloat = [&](const char* name, float v) {
        auto loc = glGetUniformLocation(pbrShader->getProgramID(), name);
        if (loc >= 0) glUniform1f(loc, v);
    };
    auto setInt = [&](const char* name, int v) {
        auto loc = glGetUniformLocation(pbrShader->getProgramID(), name);
        if (loc >= 0) glUniform1i(loc, v);
    };

    setMat4("u_modelMatrix", rotationMatrix);
    setMat4("u_viewMatrix", viewMatrix);
    setMat4("u_projMatrix", projMatrix);
    setMat3("u_normalMatrix", normalMatrix);

    setVec3("u_albedo", albedo[0], albedo[1], albedo[2]);
    setFloat("u_metallic", mat.metallic);
    setFloat("u_roughness", mat.roughness);
    setVec3("u_cameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);

    // New material uniforms
    setFloat("u_ior", mat.ior);
    setFloat("u_transparency", mat.transparency);
    setFloat("u_sssStrength", mat.sssStrength);
    setFloat("u_sssRadius", mat.sssRadius);
    setVec3("u_absorptionColor", mat.absorptionColor[0], mat.absorptionColor[1], mat.absorptionColor[2]);
    setFloat("u_thickness", currentThickness);

    // Environment cubemap sampler
    setInt("u_envMap", 0);  // Texture unit 0

    // Set light uniforms
    for (int i = 0; i < 3; ++i)
    {
        bool enabled = processor.isLightEnabled(i);
        int sourceIdx = processor.getLightSource(i);
        const float* col = lightColors[sourceIdx];

        char buf[64];
        std::snprintf(buf, sizeof(buf), "u_lightPos[%d]", i);
        setVec3(buf, lightPositions[i][0], lightPositions[i][1], lightPositions[i][2]);

        std::snprintf(buf, sizeof(buf), "u_lightColor[%d]", i);
        setVec3(buf, col[0], col[1], col[2]);

        std::snprintf(buf, sizeof(buf), "u_lightEnabled[%d]", i);
        setInt(buf, enabled ? 1 : 0);

        std::snprintf(buf, sizeof(buf), "u_lightIntensity[%d]", i);
        setFloat(buf, processor.getSynth().getLightIntensity(i));
    }

    // Select VBO
    GLuint vbo = cubeVBO;
    int vertexCount = cubeVertexCount;
    switch (currentGeometry)
    {
        case Geometry::Cube:          vbo = cubeVBO;   vertexCount = cubeVertexCount;   break;
        case Geometry::Sphere:        vbo = sphereVBO; vertexCount = sphereVertexCount; break;
        case Geometry::Torus:         vbo = torusVBO;  vertexCount = torusVertexCount;  break;
        case Geometry::Dodecahedron:  vbo = dodecaVBO; vertexCount = dodecaVertexCount; break;
    }

    // Bind VBO and set vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    auto posAttr = glGetAttribLocation(pbrShader->getProgramID(), "a_position");
    auto normAttr = glGetAttribLocation(pbrShader->getProgramID(), "a_normal");

    if (posAttr >= 0)
    {
        glEnableVertexAttribArray(static_cast<GLuint>(posAttr));
        glVertexAttribPointer(static_cast<GLuint>(posAttr), 3, GL_FLOAT, GL_FALSE,
                              sizeof(PBRVertex), reinterpret_cast<void*>(offsetof(PBRVertex, position)));
    }
    if (normAttr >= 0)
    {
        glEnableVertexAttribArray(static_cast<GLuint>(normAttr));
        glVertexAttribPointer(static_cast<GLuint>(normAttr), 3, GL_FLOAT, GL_FALSE,
                              sizeof(PBRVertex), reinterpret_cast<void*>(offsetof(PBRVertex, normal)));
    }

    // Draw with proper face ordering for transparency
    glEnable(GL_CULL_FACE);

    if (isTransparent)
    {
        // Two-pass rendering for transparent geometry:
        // Pass 1: back faces (interior) with depth write off
        glCullFace(GL_FRONT);       // Cull front → draw back faces
        glDepthMask(GL_FALSE);      // Don't write depth (back faces shouldn't occlude)
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        // Pass 2: front faces (exterior) on top
        glCullFace(GL_BACK);        // Cull back → draw front faces
        glDepthMask(GL_TRUE);       // Restore depth write
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
    else
    {
        // Opaque: single pass, cull back faces
        glCullFace(GL_BACK);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glDisable(GL_CULL_FACE);

    // Cleanup
    if (posAttr >= 0) glDisableVertexAttribArray(static_cast<GLuint>(posAttr));
    if (normAttr >= 0) glDisableVertexAttribArray(static_cast<GLuint>(normAttr));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    // Unbind environment map and disable blend
    glBindTexture(GL_TEXTURE_2D, 0);
    if (isTransparent)
    {
        glDisable(GL_BLEND);
    }
}

void Viewport3D::drawRotationGizmo(float radius)
{
    using namespace juce::gl;
    const float PI = 3.14159265359f;
    int segments = 64;

    DragAxis activeAxis = isDragging ? lockedAxis : hoveredAxis;

    // X axis ring (Red) - rotates around X
    bool xActive = (activeAxis == DragAxis::X);
    glLineWidth(xActive ? 3.5f : 1.5f);
    glColor4f(0.9f, 0.2f, 0.2f, xActive ? 1.0f : 0.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * PI * i / segments;
        glVertex3f(0.0f, radius * std::cos(angle), radius * std::sin(angle));
    }
    glEnd();

    // Y axis ring (Green) - rotates around Y
    bool yActive = (activeAxis == DragAxis::Y);
    glLineWidth(yActive ? 3.5f : 1.5f);
    glColor4f(0.2f, 0.9f, 0.2f, yActive ? 1.0f : 0.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * PI * i / segments;
        glVertex3f(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
    }
    glEnd();

    // Z axis ring (Blue) - rotates around Z
    bool zActive = (activeAxis == DragAxis::Z);
    glLineWidth(zActive ? 3.5f : 1.5f);
    glColor4f(0.2f, 0.4f, 0.9f, zActive ? 1.0f : 0.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * PI * i / segments;
        glVertex3f(radius * std::cos(angle), radius * std::sin(angle), 0.0f);
    }
    glEnd();

    glLineWidth(1.0f);
}

void Viewport3D::drawLightIndicators()
{
    using namespace juce::gl;

    // Light source colors: Sunset, Daylight, LED Cool
    const float lightColors[3][3] = {
        {1.0f, 0.55f, 0.15f},  // Sunset (warm orange)
        {1.0f, 1.0f, 0.85f},   // Daylight (warm white)
        {0.4f, 0.7f, 1.0f}     // LED Cool (blue)
    };
    const float offColor[3] = {0.4f, 0.4f, 0.4f};
    const float baseColor[3] = {0.45f, 0.45f, 0.45f};  // Screw base (metallic gray)

    const float PI = 3.14159265359f;
    const int segs = 12;

    // Bulb dimensions
    const float bulbR = 0.1f;    // Glass bulb radius
    const float baseR = 0.065f;  // Screw base radius
    const float baseH = 0.09f;   // Screw base height
    const float rayLen = 0.18f;  // Ray line length

    Vec3 positions[3] = { keyLightPosition, fillLightPosition, rimLightPosition };

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int li = 0; li < 3; ++li)
    {
        bool enabled = processor.isLightEnabled(li);
        int sourceIdx = processor.getLightSource(li);
        float intensity = processor.getSynth().getLightIntensity(li);
        const float* baseCol = enabled ? lightColors[sourceIdx] : offColor;
        // Modulate bulb color by intensity (0 = dim, 1 = full brightness)
        float brightnessFactor = enabled ? (0.3f + 0.7f * intensity) : 1.0f;
        float col[3] = { baseCol[0] * brightnessFactor, baseCol[1] * brightnessFactor, baseCol[2] * brightnessFactor };
        float alpha = enabled ? (0.5f + 0.42f * intensity) : 0.35f;
        Vec3 pos = positions[li];

        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);

        // No orientation — bulb always points up (+Y)

        // === GLASS BULB (upper hemisphere) ===
        for (int lat = 0; lat < segs / 2; ++lat)
        {
            float t1 = PI * lat / segs;
            float t2 = PI * (lat + 1) / segs;
            float st1 = std::sin(t1), ct1 = std::cos(t1);
            float st2 = std::sin(t2), ct2 = std::cos(t2);

            glBegin(GL_TRIANGLE_STRIP);
            for (int lon = 0; lon <= segs; ++lon)
            {
                float phi = 2.0f * PI * lon / segs;
                float sp = std::sin(phi), cp = std::cos(phi);

                float shade1 = 0.6f + 0.4f * ct1;
                float shade2 = 0.6f + 0.4f * ct2;

                glColor4f(col[0] * shade1, col[1] * shade1, col[2] * shade1, alpha);
                glVertex3f(bulbR * st1 * cp, bulbR * ct1, bulbR * st1 * sp);

                glColor4f(col[0] * shade2, col[1] * shade2, col[2] * shade2, alpha);
                glVertex3f(bulbR * st2 * cp, bulbR * ct2, bulbR * st2 * sp);
            }
            glEnd();
        }

        // === SCREW BASE (small tapered cylinder below the bulb) ===
        glColor4f(baseColor[0], baseColor[1], baseColor[2], alpha);
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= segs; ++i)
        {
            float a = 2.0f * PI * i / segs;
            float cx = std::cos(a), cz = std::sin(a);
            glVertex3f(baseR * cx, 0.0f, baseR * cz);
            glVertex3f(baseR * 0.7f * cx, -baseH, baseR * 0.7f * cz);
        }
        glEnd();

        // Base bottom cap
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, -baseH, 0.0f);
        for (int i = segs; i >= 0; --i)
        {
            float a = 2.0f * PI * i / segs;
            glVertex3f(baseR * 0.7f * std::cos(a), -baseH, baseR * 0.7f * std::sin(a));
        }
        glEnd();

        // === EMIT RAYS (short lines radiating outward when ON) ===
        if (enabled)
        {
            float rayAlpha = 0.2f + 0.5f * intensity;
            float scaledRayLen = rayLen * (0.4f + 0.6f * intensity);
            glColor4f(col[0], col[1], col[2], rayAlpha);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            for (int i = 0; i < 8; ++i)
            {
                float a = 2.0f * PI * i / 8.0f;
                float rx = std::cos(a), rz = std::sin(a);
                glVertex3f(bulbR * rx, bulbR * 0.3f, bulbR * rz);
                glVertex3f((bulbR + scaledRayLen) * rx, bulbR * 0.3f, (bulbR + scaledRayLen) * rz);
            }
            glEnd();
            glLineWidth(1.0f);
        }

        glPopMatrix();

        // === ARROW from light toward origin ===
        {
            float dx = -pos.x, dy = -pos.y, dz = -pos.z;
            float dlen = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (dlen > 0.001f) { dx /= dlen; dy /= dlen; dz /= dlen; }

            // Arrow starts just below the bulb base, extends partway toward origin
            float arrowStart = 0.15f;  // offset from light position
            float arrowLen = 0.45f;    // length of the shaft
            float sx = pos.x + dx * arrowStart;
            float sy = pos.y + dy * arrowStart;
            float sz = pos.z + dz * arrowStart;
            float ex = pos.x + dx * (arrowStart + arrowLen);
            float ey = pos.y + dy * (arrowStart + arrowLen);
            float ez = pos.z + dz * (arrowStart + arrowLen);

            // Shaft
            glColor4f(col[0], col[1], col[2], enabled ? 0.7f : 0.2f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(sx, sy, sz);
            glVertex3f(ex, ey, ez);
            glEnd();

            // Arrowhead (small cone at the tip using lines)
            if (enabled)
            {
                float headLen = 0.08f;
                float headR = 0.04f;
                float tipX = ex + dx * headLen;
                float tipY = ey + dy * headLen;
                float tipZ = ez + dz * headLen;

                // Build perpendicular vectors for the cone base
                float perpX, perpY, perpZ;
                if (std::abs(dx) < 0.9f) { perpX = 0; perpY = -dz; perpZ = dy; }
                else                      { perpX = -dz; perpY = 0; perpZ = dx; }
                float plen = std::sqrt(perpX*perpX + perpY*perpY + perpZ*perpZ);
                if (plen > 0.001f) { perpX /= plen; perpY /= plen; perpZ /= plen; }

                float perp2X = dy*perpZ - dz*perpY;
                float perp2Y = dz*perpX - dx*perpZ;
                float perp2Z = dx*perpY - dy*perpX;

                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(tipX, tipY, tipZ);
                for (int i = 0; i <= 8; ++i)
                {
                    float a = 2.0f * PI * i / 8.0f;
                    float ca = std::cos(a), sa = std::sin(a);
                    glVertex3f(ex + headR * (perpX * ca + perp2X * sa),
                               ey + headR * (perpY * ca + perp2Y * sa),
                               ez + headR * (perpZ * ca + perp2Z * sa));
                }
                glEnd();
            }

            glLineWidth(1.0f);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void Viewport3D::setupLighting()
{
    using namespace juce::gl;

    // Light source colors (Sunset, Daylight, LED Cool)
    const GLfloat lightColors[3][4] = {
        {1.0f, 0.6f, 0.3f, 1.0f},   // Sunset - warm orange
        {1.0f, 1.0f, 0.95f, 1.0f},  // Daylight - neutral white
        {0.8f, 0.9f, 1.0f, 1.0f}    // LED Cool - blue-white
    };

    // 3-Point Lighting positions
    // Key light: upper-right-front
    GLfloat keyLightPos[] = { 2.5f, 2.2f, 2.5f, 1.0f };
    // Fill light: left side, softer
    GLfloat fillLightPos[] = { -1.6f, 0.9f, 1.2f, 1.0f };
    // Rim/Back light: behind and above (lowered 20%)
    GLfloat rimLightPos[] = { 0.0f, 2.0f, -2.5f, 1.0f };

    // Store positions for visualization (indicator spheres)
    keyLightPosition = {keyLightPos[0], keyLightPos[1], keyLightPos[2]};
    fillLightPosition = {fillLightPos[0], fillLightPos[1], fillLightPos[2]};
    rimLightPosition = {rimLightPos[0], rimLightPos[1], rimLightPos[2]};

    // Global ambient (very dim)
    GLfloat globalAmbient[] = { 0.1f, 0.1f, 0.12f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // === KEY LIGHT (GL_LIGHT0) ===
    if (processor.isLightEnabled(0))
    {
        glEnable(GL_LIGHT0);
        int sourceIdx = processor.getLightSource(0);
        const GLfloat* color = lightColors[sourceIdx];
        float ki = processor.getSynth().getLightIntensity(0);

        GLfloat keyAmbient[] = { color[0] * 0.1f * ki, color[1] * 0.1f * ki, color[2] * 0.1f * ki, 1.0f };
        GLfloat keyDiffuse[] = { color[0] * 0.9f * ki, color[1] * 0.9f * ki, color[2] * 0.9f * ki, 1.0f };
        GLfloat keySpecular[] = { color[0] * ki, color[1] * ki, color[2] * ki, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, keyLightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, keyAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, keyDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, keySpecular);
    }
    else
    {
        glDisable(GL_LIGHT0);
    }

    // === FILL LIGHT (GL_LIGHT1) ===
    if (processor.isLightEnabled(1))
    {
        glEnable(GL_LIGHT1);
        int sourceIdx = processor.getLightSource(1);
        const GLfloat* color = lightColors[sourceIdx];
        float fi = processor.getSynth().getLightIntensity(1);

        // Fill light is softer (lower intensity)
        GLfloat fillAmbient[] = { color[0] * 0.05f * fi, color[1] * 0.05f * fi, color[2] * 0.05f * fi, 1.0f };
        GLfloat fillDiffuse[] = { color[0] * 0.4f * fi, color[1] * 0.4f * fi, color[2] * 0.4f * fi, 1.0f };
        GLfloat fillSpecular[] = { color[0] * 0.2f * fi, color[1] * 0.2f * fi, color[2] * 0.2f * fi, 1.0f };

        glLightfv(GL_LIGHT1, GL_POSITION, fillLightPos);
        glLightfv(GL_LIGHT1, GL_AMBIENT, fillAmbient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiffuse);
        glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpecular);
    }
    else
    {
        glDisable(GL_LIGHT1);
    }

    // === RIM LIGHT (GL_LIGHT2) ===
    if (processor.isLightEnabled(2))
    {
        glEnable(GL_LIGHT2);
        int sourceIdx = processor.getLightSource(2);
        const GLfloat* color = lightColors[sourceIdx];
        float ri = processor.getSynth().getLightIntensity(2);

        // Rim light emphasizes edges
        GLfloat rimAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat rimDiffuse[] = { color[0] * 0.6f * ri, color[1] * 0.6f * ri, color[2] * 0.6f * ri, 1.0f };
        GLfloat rimSpecular[] = { color[0] * 0.8f * ri, color[1] * 0.8f * ri, color[2] * 0.8f * ri, 1.0f };

        glLightfv(GL_LIGHT2, GL_POSITION, rimLightPos);
        glLightfv(GL_LIGHT2, GL_AMBIENT, rimAmbient);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, rimDiffuse);
        glLightfv(GL_LIGHT2, GL_SPECULAR, rimSpecular);
    }
    else
    {
        glDisable(GL_LIGHT2);
    }

    // Material properties for better lighting response
    GLfloat matSpecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat matShininess[] = { 60.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);
}

void Viewport3D::drawGrid(float size, int divisions)
{
    using namespace juce::gl;

    float half = size / 2.0f;
    float step = size / divisions;

    glLineWidth(1.0f);
    glBegin(GL_LINES);

    // Grid lines (subtle gray)
    glColor4f(0.3f, 0.3f, 0.35f, 0.5f);

    for (int i = 0; i <= divisions; ++i)
    {
        float pos = -half + i * step;

        // Lines parallel to X axis
        glVertex3f(-half, 0.0f, pos);
        glVertex3f(half, 0.0f, pos);

        // Lines parallel to Z axis
        glVertex3f(pos, 0.0f, -half);
        glVertex3f(pos, 0.0f, half);
    }

    glEnd();
}

void Viewport3D::drawAxes(float length)
{
    using namespace juce::gl;

    glLineWidth(2.0f);
    glBegin(GL_LINES);

    // X axis - Red
    glColor3f(0.9f, 0.2f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(length, 0.0f, 0.0f);

    // Y axis - Green
    glColor3f(0.2f, 0.9f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, length, 0.0f);

    // Z axis - Blue
    glColor3f(0.2f, 0.4f, 0.9f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, length);

    glEnd();
    glLineWidth(1.0f);
}

// ==============================================================================
// SPECTRUM DISPLAY
// ==============================================================================

SpectrumDisplay::SpectrumDisplay(ElementsAudioProcessor& p) : processor(p)
{
    startTimerHz(30);
}

SpectrumDisplay::~SpectrumDisplay()
{
    stopTimer();
}

void SpectrumDisplay::timerCallback()
{
    repaint();
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xFF0D0D15));
    g.fillRoundedRectangle(bounds, 6.0f);

    const auto& spectrum = processor.getSpectrum();
    int numBands = static_cast<int>(spectrum.size());
    if (numBands == 0) return;

    float barWidth = bounds.getWidth() / numBands;
    float maxHeight = bounds.getHeight() - 8.0f;

    for (int i = 0; i < numBands; ++i)
    {
        float wavelength = 380.0f + (400.0f * i / numBands);
        float value = spectrum[static_cast<size_t>(i)];
        float barHeight = value * maxHeight;

        juce::Colour barColour = wavelengthToColour(wavelength);
        float x = bounds.getX() + i * barWidth;
        float y = bounds.getBottom() - barHeight - 4.0f;

        g.setColour(barColour.withAlpha(0.85f));
        g.fillRect(x + 0.5f, y, barWidth - 1.0f, barHeight);
    }

    g.setColour(juce::Colour(0xFF3A3A4A));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

juce::Colour SpectrumDisplay::wavelengthToColour(float wavelength)
{
    float r = 0, g = 0, b = 0;
    if (wavelength >= 380 && wavelength < 440) { r = -(wavelength - 440) / 60; b = 1; }
    else if (wavelength >= 440 && wavelength < 490) { g = (wavelength - 440) / 50; b = 1; }
    else if (wavelength >= 490 && wavelength < 510) { g = 1; b = -(wavelength - 510) / 20; }
    else if (wavelength >= 510 && wavelength < 580) { r = (wavelength - 510) / 70; g = 1; }
    else if (wavelength >= 580 && wavelength < 645) { r = 1; g = -(wavelength - 645) / 65; }
    else if (wavelength >= 645) { r = 1; }

    float factor = 1.0f;
    if (wavelength < 420) factor = 0.3f + 0.7f * (wavelength - 380) / 40;
    else if (wavelength > 700) factor = 0.3f + 0.7f * (780 - wavelength) / 80;

    return juce::Colour::fromFloatRGBA(r * factor, g * factor, b * factor, 1.0f);
}

// ==============================================================================
// OSCILLOSCOPE DISPLAY
// ==============================================================================

OscilloscopeDisplay::OscilloscopeDisplay(ElementsAudioProcessor& p) : processor(p)
{
    startTimerHz(30);  // Reduced from 60Hz
}

OscilloscopeDisplay::~OscilloscopeDisplay()
{
    stopTimer();
}

void OscilloscopeDisplay::timerCallback()
{
    repaint();
}

void OscilloscopeDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background
    g.setColour(juce::Colour(0xFF0D0D15));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Draw center line
    float centerY = bounds.getCentreY();
    g.setColour(juce::Colour(0xFF2A2A3A));
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX() + 4, bounds.getRight() - 4);

    // Get waveform buffer (circular — writePos is the oldest sample)
    const auto& buffer = processor.getOscilloscopeBuffer();
    int bufferSize = static_cast<int>(buffer.size());
    if (bufferSize == 0) return;
    int writePos = processor.getOscilloscopeWritePos();

    // Draw waveform with auto-scaling
    juce::Path waveformPath;
    float width = bounds.getWidth() - 8.0f;
    float height = bounds.getHeight() - 8.0f;
    float startX = bounds.getX() + 4.0f;

    // Find peak for auto-scaling
    float peak = 0.001f;
    for (int i = 0; i < bufferSize; ++i)
    {
        float absVal = std::abs(buffer[static_cast<size_t>(i)]);
        if (absVal > peak) peak = absVal;
    }

    float scale = (height * 0.45f) / peak;
    scale = std::min(scale, height * 5.0f);

    for (int i = 0; i < bufferSize; ++i)
    {
        int idx = (writePos + i) % bufferSize;  // read from oldest to newest
        float x = startX + (width * i / bufferSize);
        float sample = buffer[static_cast<size_t>(idx)];
        float y = centerY - (sample * scale);

        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }

    g.setColour(waveformColour);
    g.strokePath(waveformPath, juce::PathStrokeType(1.5f));

    // Border
    g.setColour(juce::Colour(0xFF3A3A4A));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void OscilloscopeDisplay::pushSample(float sample)
{
    waveformBuffer[static_cast<size_t>(writePosition)] = sample;
    writePosition = (writePosition + 1) % static_cast<int>(waveformBuffer.size());
}

// ==============================================================================
// ADSR DISPLAY
// ==============================================================================

void ADSRDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background (same as oscilloscope)
    g.setColour(juce::Colour(0xFF0D0D15));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Get ADSR values (amp or filter depending on mode)
    float attack  = filterMode ? processor.getFilterAttack()  : processor.getAttack();
    float decay   = filterMode ? processor.getFilterDecay()   : processor.getDecay();
    float sustain = filterMode ? processor.getFilterSustain() : processor.getSustain();
    float release = filterMode ? processor.getFilterRelease() : processor.getRelease();

    // Visual sustain hold duration (fixed for display)
    float sustainHold = 0.3f;
    float totalTime = attack + decay + sustainHold + release;
    if (totalTime < 0.001f) totalTime = 0.001f;

    // Drawing area with padding
    float pad = 6.0f;
    float drawX = bounds.getX() + pad;
    float drawW = bounds.getWidth() - pad * 2.0f;
    float drawY = bounds.getY() + pad;
    float drawH = bounds.getHeight() - pad * 2.0f;

    // Time to X coordinate
    auto timeToX = [&](float t) { return drawX + (t / totalTime) * drawW; };
    // Amplitude to Y coordinate (0=bottom, 1=top)
    auto ampToY = [&](float a) { return drawY + drawH * (1.0f - a); };

    // Key points of the ADSR curve
    float x0 = timeToX(0.0f);                          // Start
    float x1 = timeToX(attack);                         // End of attack
    float x2 = timeToX(attack + decay);                 // End of decay
    float x3 = timeToX(attack + decay + sustainHold);   // End of sustain
    float x4 = timeToX(totalTime);                      // End of release

    float yBottom = ampToY(0.0f);
    float yTop    = ampToY(1.0f);
    float ySus    = ampToY(sustain);

    // Sustain level reference line (dashed)
    g.setColour(juce::Colour(0xFF2A2A3A));
    float dashX = drawX;
    while (dashX < drawX + drawW)
    {
        float dashEnd = std::min(dashX + 4.0f, drawX + drawW);
        g.drawLine(dashX, ySus, dashEnd, ySus, 0.5f);
        dashX += 8.0f;
    }

    // ADSR curve
    juce::Path curvePath;
    curvePath.startNewSubPath(x0, yBottom);   // Start at 0
    curvePath.lineTo(x1, yTop);               // Attack: 0 → 1
    curvePath.lineTo(x2, ySus);               // Decay: 1 → sustain
    curvePath.lineTo(x3, ySus);               // Sustain hold
    curvePath.lineTo(x4, yBottom);            // Release: sustain → 0

    g.setColour(envelopeColour);
    g.strokePath(curvePath, juce::PathStrokeType(1.5f));

    // Filled area under curve (subtle)
    juce::Path fillPath(curvePath);
    fillPath.lineTo(x4, yBottom);
    fillPath.lineTo(x0, yBottom);
    fillPath.closeSubPath();
    g.setColour(envelopeColour.withAlpha(0.08f));
    g.fillPath(fillPath);

    // Border
    g.setColour(juce::Colour(0xFF3A3A4A));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

// ==============================================================================
// PIANO ROLL
// ==============================================================================

PianoRoll::PianoRoll(ElementsAudioProcessor& p) : processor(p)
{
    startTimerHz(30);
}

PianoRoll::~PianoRoll()
{
    stopTimer();
}

void PianoRoll::timerCallback()
{
    // Poll active voices from the synth engine (catches MIDI controller input)
    std::array<bool, 128> synthNotes{};
    processor.getSynth().getActiveNotes(synthNotes);

    // Merge: a note is active if the synth is playing it OR it's being mouse-clicked
    bool changed = false;
    for (int i = 0; i < 128; ++i)
    {
        bool shouldBeActive = synthNotes[static_cast<size_t>(i)] || (i == currentNote);
        if (shouldBeActive != activeNotes[static_cast<size_t>(i)])
        {
            activeNotes[static_cast<size_t>(i)] = shouldBeActive;
            changed = true;
        }
    }
    if (changed)
        repaint();
}

void PianoRoll::resized()
{
}

void PianoRoll::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour(juce::Colour(0xFF1A1A2A));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    int whiteKeyWidth = bounds.getWidth() / (numOctaves * 7);
    int blackKeyWidth = static_cast<int>(whiteKeyWidth * 0.6f);
    int blackKeyHeight = static_cast<int>(bounds.getHeight() * 0.6f);

    // Draw white keys
    int x = 0;
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int key = 0; key < 7; ++key)
        {
            int noteInOctave[] = {0, 2, 4, 5, 7, 9, 11};
            int note = (startOctave + oct) * 12 + noteInOctave[key];

            juce::Rectangle<int> keyRect(x, 0, whiteKeyWidth - 1, bounds.getHeight());

            bool isActive = (note < 128 && activeNotes[static_cast<size_t>(note)]);
            g.setColour(isActive ? highlightColour : juce::Colours::white);
            g.fillRect(keyRect);
            g.setColour(juce::Colour(0xFF2A2A3A));
            g.drawRect(keyRect);

            // Label C notes with octave number
            if (noteInOctave[key] == 0)
            {
                g.setColour(isActive ? juce::Colours::white : juce::Colour(0xFF888888));
                g.setFont(juce::Font(9.0f));
                g.drawText("C" + juce::String(startOctave + oct),
                           keyRect.withTrimmedTop(keyRect.getHeight() - 14),
                           juce::Justification::centred);
            }

            x += whiteKeyWidth;
        }
    }

    // Draw black keys
    x = 0;
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int key = 0; key < 7; ++key)
        {
            if (key != 2 && key != 6)  // Skip E-F and B-C gaps
            {
                int noteInOctave[] = {1, 3, -1, 6, 8, 10, -1};
                int note = (startOctave + oct) * 12 + noteInOctave[key];

                int bx = x + whiteKeyWidth - blackKeyWidth / 2;
                juce::Rectangle<int> keyRect(bx, 0, blackKeyWidth, blackKeyHeight);

                bool isActive = (note >= 0 && note < 128 && activeNotes[static_cast<size_t>(note)]);
                g.setColour(isActive ? highlightColour : juce::Colour(0xFF1A1A2A));
                g.fillRect(keyRect);
            }
            x += whiteKeyWidth;
        }
    }
}

void PianoRoll::mouseDown(const juce::MouseEvent& e)
{
    currentNote = getNoteFromPosition(e.getPosition());
    if (currentNote >= 0 && currentNote < 128)
        processor.getSynth().noteOn(currentNote, 0.8f);
    repaint();
}

void PianoRoll::mouseUp(const juce::MouseEvent&)
{
    if (currentNote >= 0 && currentNote < 128)
        processor.getSynth().noteOff(currentNote);
    currentNote = -1;
    repaint();
}

void PianoRoll::mouseDrag(const juce::MouseEvent& e)
{
    int newNote = getNoteFromPosition(e.getPosition());
    if (newNote != currentNote)
    {
        if (currentNote >= 0 && currentNote < 128)
            processor.getSynth().noteOff(currentNote);
        currentNote = newNote;
        if (currentNote >= 0 && currentNote < 128)
            processor.getSynth().noteOn(currentNote, 0.8f);
    }
    repaint();
}

int PianoRoll::getNoteFromPosition(juce::Point<int> pos)
{
    int whiteKeyWidth = getWidth() / (numOctaves * 7);
    int blackKeyWidth = static_cast<int>(whiteKeyWidth * 0.6f);
    int blackKeyHeight = static_cast<int>(getHeight() * 0.6f);

    // Check black keys first (they're on top)
    if (pos.y < blackKeyHeight)
    {
        int x = 0;
        for (int oct = 0; oct < numOctaves; ++oct)
        {
            for (int key = 0; key < 7; ++key)
            {
                if (key != 2 && key != 6)
                {
                    int bx = x + whiteKeyWidth - blackKeyWidth / 2;
                    if (pos.x >= bx && pos.x < bx + blackKeyWidth)
                    {
                        int noteInOctave[] = {1, 3, -1, 6, 8, 10, -1};
                        return (startOctave + oct) * 12 + noteInOctave[key];
                    }
                }
                x += whiteKeyWidth;
            }
        }
    }

    // Check white keys
    int whiteKeyIndex = pos.x / whiteKeyWidth;
    int octave = whiteKeyIndex / 7;
    int keyInOctave = whiteKeyIndex % 7;
    int noteInOctave[] = {0, 2, 4, 5, 7, 9, 11};
    return (startOctave + octave) * 12 + noteInOctave[keyInOctave];
}

bool PianoRoll::isBlackKey(int note)
{
    int n = note % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

juce::Rectangle<int> PianoRoll::getKeyBounds(int note, bool isBlack)
{
    juce::ignoreUnused(note, isBlack);
    return {};
}

// ==============================================================================
// LIGHT PANEL
// ==============================================================================

static juce::Colour getLightSourceColour(int id)
{
    switch (id)
    {
        case 1: return juce::Colour(0xFFE07830);  // Sunset — orange
        case 2: return juce::Colour(0xFFD4A843);  // Daylight — gold
        case 3: return juce::Colour(0xFF7EC8E3);  // LED Cool — light blue
        default: return ElementsColors::text;
    }
}

LightPanel::LightPanel(ElementsAudioProcessor& p, int idx, const juce::String& name)
    : processor(p), lightIndex(idx), lightName(name)
{
    enableButton.setButtonText(name);
    enableButton.setToggleState(idx == 0, juce::dontSendNotification);  // Key light on by default
    enableButton.addListener(this);
    addAndMakeVisible(enableButton);

    sourceCombo.addItem("Sunset", 1);
    sourceCombo.addItem("Daylight", 2);
    sourceCombo.addItem("LED Cool", 3);
    sourceCombo.setSelectedId(idx + 1);
    sourceCombo.setColour(juce::ComboBox::textColourId, getLightSourceColour(idx + 1));
    sourceCombo.addListener(this);
    addAndMakeVisible(sourceCombo);

    intensityLabel.setText("Intensity", juce::dontSendNotification);
    intensityLabel.setFont(juce::Font(11.0f));
    intensityLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAABBCC));
    intensityLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(intensityLabel);

    intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    intensitySlider.setRange(0.0, 1.0, 0.01);
    intensitySlider.setValue(0.5);
    intensitySlider.setDoubleClickReturnValue(true, 0.5);
    intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 18);
    intensitySlider.setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(intensitySlider);
}

LightPanel::~LightPanel() {}

void LightPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0x601c2534));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void LightPanel::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    auto topRow = bounds.removeFromTop(bounds.getHeight() / 2);
    auto bottomRow = bounds;

    // Top row: toggle + source combo
    enableButton.setBounds(topRow.removeFromLeft(56));
    topRow.removeFromLeft(2);
    sourceCombo.setBounds(topRow);

    // Bottom row: "Intensity" label + slider
    intensityLabel.setBounds(bottomRow.removeFromLeft(48));
    intensitySlider.setBounds(bottomRow);
}

void LightPanel::buttonClicked(juce::Button*)
{
    processor.setLightEnabled(lightIndex, enableButton.getToggleState());
}

void LightPanel::comboBoxChanged(juce::ComboBox*)
{
    processor.setLightSource(lightIndex, sourceCombo.getSelectedId() - 1);
    sourceCombo.setColour(juce::ComboBox::textColourId,
                          getLightSourceColour(sourceCombo.getSelectedId()));
}

void LightPanel::setEnabled(bool enabled)
{
    enableButton.setToggleState(enabled, juce::dontSendNotification);
}

// ==============================================================================
// LOOK AND FEEL
// ==============================================================================

void ElementsLookAndFeel::loadKnobFramesFromBinaryData()
{
    knobFramesOriginal.clear();
    knobFrames.clear();

    // Knob frames are named knob_00000_png .. knob_00063_png in BinaryData
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String name(BinaryData::namedResourceList[i]);
        if (! name.startsWith("knob_"))
            continue;

        int size = 0;
        const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
        if (data != nullptr && size > 0)
        {
            auto img = juce::ImageFileFormat::loadFrom(data, static_cast<size_t>(size));
            if (img.isValid())
                knobFramesOriginal.push_back(img);
        }
    }

    DBG("Loaded " + juce::String((int)knobFramesOriginal.size()) + " knob frames from BinaryData");
    rebuildTintedFrames();
}

void ElementsLookAndFeel::rebuildTintedFrames()
{
    knobFrames.clear();
    if (knobFramesOriginal.empty()) return;

    // Convert accent to normalised RGB for multiply blend
    float tR = currentAccent.getFloatRed();
    float tG = currentAccent.getFloatGreen();
    float tB = currentAccent.getFloatBlue();

    for (auto& src : knobFramesOriginal)
    {
        juce::Image tinted(juce::Image::ARGB, src.getWidth(), src.getHeight(), true);

        juce::Image::BitmapData srcData(src, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dstData(tinted, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < src.getHeight(); ++y)
        {
            for (int x = 0; x < src.getWidth(); ++x)
            {
                auto px = srcData.getPixelColour(x, y);
                // Luminance-based tint: preserve brightness, shift hue to accent
                float lum = px.getFloatRed() * 0.299f
                          + px.getFloatGreen() * 0.587f
                          + px.getFloatBlue() * 0.114f;

                // Mix: blend between original luminance (grayscale) and accent-tinted
                // This keeps highlights/shadows natural while colouring midtones
                float mixAmt = 0.6f; // tint strength
                float oR = lum * (1.0f - mixAmt) + lum * tR * mixAmt;
                float oG = lum * (1.0f - mixAmt) + lum * tG * mixAmt;
                float oB = lum * (1.0f - mixAmt) + lum * tB * mixAmt;

                dstData.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, oR),
                    juce::jlimit(0.0f, 1.0f, oG),
                    juce::jlimit(0.0f, 1.0f, oB),
                    px.getFloatAlpha()));
            }
        }

        knobFrames.push_back(tinted);
    }
}

ElementsLookAndFeel::ElementsLookAndFeel()
{
    jbmRegular = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
    jbmBold = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoBold_ttf, BinaryData::JetBrainsMonoBold_ttfSize);

    setColour(juce::Slider::thumbColourId, currentAccent);
    setColour(juce::Slider::rotarySliderFillColourId, currentAccent);
    setColour(juce::Slider::rotarySliderOutlineColourId, ElementsColors::bg2);
    setColour(juce::ComboBox::backgroundColourId, ElementsColors::bg3.withAlpha(0.6f));
    setColour(juce::ComboBox::textColourId, ElementsColors::text);
    setColour(juce::ComboBox::outlineColourId, ElementsColors::border.withAlpha(0.5f));
    setColour(juce::TextButton::buttonColourId, ElementsColors::bg3.withAlpha(0.6f));
    setColour(juce::Label::textColourId, ElementsColors::text);
    setColour(juce::ToggleButton::textColourId, ElementsColors::text);
    setColour(juce::ToggleButton::tickColourId, currentAccent);
    setColour(juce::PopupMenu::backgroundColourId, ElementsColors::bg2.withAlpha(0.9f));
    setColour(juce::PopupMenu::textColourId, ElementsColors::mid);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ElementsColors::bg3.withAlpha(0.9f));
    setColour(juce::PopupMenu::highlightedTextColourId, ElementsColors::text);
}

juce::Typeface::Ptr ElementsLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    if (font.isBold())
        return jbmBold;
    return jbmRegular;
}

void ElementsLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    auto knobSize = static_cast<float>(juce::jmin(width, height));
    auto centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    auto centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    auto posX = [&](float a, float r) { return centreX + r * std::sin(a); };
    auto posY = [&](float a, float r) { return centreY - r * std::cos(a); };

    // --- Arc track (full sweep) ---
    float arcR = knobSize * 0.46f;
    {
        juce::Path track;
        track.addCentredArc(centreX, centreY, arcR, arcR, 0,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(ElementsColors::border);
        g.strokePath(track, juce::PathStrokeType(2.0f));
    }

    // --- Progress arc with opacity gradient ---
    if (sliderPos > 0.001f)
    {
        const int numSegs = 40;
        for (int i = 0; i < numSegs; ++i)
        {
            float t0 = static_cast<float>(i) / numSegs;
            float t1 = static_cast<float>(i + 1) / numSegs;
            if (t0 >= sliderPos) break;
            if (t1 > sliderPos) t1 = sliderPos;

            float a0 = rotaryStartAngle + t0 * (rotaryEndAngle - rotaryStartAngle);
            float a1 = rotaryStartAngle + t1 * (rotaryEndAngle - rotaryStartAngle);

            float opacity = 0.08f + 0.92f * t1;

            juce::Path seg;
            seg.addCentredArc(centreX, centreY, arcR, arcR, 0, a0, a1, true);
            g.setColour(currentAccent.withAlpha(opacity));
            g.strokePath(seg, juce::PathStrokeType(2.0f));
        }
    }

    // --- Endpoint dot ---
    {
        float endX = posX(angle, arcR);
        float endY = posY(angle, arcR);
        g.setColour(currentAccent.withAlpha(0.25f));
        g.fillEllipse(endX - 4, endY - 4, 8, 8);
        g.setColour(juce::Colours::white);
        g.fillEllipse(endX - 2.0f, endY - 2.0f, 4, 4);
    }

    // --- Knob body: filmstrip frame or fallback ---
    float bodySize = knobSize * 0.82f;

    if (hasKnobFrames())
    {
        int frameIndex = juce::jlimit(0, (int)knobFramesOriginal.size() - 1,
                                       (int)(sliderPos * (knobFramesOriginal.size() - 1)));
        auto& frame = knobFramesOriginal[(size_t)frameIndex];

        g.drawImage(frame,
                    centreX - bodySize * 0.5f, centreY - bodySize * 0.5f,
                    bodySize, bodySize,
                    0, 0, frame.getWidth(), frame.getHeight());
    }
    else
    {
        // Fallback: simple filled circle when no filmstrip loaded
        g.setColour(ElementsColors::bg2);
        g.fillEllipse(centreX - bodySize * 0.5f, centreY - bodySize * 0.5f, bodySize, bodySize);

        g.setColour(ElementsColors::border.withAlpha(0.8f));
        g.drawEllipse(centreX - bodySize * 0.5f, centreY - bodySize * 0.5f, bodySize, bodySize, 0.8f);

        // Indicator line
        float r0 = bodySize * 0.15f;
        float r1 = bodySize * 0.42f;
        g.setColour(currentAccent.withAlpha(0.6f));
        g.drawLine(posX(angle, r0), posY(angle, r0),
                   posX(angle, r1), posY(angle, r1), 1.2f);
    }
}

void ElementsLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider&)
{
    bool isHorizontal = style == juce::Slider::LinearHorizontal;

    if (isHorizontal)
    {
        float trackY = static_cast<float>(y + height / 2);
        g.setColour(ElementsColors::bg2);
        g.fillRoundedRectangle(static_cast<float>(x), trackY - 1.5f, static_cast<float>(width), 3.0f, 1.5f);

        g.setColour(currentAccent);
        g.fillRoundedRectangle(static_cast<float>(x), trackY - 1.5f, sliderPos - x, 3.0f, 1.5f);

        // Thumb with glow
        g.setColour(currentAccent.withAlpha(0.2f));
        g.fillEllipse(sliderPos - 7, trackY - 7, 14.0f, 14.0f);
        g.setColour(ElementsColors::bg1);
        g.fillEllipse(sliderPos - 5, trackY - 5, 10.0f, 10.0f);
        g.setColour(currentAccent);
        g.drawEllipse(sliderPos - 5, trackY - 5, 10.0f, 10.0f, 1.2f);
    }
    else
    {
        float trackX = static_cast<float>(x + width / 2);
        g.setColour(ElementsColors::bg2);
        g.fillRoundedRectangle(trackX - 1.5f, static_cast<float>(y), 3.0f, static_cast<float>(height), 1.5f);

        g.setColour(currentAccent);
        g.fillRoundedRectangle(trackX - 1.5f, sliderPos, 3.0f, static_cast<float>(y + height) - sliderPos, 1.5f);

        g.setColour(currentAccent.withAlpha(0.2f));
        g.fillEllipse(trackX - 7, sliderPos - 7, 14.0f, 14.0f);
        g.setColour(ElementsColors::bg1);
        g.fillEllipse(trackX - 5, sliderPos - 5, 10.0f, 10.0f);
        g.setColour(currentAccent);
        g.drawEllipse(trackX - 5, sliderPos - 5, 10.0f, 10.0f, 1.2f);
    }
}

juce::Colour ElementsLookAndFeel::getColourForItemText(const juce::String& text) const
{
    // Materials
    if (text == "Diamond")   return MaterialAccents::diamond;
    if (text == "Water")     return MaterialAccents::water;
    if (text == "Amber")     return MaterialAccents::amber;
    if (text == "Ruby")      return MaterialAccents::ruby;
    if (text == "Gold")      return MaterialAccents::gold;
    if (text == "Emerald")   return MaterialAccents::emerald;
    if (text == "Amethyst")  return MaterialAccents::amethyst;
    if (text == "Sapphire")  return MaterialAccents::sapphire;
    if (text == "Copper")    return MaterialAccents::copper;
    if (text == "Obsidian")  return MaterialAccents::obsidian;
    // Light sources
    if (text == "Sunset")    return juce::Colour(0xFFE07830);
    if (text == "Daylight")  return juce::Colour(0xFFD4A843);
    if (text == "LED Cool")  return juce::Colour(0xFF7EC8E3);

    return ElementsColors::text;
}

void ElementsLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                            bool isSeparator, bool isActive, bool isHighlighted,
                                            bool isTicked, bool /*hasSubMenu*/,
                                            const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                            const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0).withHeight(1).withCentre(area.getCentre());
        g.setColour(ElementsColors::border);
        g.fillRect(r);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(ElementsColors::bg3.withAlpha(0.9f));
        g.fillRect(area);
    }

    auto itemColour = getColourForItemText(text);
    if (!isActive)
        itemColour = itemColour.withAlpha(0.3f);

    auto textArea = area;
    if (isTicked)
    {
        g.setColour(itemColour);
        auto tickArea = textArea.removeFromLeft(textArea.getHeight()).reduced(6);
        g.fillEllipse(tickArea.toFloat());
    }

    g.setColour(itemColour);
    g.setFont(juce::Font(13.0f));
    g.drawText(text, textArea.reduced(8, 0), juce::Justification::centredLeft);
}

// ==============================================================================
// MAIN EDITOR
// ==============================================================================

ElementsAudioProcessorEditor::ElementsAudioProcessorEditor(ElementsAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      viewport3D(p),
      pianoRoll(p),
      spectrumDisplay(p),
      oscilloscopeDisplay(p),
      adsrDisplay(p),
      filterAdsrDisplay(p, true)
{
    setLookAndFeel(&lookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    auto initAccent = MaterialAccents::getAccentForMaterial(audioProcessor.getMaterial());
    lookAndFeel.setAccent(initAccent);
    pianoRoll.setHighlightColour(initAccent);

    // Load custom knob frames from disk (test mode)
    lookAndFeel.loadKnobFramesFromBinaryData();

    // === TOOLBAR: Logo + Geometry + Material dropdowns + Help ===
    addAndMakeVisible(elementsLogo);

    helpButton.setColour(juce::TextButton::buttonColourId, ElementsColors::bg3);
    helpButton.setColour(juce::TextButton::textColourOffId, ElementsColors::mid);
    helpButton.addListener(this);
    addAndMakeVisible(helpButton);

    helpOverlay.setVisible(false);
    helpOverlay.onClose = [this]() { viewport3D.setVisible(true); };
    addChildComponent(helpOverlay);

    // Geometry + Material dropdowns (floating inside viewport)
    geoLabel.setText("GEO", juce::dontSendNotification);
    geoLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    geoLabel.setJustificationType(juce::Justification::centred);
    geoLabel.setColour(juce::Label::textColourId, ElementsColors::text);
    viewport3D.addAndMakeVisible(geoLabel);

    geoCombo.addItem("Cube", 1);
    geoCombo.addItem("Sphere", 2);
    geoCombo.addItem("Torus", 3);
    geoCombo.addItem("Dodeca", 4);
    geoCombo.setSelectedId(static_cast<int>(audioProcessor.getGeometry()) + 1, juce::dontSendNotification);
    geoCombo.addListener(this);
    viewport3D.addAndMakeVisible(geoCombo);

    matLabel.setText("MAT", juce::dontSendNotification);
    matLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    matLabel.setJustificationType(juce::Justification::centred);
    matLabel.setColour(juce::Label::textColourId, ElementsColors::text);
    viewport3D.addAndMakeVisible(matLabel);

    for (int i = 0; i < NUM_MATERIALS; ++i)
        matCombo.addItem(materialNames[i], i + 1);
    matCombo.setSelectedId(audioProcessor.getMaterial() + 1, juce::dontSendNotification);
    matCombo.addListener(this);
    matCombo.setColour(juce::ComboBox::textColourId,
                       MaterialAccents::getAccentForMaterial(audioProcessor.getMaterial()));
    viewport3D.addAndMakeVisible(matCombo);

    // Set initial visualizer colors to match material
    oscilloscopeDisplay.setWaveformColour(
        MaterialAccents::getAccentForMaterial(audioProcessor.getMaterial()));
    adsrDisplay.setEnvelopeColour(
        MaterialAccents::getAccentForMaterial(audioProcessor.getMaterial()));
    filterAdsrDisplay.setEnvelopeColour(
        MaterialAccents::getAccentForMaterial(audioProcessor.getMaterial()));

    // === Rotation fields (floating inside viewport, left column) ===
    auto setupRotLabel = [&](juce::Label& lbl, const juce::String& text, juce::Colour col) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(juce::Font(10.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, col);
        lbl.setJustificationType(juce::Justification::centredRight);
        viewport3D.addAndMakeVisible(lbl);
    };
    auto setupRotValue = [&](juce::Label& lbl) {
        lbl.setText("0.0", juce::dontSendNotification);
        lbl.setFont(juce::Font(11.0f));
        lbl.setColour(juce::Label::textColourId, ElementsColors::text);
        lbl.setColour(juce::Label::backgroundColourId, ElementsColors::bg3.withAlpha(0.5f));
        lbl.setColour(juce::Label::outlineColourId, ElementsColors::border.withAlpha(0.4f));
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setEditable(true);
        lbl.addListener(this);
        viewport3D.addAndMakeVisible(lbl);
    };

    setupRotLabel(rotXLabel, "rotX", juce::Colour(0xFFE03030));
    setupRotLabel(rotYLabel, "rotY", juce::Colour(0xFF30E030));
    setupRotLabel(rotZLabel, "rotZ", juce::Colour(0xFF3060E0));
    setupRotValue(rotXValue);
    setupRotValue(rotYValue);
    setupRotValue(rotZValue);

    resetRotationButton.setColour(juce::TextButton::buttonColourId, ElementsColors::bg3.withAlpha(0.5f));
    resetRotationButton.setColour(juce::TextButton::textColourOffId, ElementsColors::text);
    resetRotationButton.addListener(this);
    viewport3D.addAndMakeVisible(resetRotationButton);

    // === Lights + Thickness (floating inside viewport bottom) ===

    // Thickness — horizontal slider in viewport top bar
    thicknessLabel.setText("Thickness", juce::dontSendNotification);
    thicknessLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    thicknessLabel.setJustificationType(juce::Justification::centredLeft);
    thicknessLabel.setColour(juce::Label::textColourId, ElementsColors::text);
    viewport3D.addAndMakeVisible(thicknessLabel);

    thicknessSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thicknessSlider.setRange(0.1, 2.0, 0.01);
    thicknessSlider.setValue(0.5);
    thicknessSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thicknessSlider.addListener(this);
    viewport3D.addAndMakeVisible(thicknessSlider);
    thicknessAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thickness", thicknessSlider);

    lightsLabel.setText("LIGHTS", juce::dontSendNotification);
    lightsLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    lightsLabel.setJustificationType(juce::Justification::centred);
    lightsLabel.setColour(juce::Label::textColourId, ElementsColors::dim);
    viewport3D.addAndMakeVisible(lightsLabel);

    keyLightPanel = std::make_unique<LightPanel>(p, 0, "Key");
    fillLightPanel = std::make_unique<LightPanel>(p, 1, "Fill");
    rimLightPanel = std::make_unique<LightPanel>(p, 2, "Rim");
    viewport3D.addAndMakeVisible(keyLightPanel.get());
    viewport3D.addAndMakeVisible(fillLightPanel.get());
    viewport3D.addAndMakeVisible(rimLightPanel.get());

    // Light intensity APVTS attachments
    keyIntensityAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.apvts, "lightIntensityKey", keyLightPanel->getIntensitySlider());
    fillIntensityAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.apvts, "lightIntensityFill", fillLightPanel->getIntensitySlider());
    rimIntensityAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.apvts, "lightIntensityRim", rimLightPanel->getIntensitySlider());

    // === CENTER COLUMN: Viewport + Piano ===
    addAndMakeVisible(viewport3D);
    addAndMakeVisible(pianoRoll);

    // === RIGHT COLUMN: Spectrum + Oscilloscope + Controls ===
    setupLabel(spectrumLabel, "SPECTRUM", 13.0f, true);
    spectrumLabel.setColour(juce::Label::textColourId, ElementsColors::dim);
    setupLabel(oscilloscopeLabel, "OSCILLOSCOPE", 13.0f, true);
    oscilloscopeLabel.setColour(juce::Label::textColourId, ElementsColors::dim);
    addAndMakeVisible(spectrumDisplay);
    addAndMakeVisible(oscilloscopeDisplay);
    addAndMakeVisible(adsrDisplay);
    addAndMakeVisible(filterAdsrDisplay);

    setupLabel(filterLabel, "FILTER", 13.0f, true);
    filterBypassButton.setToggleState(true, juce::dontSendNotification);  // Filter ON by default
    filterBypassButton.addListener(this);
    addAndMakeVisible(filterBypassButton);
    setupLabel(filterCutoffLabel, "Cutoff", 10.0f);
    setupLabel(filterResonanceLabel, "Reso", 10.0f);
    setupRotarySlider(filterCutoffSlider, 20, 20000, 2000);
    setupRotarySlider(filterResonanceSlider, 0.5, 10, 1);
    filterTypeCombo.addItem("Lowpass", 1);
    filterTypeCombo.addItem("Highpass", 2);
    filterTypeCombo.addItem("Bandpass", 3);
    filterTypeCombo.setSelectedId(1);
    addAndMakeVisible(filterTypeCombo);

    // APVTS attachments: bidirectional sync between UI sliders and DAW-automatable params
    cutoffAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterCutoff", filterCutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterResonance", filterResonanceSlider);
    filterTypeAttachment = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "filterType", filterTypeCombo);

    // Filter Envelope (APVTS attachments)
    setupLabel(filterEnvLabel, "FILTER ENV", 13.0f, true);
    setupLabel(fAttackLabel, "A", 10.0f);
    setupLabel(fDecayLabel, "D", 10.0f);
    setupLabel(fSustainLabel, "S", 10.0f);
    setupLabel(fReleaseLabel, "R", 10.0f);
    setupRotarySlider(filterAttackSlider, 0.001, 2, 0.01);
    setupRotarySlider(filterDecaySlider, 0.001, 2, 0.3);
    setupRotarySlider(filterSustainSlider, 0, 1, 0.0);
    setupRotarySlider(filterReleaseSlider, 0.001, 2, 0.3);
    setupLabel(filterEnvAmountLabel, "Amt", 10.0f);
    setupRotarySlider(filterEnvAmountSlider, 0.0, 1.0, 0.0);

    filterAttackAttachment    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterAttack", filterAttackSlider);
    filterDecayAttachment     = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterDecay", filterDecaySlider);
    filterSustainAttachment   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterSustain", filterSustainSlider);
    filterReleaseAttachment   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterRelease", filterReleaseSlider);
    filterEnvAmountAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterEnvAmount", filterEnvAmountSlider);

    // Amplitude Envelope
    setupLabel(envelopeLabel, "AMP ENV", 13.0f, true);
    setupLabel(attackLabel, "A", 10.0f);
    setupLabel(decayLabel, "D", 10.0f);
    setupLabel(sustainLabel, "S", 10.0f);
    setupLabel(releaseLabel, "R", 10.0f);
    setupRotarySlider(attackSlider, 0.001, 2, 0.01);
    setupRotarySlider(decaySlider, 0.001, 2, 0.1);
    setupRotarySlider(sustainSlider, 0, 1, 0.7);
    setupRotarySlider(releaseSlider, 0.001, 2, 0.3);

    ampAttackAttachment  = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ampAttack", attackSlider);
    ampDecayAttachment   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ampDecay", decaySlider);
    ampSustainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ampSustain", sustainSlider);
    ampReleaseAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ampRelease", releaseSlider);

    // Envelope mode combo (Classic / Physical)
    envModeCombo.addItem("Classic", 1);
    envModeCombo.addItem("Physical", 2);
    envModeCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(envModeCombo);
    envModeAttachment = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "envMode", envModeCombo);

    setupLabel(volumeLabel, "VOLUME", 13.0f, true);
    setupRotarySlider(volumeSlider, 0, 1, 0.8);

    startTimerHz(30);
    setSize(1100, 770);
}

ElementsAudioProcessorEditor::~ElementsAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ElementsAudioProcessorEditor::timerCallback()
{
    // Sync combos from processor (for DAW automation changes)
    int geo = static_cast<int>(audioProcessor.getGeometry()) + 1;
    if (geoCombo.getSelectedId() != geo)
        geoCombo.setSelectedId(geo, juce::dontSendNotification);

    int mat = audioProcessor.getMaterial() + 1;
    if (matCombo.getSelectedId() != mat)
    {
        matCombo.setSelectedId(mat, juce::dontSendNotification);
        auto accent = MaterialAccents::getAccentForMaterial(mat - 1);
        lookAndFeel.setAccent(accent);
        matCombo.setColour(juce::ComboBox::textColourId, accent);
        oscilloscopeDisplay.setWaveformColour(accent);
        adsrDisplay.setEnvelopeColour(accent);
        filterAdsrDisplay.setEnvelopeColour(accent);
        repaint();
    }

    // Sync rotation value displays
    auto fmt = [](float v) { return juce::String(v, 1); };
    rotXValue.setText(fmt(audioProcessor.getRotationX()), juce::dontSendNotification);
    rotYValue.setText(fmt(audioProcessor.getRotationY()), juce::dontSendNotification);
    rotZValue.setText(fmt(audioProcessor.getRotationZ()), juce::dontSendNotification);
}

void ElementsAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, double min, double max, double def)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRange(min, max, 0.001);
    slider.setValue(def);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void ElementsAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text, float fontSize, bool bold)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(fontSize, bold ? juce::Font::bold : juce::Font::plain));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);
}

void ElementsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ElementsColors::bg0);

    // Subtitle below logo
    g.setColour(lookAndFeel.getAccent());
    g.setFont(juce::Font(11.0f));
    g.drawText("Spectral Synthesizer", elementsLogo.getX(), elementsLogo.getBottom() - 2,
               200, 14, juce::Justification::centredLeft);

    // Section frames (Microfreak-style rounded borders)
    for (auto& frame : sectionFrames)
    {
        g.setColour(ElementsColors::mid.withAlpha(0.35f));
        g.drawRoundedRectangle(frame.toFloat().reduced(0.5f), 4.0f, 1.17f);
    }
}

void ElementsAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    int pad = 8;

    // === BOTTOM: Piano Roll (full width) ===
    auto pianoArea = bounds.removeFromBottom(52);
    pianoRoll.setBounds(pianoArea);

    // === Header: Logo + subtitle + Help button ===
    elementsLogo.setBounds(14, 8, 220, 38);
    helpButton.setBounds(bounds.getWidth() - 14 - 28, 14, 28, 28);
    helpOverlay.setBounds(getLocalBounds());
    bounds.removeFromTop(56);

    // === RIGHT COLUMN: 310px ===
    auto rightCol = bounds.removeFromRight(310).reduced(pad);
    sectionFrames.clear();
    int envTotalW = rightCol.getWidth() * 3 / 4;
    int envMargin = (rightCol.getWidth() - envTotalW) / 2;

    // --- SPECTRUM ---
    spectrumLabel.setBounds(rightCol.removeFromTop(14));
    spectrumDisplay.setBounds(rightCol.removeFromTop(48));
    rightCol.removeFromTop(6);

    // --- OSCILLOSCOPE ---
    oscilloscopeLabel.setBounds(rightCol.removeFromTop(14));
    oscilloscopeDisplay.setBounds(rightCol.removeFromTop(48));
    rightCol.removeFromTop(8);

    // --- FILTER section (framed) ---
    int filterFrameTop = rightCol.getY();
    rightCol.removeFromTop(6); // inner padding top
    auto filterHeaderRow = rightCol.removeFromTop(20);
    filterLabel.setBounds(filterHeaderRow.removeFromLeft(60));
    filterBypassButton.setBounds(filterHeaderRow.removeFromRight(40));
    rightCol.removeFromTop(4);
    filterTypeCombo.setBounds(rightCol.removeFromTop(24).reduced(10, 0));
    rightCol.removeFromTop(4);
    auto filterRow = rightCol.removeFromTop(55);
    int filterKnobW = filterRow.getWidth() / 2;
    auto filterCut = filterRow.removeFromLeft(filterKnobW);
    auto filterRes = filterRow;
    filterCutoffLabel.setBounds(filterCut.removeFromTop(14));
    filterCutoffSlider.setBounds(filterCut);
    filterResonanceLabel.setBounds(filterRes.removeFromTop(14));
    filterResonanceSlider.setBounds(filterRes);

    // Filter Envelope (inside the FILTER frame)
    rightCol.removeFromTop(4);
    filterEnvLabel.setBounds(rightCol.removeFromTop(16));
    rightCol.removeFromTop(4);
    {
        auto fEnvRow = rightCol.removeFromTop(50);
        auto fEnvCenter = fEnvRow.withTrimmedLeft(envMargin).withTrimmedRight(envMargin);
        int fEnvKnobW = fEnvCenter.getWidth() / 4;
        auto fEnvA = fEnvCenter.removeFromLeft(fEnvKnobW);
        auto fEnvD = fEnvCenter.removeFromLeft(fEnvKnobW);
        auto fEnvS = fEnvCenter.removeFromLeft(fEnvKnobW);
        auto fEnvR = fEnvCenter;
        fAttackLabel.setBounds(fEnvA.removeFromTop(12));
        filterAttackSlider.setBounds(fEnvA);
        fDecayLabel.setBounds(fEnvD.removeFromTop(12));
        filterDecaySlider.setBounds(fEnvD);
        fSustainLabel.setBounds(fEnvS.removeFromTop(12));
        filterSustainSlider.setBounds(fEnvS);
        fReleaseLabel.setBounds(fEnvR.removeFromTop(12));
        filterReleaseSlider.setBounds(fEnvR);
    }
    rightCol.removeFromTop(2);
    filterEnvAmountLabel.setBounds(rightCol.removeFromTop(14));
    rightCol.removeFromTop(2);
    filterEnvAmountSlider.setBounds(rightCol.removeFromTop(49).reduced(30, 0));
    rightCol.removeFromTop(2);
    // Filter ADSR display — same 75% centered width as knobs
    {
        auto fAdsrRow = rightCol.removeFromTop(34);
        filterAdsrDisplay.setBounds(fAdsrRow.withTrimmedLeft(envMargin).withTrimmedRight(envMargin));
    }
    rightCol.removeFromTop(6); // inner padding bottom
    int filterFrameBottom = rightCol.getY();
    sectionFrames.push_back({rightCol.getX(), filterFrameTop, rightCol.getWidth(), filterFrameBottom - filterFrameTop});
    rightCol.removeFromTop(8);

    // --- AMP ENV section (framed) ---
    int ampFrameTop = rightCol.getY();
    rightCol.removeFromTop(6); // inner padding top
    {
        auto ampHeaderRow = rightCol.removeFromTop(16);
        envelopeLabel.setBounds(ampHeaderRow.removeFromLeft(80));
        envModeCombo.setBounds(ampHeaderRow.removeFromRight(90));
    }
    rightCol.removeFromTop(4);
    {
        auto envRow = rightCol.removeFromTop(50);
        auto envCenter = envRow.withTrimmedLeft(envMargin).withTrimmedRight(envMargin);
        int envKnobW = envCenter.getWidth() / 4;
        auto envA = envCenter.removeFromLeft(envKnobW);
        auto envD = envCenter.removeFromLeft(envKnobW);
        auto envS = envCenter.removeFromLeft(envKnobW);
        auto envR = envCenter;
        attackLabel.setBounds(envA.removeFromTop(12));
        attackSlider.setBounds(envA);
        decayLabel.setBounds(envD.removeFromTop(12));
        decaySlider.setBounds(envD);
        sustainLabel.setBounds(envS.removeFromTop(12));
        sustainSlider.setBounds(envS);
        releaseLabel.setBounds(envR.removeFromTop(12));
        releaseSlider.setBounds(envR);
    }
    rightCol.removeFromTop(4);
    // Amp ADSR display — same 75% centered width as knobs
    {
        auto aAdsrRow = rightCol.removeFromTop(34);
        adsrDisplay.setBounds(aAdsrRow.withTrimmedLeft(envMargin).withTrimmedRight(envMargin));
    }
    rightCol.removeFromTop(6); // inner padding bottom
    int ampFrameBottom = rightCol.getY();
    sectionFrames.push_back({rightCol.getX(), ampFrameTop, rightCol.getWidth(), ampFrameBottom - ampFrameTop});
    rightCol.removeFromTop(8);

    // --- OUTPUT (framed) ---
    int volFrameTop = rightCol.getY();
    rightCol.removeFromTop(6);
    volumeLabel.setBounds(rightCol.removeFromTop(16));
    rightCol.removeFromTop(3);
    volumeSlider.setBounds(rightCol.removeFromTop(80).reduced(30, 0));
    rightCol.removeFromTop(6);
    int volFrameBottom = rightCol.getY();
    sectionFrames.push_back({rightCol.getX(), volFrameTop, rightCol.getWidth(), volFrameBottom - volFrameTop});

    // === LEFT COLUMN: Viewport fills everything ===
    auto leftCol = bounds.reduced(pad);
    viewport3D.setBounds(leftCol);

    // Position floating UI inside viewport (local coordinates)
    int vpW = viewport3D.getWidth();
    int vpH = viewport3D.getHeight();

    // --- Top bar: Geo (left) + Mat (center) + Thickness (right) ---
    int comboY = 8;
    int comboH = 24;

    // Geometry — left
    geoLabel.setBounds(8, comboY, 30, comboH);
    geoCombo.setBounds(40, comboY, 110, comboH);

    // Material — center
    int matX = vpW / 2 - 75;
    matLabel.setBounds(matX, comboY, 30, comboH);
    matCombo.setBounds(matX + 32, comboY, 120, comboH);

    // Thickness — right
    int thkSliderW = 100;
    int thkLabelW = 58;
    thicknessLabel.setBounds(vpW - 8 - thkSliderW - thkLabelW, comboY, thkLabelW, comboH);
    thicknessSlider.setBounds(vpW - 8 - thkSliderW, comboY + 2, thkSliderW, comboH - 4);

    // --- Left side: Rotation X Y Z (vertical stack) + Reset ---
    int rotFieldW = 44;
    int rotLabelW = 28;
    int rotH = 20;
    int rotLx = 8;
    int ry = 44;

    rotXLabel.setBounds(rotLx, ry, rotLabelW, rotH);
    rotXValue.setBounds(rotLx + rotLabelW + 2, ry, rotFieldW, rotH);
    ry += rotH + 3;
    rotYLabel.setBounds(rotLx, ry, rotLabelW, rotH);
    rotYValue.setBounds(rotLx + rotLabelW + 2, ry, rotFieldW, rotH);
    ry += rotH + 3;
    rotZLabel.setBounds(rotLx, ry, rotLabelW, rotH);
    rotZValue.setBounds(rotLx + rotLabelW + 2, ry, rotFieldW, rotH);
    ry += rotH + 4;
    resetRotationButton.setBounds(rotLx, ry, rotLabelW + 2 + rotFieldW, rotH);

    // --- Bottom: Lights bar (two-row panels: toggle+combo / intensity slider) ---
    int barH = 64;
    int barY = vpH - barH - 6;
    int lx = 8;

    lightsLabel.setBounds(lx, barY, 46, barH);
    lx += 48;

    int lightW = (vpW - lx - 8) / 3;
    keyLightPanel->setBounds(lx, barY + 4, lightW - 4, barH - 8);
    lx += lightW;
    fillLightPanel->setBounds(lx, barY + 4, lightW - 4, barH - 8);
    lx += lightW;
    rimLightPanel->setBounds(lx, barY + 4, lightW - 4, barH - 8);
}

void ElementsAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    // Most sliders are handled by APVTS attachments (DAW-automatable via processBlock).
    // Only volume remains manual (not exposed as DAW parameter).
    if (slider == &volumeSlider)
        audioProcessor.setVolume(static_cast<float>(slider->getValue()));
}

void ElementsAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &filterBypassButton)
    {
        audioProcessor.setFilterEnabled(filterBypassButton.getToggleState());
    }
    else if (button == &resetRotationButton)
    {
        viewport3D.resetRotation();
    }
    else if (button == &helpButton)
    {
        bool show = !helpOverlay.isVisible();
        helpOverlay.setVisible(show);
        viewport3D.setVisible(!show);
        if (show)
        {
            helpOverlay.toFront(true);
        }
    }
}

void ElementsAudioProcessorEditor::comboBoxChanged(juce::ComboBox* combo)
{
    if (combo == &geoCombo)
    {
        int id = geoCombo.getSelectedId();  // 1=Sphere, 2=Cube, 3=Torus, 4=Dodeca
        audioProcessor.setGeometry(static_cast<Geometry>(id - 1));
    }
    else if (combo == &matCombo)
    {
        int matIndex = matCombo.getSelectedId() - 1;
        audioProcessor.setMaterial(matIndex);
        auto accent = MaterialAccents::getAccentForMaterial(matIndex);
        lookAndFeel.setAccent(accent);
        matCombo.setColour(juce::ComboBox::textColourId, accent);
        oscilloscopeDisplay.setWaveformColour(accent);
        adsrDisplay.setEnvelopeColour(accent);
        filterAdsrDisplay.setEnvelopeColour(accent);
        pianoRoll.setHighlightColour(accent);
        repaint();
    }
    // Filter type is handled by APVTS attachment (DAW-automatable)
}

void ElementsAudioProcessorEditor::labelTextChanged(juce::Label* label)
{
    float val = label->getText().getFloatValue();
    val = std::fmod(val, 360.0f);
    if (val < 0.0f) val += 360.0f;

    auto writeParam = [](juce::RangedAudioParameter* param, float v) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(v));
        param->endChangeGesture();
    };

    if (label == &rotXValue)
        writeParam(audioProcessor.getRotationXParam(), val);
    else if (label == &rotYValue)
        writeParam(audioProcessor.getRotationYParam(), val);
    else if (label == &rotZValue)
        writeParam(audioProcessor.getRotationZParam(), val);
    else
        return;

    viewport3D.setRotationFromEuler(audioProcessor.getRotationX(),
                                     audioProcessor.getRotationY(),
                                     audioProcessor.getRotationZ());
}

