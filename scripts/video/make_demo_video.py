#!/usr/bin/env python3
"""Elements demo-video template: intro card (logo, version, deformed-sphere render, title, subtitle)
+ the screen capture + picture and sound fade-out at the end. macOS only (renders the sphere with CGL).

  python3 scripts/video/make_demo_video.py INPUT.mp4 OUTPUT.mp4 --title "The Deformer" \\
      --subtitle "The Deformer as a sound shaping tool. Noise types + wavefolding" \\
      [--material 12] [--thickness 0.36] [--deform 1] [--noise 0] [--freq 2.8] [--time 3] [--no-sphere] \\
      [--fade-start 50] [--intro 3.5] [--version v1.0.0] [--no-bars] [--bars-opacity 0.30]

--material: index in the plugin (0 Diamond, 1 Water, 2 Amber, 3 Ruby, 4 Gold, 5 Emerald, 6 Amethyst, 7 Sapphire,
            8 Copper, 9 Obsidian, 10 Alexandrite, 11 Malachite, 12 Neodymium). --noise: 0 Simplex, 1 Alligator, 2 Worley.
--time: the animation moment of the noise field (pick one you like; look at --keep-dir output).
Template defaults: 3.5 s intro with 0.7 s fades, silent; fade-out of image AND sound from --fade-start to the end.
16:9 canvas (Oct 28): a capture narrower than 16:9 is centred on a 16:9 canvas. The intro card is a full 16:9 card (no
effects). Behind the capture, soft light in the Spectrum display's colours (380-780 nm, the plugin's own wavelengthToColour,
split in two halves, violet..green left / yellow..red right, high frequencies on top) spills from the capture's left and
right edges and brightens with the capture's audio (FFT). Dim by design (--bars-opacity). --no-bars keeps the old
native-size output.
"""
import argparse, os, subprocess, sys, tempfile
import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TOOLS = os.path.join(ROOT, "scripts", "docs-tools")
SIZE = 700

FPS = 60
BG = (13, 17, 23)
N_BARS = 41          # per side strip (82 across the two halves)
F_LO, F_HI = 40.0, 14000.0

def wl2rgb(w):
    """Source/PluginEditor.cpp SpectrumDisplay::wavelengthToColour, verbatim."""
    r = g = b = 0.0
    if 380 <= w < 440: r = -(w - 440) / 60; b = 1
    elif 440 <= w < 490: g = (w - 440) / 50; b = 1
    elif 490 <= w < 510: g = 1; b = -(w - 510) / 20
    elif 510 <= w < 580: r = (w - 510) / 70; g = 1
    elif w >= 580 and w < 645: r = 1; g = -(w - 645) / 65
    elif w >= 645: r = 1
    f = 1.0
    if w < 420: f = 0.3 + 0.7 * (w - 380) / 40
    elif w > 700: f = 0.3 + 0.7 * (780 - w) / 80
    return np.array([r * f, g * f, b * f])

