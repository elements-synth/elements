#!/usr/bin/env python3
"""Elements demo-video template: intro card (logo, version, deformed-sphere render, title, subtitle)
+ the screen capture + picture and sound fade-out at the end. macOS only (renders the sphere with CGL).

  python3 scripts/video/make_demo_video.py INPUT.mp4 OUTPUT.mp4 --title "The Deformer" \\
      --subtitle "The Deformer as a sound shaping tool. Noise types + wavefolding" \\
      [--material 12] [--thickness 0.36] [--deform 1] [--noise 0] [--freq 2.8] [--time 3] [--no-sphere] \\
      [--fade-start 50] [--intro 3.5] [--version v1.0.0]

--material: index in the plugin (0 Diamond, 1 Water, 2 Amber, 3 Ruby, 4 Gold, 5 Emerald, 6 Amethyst, 7 Sapphire,
            8 Copper, 9 Obsidian, 10 Alexandrite, 11 Malachite, 12 Neodymium). --noise: 0 Simplex, 1 Alligator, 2 Worley.
--time: the animation moment of the noise field (pick one you like; look at --keep-dir output).
Template defaults: 3.5 s intro with 0.7 s fades, silent; fade-out of image AND sound from --fade-start to the end.
"""
import argparse, os, subprocess, sys, tempfile
import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TOOLS = os.path.join(ROOT, "scripts", "docs-tools")
SIZE = 700

def build_renderer(d):
    exe = os.path.join(d, "render_deformed")
    subprocess.check_call(["clang++", "-std=c++17", "-O1", "-Wno-deprecated-declarations",
                           "render_deformed.cpp", os.path.join(ROOT, "Source", "Physics.cpp"), "-I" + os.path.join(ROOT, "Source"),
                           "-framework", "OpenGL", "-framework", "CoreFoundation", "-o", exe], cwd=TOOLS)
    return exe

def render_sphere(a, d, out_png):
    exe = build_renderer(d)
    env = dict(os.environ, DEFORM=str(a.deform), FREQ=str(a.freq), NOISE=str(a.noise), TIME=str(a.time), THICK=str(a.thickness))
    raw = {}
    for bg in (0, 1):   # black + white backgrounds recover the true alpha of the glass materials
        od = os.path.join(d, f"bg{bg}"); os.makedirs(od, exist_ok=True)
        subprocess.check_call([exe, od, str(SIZE), str(bg), "kf1", "1", "5.8", "1", str(a.material)], cwd=TOOLS, env=env, stderr=subprocess.DEVNULL)
        raw[bg] = np.fromfile(os.path.join(od, f"m{a.material:02d}.rgba"), np.uint8).reshape(SIZE, SIZE, 4)[::-1, :, :3].astype(float)
    alpha = np.clip(1 - (raw[1] - raw[0]).mean(axis=2) / 255, 0, 1)
    col = np.where(alpha[..., None] > 0.01, raw[0] / np.maximum(alpha[..., None], 0.01), 0)
    Image.fromarray(np.dstack([np.clip(col, 0, 255), alpha * 255]).astype(np.uint8)).save(out_png)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("input"); p.add_argument("output")
    p.add_argument("--title", required=True); p.add_argument("--subtitle", required=True)
    p.add_argument("--version", default="v1.0.0")
    p.add_argument("--material", type=int, default=12); p.add_argument("--thickness", type=float, default=0.36)
    p.add_argument("--deform", type=float, default=1.0); p.add_argument("--noise", type=int, default=0)
    p.add_argument("--freq", type=float, default=2.8); p.add_argument("--time", type=float, default=3.0)
    p.add_argument("--no-sphere", action="store_true"); p.add_argument("--sphere-png", help="use this PNG instead of rendering")
    p.add_argument("--intro", type=float, default=3.5); p.add_argument("--fade-start", type=float, default=50.0)
    p.add_argument("--keep-dir", help="keep the intermediate PNGs here")
    a = p.parse_args()
    d = a.keep_dir or tempfile.mkdtemp(prefix="elements_video_"); os.makedirs(d, exist_ok=True)
    pr = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "stream=width,height:format=duration", "-of", "csv=p=0:s=x", a.input], capture_output=True, text=True).stdout.split()
    W, H = (int(v) for v in pr[0].split("x")); dur = float(pr[-1])
    sphere = a.sphere_png
    if not a.no_sphere and not sphere:
        sphere = os.path.join(d, "sphere.png"); render_sphere(a, d, sphere)
    card = os.path.join(d, "intro.png")
    subprocess.check_call([sys.executable, os.path.join(ROOT, "scripts", "video", "make_intro_card.py"), card, a.title, a.subtitle, a.version, sphere or "-", str(W), str(H)], cwd=ROOT)
    f = max(dur - a.fade_start, 0.1)
    graph = (f"[0:v]format=yuv420p,fade=t=in:st=0:d=0.7,fade=t=out:st={a.intro-0.7}:d=0.7,setsar=1[iv];"
             f"anullsrc=r=48000:cl=stereo,atrim=0:{a.intro},asetpts=N/SR/TB[ia];"
             f"[1:v]fade=t=out:st={a.fade_start}:d={f-0.1:.2f},setsar=1[mv];[1:a]afade=t=out:st={a.fade_start}:d={f:.2f}[ma];"
             "[iv][ia][mv][ma]concat=n=2:v=1:a=1[v][a]")
    subprocess.check_call(["ffmpeg", "-y", "-v", "error", "-loop", "1", "-framerate", "60", "-t", str(a.intro), "-i", card, "-i", a.input,
                           "-filter_complex", graph, "-map", "[v]", "-map", "[a]", "-r", "60", "-c:v", "libx264", "-crf", "17", "-preset", "slow",
                           "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", a.output])
    print("wrote", a.output, "| intermediates:", d)

if __name__ == "__main__":
    main()
