#!/usr/bin/env python3
"""Regenerate the per-material images used by docs/materials-and-geometry.md.

  python3 scripts/make_material_images.py --dump /path/to/probe_out

Produces, in docs/assets/images/materials/:
  <material>-sphere.png      the real Elements PBR sphere (real Shaders.h + HDR + per-material table), transparent bg
  <material>-wavetable.png   Bitwig-style stack of the material's 10 octave-band wavetables

Reference scene (also stated on the docs page): undeformed Sphere, Daylight, Thickness 0.5.
  - sphere render: Key + Fill lights at intensity 1.0 (like the plugin screenshots), camera tilt 25 / rotY -30.
  - wavetables: Key light only at intensity 0.5, taken from the engine dump (scene_m<i>_s1).

The wavetable dump comes from scripts/docs-tools/dump_probe.cpp (see its header); the sphere renderer is
scripts/docs-tools/render_spheres.cpp (macOS only: CGL offscreen + the real shaders). Needs numpy, Pillow, matplotlib.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TOOLS = os.path.join(HERE, "docs-tools")
OUT = os.path.join(ROOT, "docs", "assets", "images", "materials")
sys.path.insert(0, HERE)
import wavetable_stack as ws  # noqa: E402

NAMES = ["diamond", "water", "amber", "ruby", "gold", "emerald", "amethyst", "sapphire",
         "copper", "obsidian", "alexandrite", "malachite", "neodymium"]
ACCENT = ["#a8d8f0", "#7ec8e3", "#f5b942", "#e84b6a", "#d4a843", "#4ecb8d", "#b57bee",
          "#5b9ef5", "#cf7e46", "#6a7a8a", "#5b8a64", "#2e7d52", "#9070c8"]
RENDER_SIZE = 1200
FINAL_SIZE = 320


def build_sphere_renderer(workdir):
    editor_h = open(os.path.join(ROOT, "Source", "PluginEditor.h")).read()
    i = editor_h.index("static constexpr PBRMaterialProps pbrMaterials")
    j = editor_h.index("};", i) + 2
    table = editor_h[i:j].replace("static constexpr PBRMaterialProps pbrMaterials[NUM_MATERIALS]",
                                  "static const PBRMaterialProps pbrMaterials[13]")
    inc = ("struct PBRMaterialProps { float metallic, roughness, ior, transparency, sssStrength, sssRadius; "
           "float absorptionColor[3]; bool hasChromism; float albedoSecondary[3]; float bandingStrength, bandingFrequency; };\n"
           + table + "\n")
    open(os.path.join(TOOLS, "pbr_table.inc"), "w").write(inc)
    exe = os.path.join(workdir, "render_spheres")
    subprocess.check_call(["clang++", "-std=c++17", "-O1", "-Wno-deprecated-declarations",
                           os.path.join(TOOLS, "render_spheres.cpp"), "-framework", "OpenGL",
                           "-framework", "CoreFoundation", "-o", exe], cwd=TOOLS)
    return exe


def render_spheres(exe, workdir):
    outs = {}
    for bg in (0, 1):
        d = os.path.join(workdir, f"bg{bg}")
        os.makedirs(d, exist_ok=True)
        subprocess.check_call([exe, d, str(RENDER_SIZE), str(bg), "kf1", "1"], cwd=TOOLS)
        outs[bg] = d
    for m, name in enumerate(NAMES):
        rd = lambda d: np.fromfile(os.path.join(d, f"m{m:02d}.rgba"), dtype=np.uint8) \
            .reshape(RENDER_SIZE, RENDER_SIZE, 4)[::-1, :, :3].astype(np.float32) / 255
        black, white = rd(outs[0]), rd(outs[1])
        # Source-over compositing: on black = C*a, on white = C*a + (1-a)  ->  a = 1 - (white - black)
        a = np.clip(1 - (white - black).mean(axis=2), 0, 1)
        c = np.where(a[..., None] > 1e-3, black / np.maximum(a[..., None], 1e-3), 0).clip(0, 1)
        img = Image.fromarray((np.dstack([c, a]) * 255).astype(np.uint8), "RGBA")
        ys, xs = np.where(a > 0.02)
        pad = int(0.04 * RENDER_SIZE)
        cx, cy = (xs.min() + xs.max()) // 2, (ys.min() + ys.max()) // 2
        half = max(xs.max() - xs.min(), ys.max() - ys.min()) // 2 + pad
        img = img.crop((cx - half, cy - half, cx + half, cy + half)).resize((FINAL_SIZE, FINAL_SIZE), Image.LANCZOS)
        img.save(os.path.join(OUT, f"{name}-sphere.png"), optimize=True)


def render_wavetables(dump):
    for m, name in enumerate(NAMES):
        tab = np.fromfile(os.path.join(dump, f"tab_scene_m{m}_s1.f32"), dtype=np.float32).reshape(10, 2048)
        ws.render([tab[b] for b in range(10)], ACCENT[m], os.path.join(OUT, f"{name}-wavetable.png"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dump", required=True, help="directory written by dump_probe.cpp")
    ap.add_argument("--skip-spheres", action="store_true")
    args = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    render_wavetables(args.dump)
    if not args.skip_spheres:
        with tempfile.TemporaryDirectory() as wd:
            render_spheres(build_sphere_renderer(wd), wd)
    print("wrote", len(os.listdir(OUT)), "images to", OUT)


if __name__ == "__main__":
    main()
