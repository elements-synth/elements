// Dump probe for the docs images. NOT part of any build: ElementsTests is not under git, so this lives here.
// To use: back up ElementsTests/Tests/main.cpp, replace it with this file, build ElementsTests (Release),
// run it with an output directory, then restore main.cpp. See scripts/make_material_images.py.
// Output: manifest.json (spectra, loudness, average transmission per material/light) + tab_<id>.f32
// (10 band wavetables x 2048 floats) rendered by the REAL engine (SynthEngine + Physics).
#include <JuceHeader.h>
#include <fstream>
#include <iostream>
#include <sstream>
#define private public
#include "../../Elements/Source/SynthEngine.h"
#undef private

static std::string OUT;

static void configure(ElementsSynth& s, int mat, int src, float thickness, float deform)
{
    s.setGeometry(Geometry::Sphere);
    s.setLightEnabled(0, true);  s.setLightSource(0, src);  s.setLightIntensity(0, 0.5f);
    s.setLightEnabled(1, false); s.setLightEnabled(2, false);
    s.setMixAmount(0.0f);
    s.setThickness(thickness);
    s.deformAmountTarget = deform; s.deformAmount = deform;
    s.setMaterialA(mat);
    s.updateSpectrum();
    s.regenerateWavetables();
}

int main(int argc, char** argv)
{
    OUT = argc > 1 ? argv[1] : "/tmp/probe_out";
    const auto& mats = getMaterials();
    std::ostringstream js; js << "{\"entries\":[";
    bool first = true;
    // Reference scene: undeformed Sphere, Key light only (source 0 Sunset / 1 Daylight / 2 LED Cool), intensity 0.5, Thickness 0.5.
    for (int m = 0; m < (int)mats.size(); ++m)
        for (int src = 0; src < 3; ++src)
        {
            ElementsSynth s; s.prepareToPlay(44100.0, 512);
            configure(s, m, src, 0.5f, 0.0f);
            std::string id = "scene_m" + std::to_string(m) + "_s" + std::to_string(src);
            if (!first) js << ","; first = false;
            js << "{\"id\":\"" << id << "\",\"mat\":" << m << ",\"name\":\"" << mats[(size_t)m].name << "\",\"src\":" << src
               << ",\"avgTrans\":" << calculateAverageTransmission(mats[(size_t)m]) << ",\"ior\":" << mats[(size_t)m].refractiveIndex
               << ",\"amp\":" << s.spectralAmplitudeTarget << ",\"spectrum\":[";
            for (int w = 0; w < NUM_WAVELENGTHS; ++w) js << (w ? "," : "") << s.spectrumA[(size_t)w];
            js << "]}";
            std::ofstream f(OUT + "/tab_" + id + ".f32", std::ios::binary);
            for (int b = 0; b < NUM_WAVETABLE_BANDS; ++b)
                f.write(reinterpret_cast<const char*>(s.currentWavetablesA.bands[(size_t)b].data()), sizeof(float) * WAVETABLE_SIZE);
        }
    js << "]}";
    std::ofstream(OUT + "/manifest.json") << js.str();
    std::cout << "done\n";
    return 0;
}