def band_levels(audio_in, nframes):
    """(frames, 2*N_BARS) levels 0..1; index 0 = violet/highest frequency, last = red/lowest."""
    sr = 48000
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", audio_in, "-vn", "-ac", "1", "-ar", str(sr), "-f", "f32le", "-"],
                         capture_output=True).stdout
    x = np.frombuffer(raw, np.float32)
    nb = 2 * N_BARS
    freqs = F_HI * (F_LO / F_HI) ** (np.arange(nb) / (nb - 1))
    win_n = 4096; win = np.hanning(win_n).astype(np.float32)
    bins = np.fft.rfftfreq(win_n, 1 / sr)
    tilt = 3.0 * np.log2(freqs / 1000.0)            # +3 dB/oct: compensates the natural spectral slope
    db = np.full((nframes, nb), -200.0)
    pad = np.concatenate([np.zeros(win_n // 2, np.float32), x, np.zeros(win_n, np.float32)])
    for k in range(nframes):
        c = int(k / FPS * sr)
        seg = pad[c:c + win_n]
        if len(seg) < win_n or not seg.any(): continue
        mag = np.abs(np.fft.rfft(seg * win)) / win_n
        db[k] = 20 * np.log10(np.interp(freqs, bins, mag) + 1e-9) + tilt
    live = db[db > -150]
    ref = np.percentile(live, 99.5) if live.size else 0.0
    lvl = np.clip((db - (ref - 48)) / 48, 0, 1) ** 1.3
    out = np.zeros_like(lvl); y = np.zeros(nb)
    for k in range(nframes):                         # instant attack, slow release
        y = np.maximum(lvl[k], y * 0.95); out[k] = y
    return out

def write_glow(proc, levels, side, H, opacity):
    """Soft spectrum light spilling out of the capture's left/right edges (not bars): each strip is the plugin's
    spectrum colours running top to bottom (violet..green left, yellow..red right); brightness follows the audio."""
    nb = 2 * N_BARS; Ws = 2 * side
    cols = np.array([wl2rgb(380 + 400 * i / nb) for i in range(nb)]) * 255
    yy = np.linspace(0, N_BARS - 1, H)
    colrow = [np.stack([np.interp(yy, np.arange(N_BARS), cols[h * N_BARS:(h + 1) * N_BARS, c]) for c in range(3)], 1) for h in (0, 1)]
    d = np.arange(side) / side                        # 0 at the capture's edge, 1 at the screen edge
    fall = np.exp(-d / 0.42)[None, :] * (1 - d ** 2)[None, :]
    falls = [fall[:, ::-1], fall]                     # left strip: edge on the right; right strip: edge on the left
    bg = np.array(BG, np.float32)[None, None, :]
    k3 = np.array([1, 2, 1]) / 4
    try:
        for lv in levels:
            frame = np.empty((H, Ws, 3), np.uint8)
            for h in (0, 1):
                prof = lv[h * N_BARS:(h + 1) * N_BARS]
                for _ in range(2): prof = np.convolve(np.pad(prof, 1, mode="edge"), k3, "valid")
                I = np.interp(yy, np.arange(N_BARS), 0.10 + 0.90 * prof)[:, None] * falls[h] * opacity
                frame[:, h * side:(h + 1) * side] = (bg + (colrow[h][:, None, :] - bg) * I[..., None]).astype(np.uint8)
            proc.stdin.write(frame.tobytes())
        proc.stdin.close()
    except BrokenPipeError:   # ffmpeg finished first (overlay eof_action=endall): the output is complete
        pass

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
    p.add_argument("--no-bars", action="store_true", help="keep the native capture size (no 16:9 canvas / spectrum strips)")
    p.add_argument("--bars-opacity", type=float, default=0.30)
    p.add_argument("--keep-dir", help="keep the intermediate PNGs here")
    a = p.parse_args()
    d = a.keep_dir or tempfile.mkdtemp(prefix="elements_video_"); os.makedirs(d, exist_ok=True)
    pr = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "stream=width,height:format=duration", "-of", "csv=p=0:s=x", a.input], capture_output=True, text=True).stdout.split()
    W, H = (int(v) for v in pr[0].split("x")); dur = float(pr[-1])
    sphere = a.sphere_png
    if not a.no_sphere and not sphere:
        sphere = os.path.join(d, "sphere.png"); render_sphere(a, d, sphere)
    f = max(dur - a.fade_start, 0.1)
    CW = int(round(H * 16 / 9 / 2)) * 2
    bars = not a.no_bars and CW > W and (CW - W) % 2 == 0
    BGHEX = "0x%02x%02x%02x" % BG
    card = os.path.join(d, "intro.png")
    cardargs = [str(CW if bars else W), str(H)] + ([str(W)] if bars else [])
    subprocess.check_call([sys.executable, os.path.join(ROOT, "scripts", "video", "make_intro_card.py"), card, a.title, a.subtitle, a.version, sphere or "-"] + cardargs, cwd=ROOT)
    enc = ["-r", str(FPS), "-c:v", "libx264", "-crf", "17", "-preset", "slow", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", a.output]
    intro = (f"[0:v]format=yuv420p,fade=t=in:st=0:d=0.7,fade=t=out:st={a.intro-0.7}:d=0.7,setsar=1[iv];"
             f"anullsrc=r=48000:cl=stereo,atrim=0:{a.intro},asetpts=N/SR/TB[ia];")
    audio = f"[1:a]afade=t=out:st={a.fade_start}:d={f:.2f}[ma];"
    inp = ["ffmpeg", "-y", "-v", "error", "-loop", "1", "-framerate", str(FPS), "-t", str(a.intro), "-i", card, "-i", a.input]
    if not bars:
        graph = (intro + f"[1:v]fade=t=out:st={a.fade_start}:d={f-0.1:.2f},setsar=1[mv];" + audio + "[iv][ia][mv][ma]concat=n=2:v=1:a=1[v][a]")
        subprocess.check_call(inp + ["-filter_complex", graph, "-map", "[v]", "-map", "[a]"] + enc)
    else:
        side = (CW - W) // 2; nfr = int(round(dur * FPS))
        levels = band_levels(a.input, nfr)
        grad = np.repeat(np.linspace(0, 1, H)[:, None, None], CW, 1)             # same top-to-bottom gradient as the intro card
        c0, c1 = np.array((17, 24, 32), float), np.array(BG, float)
        Image.fromarray((c0 + (c1 - c0) * grad).astype(np.uint8)).save(os.path.join(d, "bg.png"))
        graph = (intro + audio +
                 f"[2:v]split[sa][sb];[sa]crop={side}:{H}:0:0[L];[sb]crop={side}:{H}:{side}:0[R];"
                 f"[3:v][L]overlay=0:0[b1];[b1][R]overlay={CW-side}:0[b2];[b2][1:v]overlay={side}:0:eof_action=endall,format=yuv420p,"
                 f"fade=t=out:st={a.fade_start}:d={f-0.1:.2f},setsar=1[mv];"
                 "[iv][ia][mv][ma]concat=n=2:v=1:a=1[v][a]")
        proc = subprocess.Popen(inp + ["-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{2*side}x{H}", "-framerate", str(FPS), "-i", "pipe:0",
                                       "-loop", "1", "-framerate", str(FPS), "-t", f"{dur + 1:.3f}", "-i", os.path.join(d, "bg.png"),
                                       "-filter_complex", graph, "-map", "[v]", "-map", "[a]"] + enc, stdin=subprocess.PIPE)
        write_glow(proc, levels, side, H, a.bars_opacity)
        if proc.wait(): sys.exit("ffmpeg failed")
    print("wrote", a.output, "| intermediates:", d)

if __name__ == "__main__":
    main()
