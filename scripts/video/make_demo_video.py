#!/usr/bin/env python3
"""Elements demo-video template: intro card (logo, version, deformed-sphere render, title, subtitle)
+ the screen capture + picture and sound fade-out at the end. macOS only (renders the sphere with CGL).

  python3 scripts/video/make_demo_video.py INPUT.mp4 OUTPUT.mp4 --title "The Deformer" \\
      --subtitle "The Deformer as a sound shaping tool. Noise types + wavefolding" \\
      [--material 12] [--thickness 0.36] [--deform 1] [--noise 0] [--freq 2.8] [--time 3] [--no-sphere] \\
      [--fade-start 50] [--intro 3.5] [--version v1.0.0] [--no-bars] [--bars-opacity 0.38]

--material: index in the plugin (0 Diamond, 1 Water, 2 Amber, 3 Ruby, 4 Gold, 5 Emerald, 6 Amethyst, 7 Sapphire,
            8 Copper, 9 Obsidian, 10 Alexandrite, 11 Malachite, 12 Neodymium). --noise: 0 Simplex, 1 Alligator, 2 Worley.
--time: the animation moment of the noise field (pick one you like; look at --keep-dir output).
Template defaults: 3.5 s intro with 0.7 s fades, silent; fade-out of image AND sound from --fade-start to the end.
16:9 canvas (Oct 28): a capture narrower than 16:9 is centred on a 16:9 canvas and the side strips are filled with the
Spectrum display's colours (380-780 nm, the plugin's own wavelengthToColour) split in two halves, dim and driven by the
capture's audio (FFT). As in the plugin, the lowest frequencies are the red end (right) and the highest the violet end
(left). --no-bars keeps the old native-size output.
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

def band_levels(audio_in, intro, total_frames):
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
    db = np.full((total_frames, nb), -200.0)
    pad = np.concatenate([np.zeros(win_n // 2, np.float32), x, np.zeros(win_n, np.float32)])
    for k in range(total_frames):
        t = k / FPS - intro
        if t < 0: continue
        c = int(t * sr)
        seg = pad[c:c + win_n]
        if len(seg) < win_n or not seg.any(): continue
        mag = np.abs(np.fft.rfft(seg * win)) / win_n
        db[k] = 20 * np.log10(np.interp(freqs, bins, mag) + 1e-9) + tilt
    live = db[db > -150]
    ref = np.percentile(live, 99.5) if live.size else 0.0
    lvl = np.clip((db - (ref - 48)) / 48, 0, 1) ** 1.3
    out = np.zeros_like(lvl); y = np.zeros(nb)
    for k in range(total_frames):                    # instant attack, smooth release
        y = np.maximum(lvl[k], y * 0.93); out[k] = y
    return out

def write_bars(proc, levels, side, H, opacity):
    nb = 2 * N_BARS; Ws = 2 * side
    cols = np.array([wl2rgb(380 + 400 * i / nb) for i in range(nb)]) * 255
    xs = np.arange(Ws); half = xs // side; xin = xs - half * side
    pos = xin * N_BARS / side; k = np.floor(pos).astype(int); gap = (pos - k) > 0.83
    idx = half * N_BARS + np.minimum(k, N_BARS - 1)
    vf = (0.55 + 0.45 * np.arange(H) / H)[:, None, None]         # brighter at the base, dimmer at the top
    bg = np.array(BG, float)[None, None, :]
    barfull = (bg + (cols[idx][None, :, :] - bg) * opacity * vf).astype(np.uint8)
    bgfull = np.broadcast_to(np.array(BG, np.uint8), (H, Ws, 3))
    ys = np.arange(H)[:, None]
    try:
        for lv in levels:
            hpx = (0.012 + 0.70 * lv) * H
            mask = (ys >= (H - hpx[idx])[None, :]) & (~gap)[None, :]
            proc.stdin.write(np.where(mask[..., None], barfull, bgfull).tobytes())
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
    p.add_argument("--bars-opacity", type=float, default=0.38)
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
    CW = int(round(H * 16 / 9 / 2)) * 2
    bars = not a.no_bars and CW > W and (CW - W) % 2 == 0
    BGHEX = "0x%02x%02x%02x" % BG
    enc = ["-r", str(FPS), "-c:v", "libx264", "-crf", "17", "-preset", "slow", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", a.output]
    if not bars:
        graph = (f"[0:v]format=yuv420p,fade=t=in:st=0:d=0.7,fade=t=out:st={a.intro-0.7}:d=0.7,setsar=1[iv];"
                 f"anullsrc=r=48000:cl=stereo,atrim=0:{a.intro},asetpts=N/SR/TB[ia];"
                 f"[1:v]fade=t=out:st={a.fade_start}:d={f-0.1:.2f},setsar=1[mv];[1:a]afade=t=out:st={a.fade_start}:d={f:.2f}[ma];"
                 "[iv][ia][mv][ma]concat=n=2:v=1:a=1[v][a]")
        subprocess.check_call(["ffmpeg", "-y", "-v", "error", "-loop", "1", "-framerate", str(FPS), "-t", str(a.intro), "-i", card, "-i", a.input,
                               "-filter_complex", graph, "-map", "[v]", "-map", "[a]"] + enc)
    else:
        side = (CW - W) // 2; total = a.intro + dur; nfr = int(round(total * FPS))
        levels = band_levels(a.input, a.intro, nfr)
        ft = a.intro + a.fade_start
        graph = (f"[0:v]format=yuv420p,fade=t=in:st=0:d=0.7:color={BGHEX},fade=t=out:st={a.intro-0.7}:d=0.7:color={BGHEX},setsar=1[iv];"
                 f"anullsrc=r=48000:cl=stereo,atrim=0:{a.intro},asetpts=N/SR/TB[ia];"
                 f"[1:v]fade=t=out:st={a.fade_start}:d={f-0.1:.2f},setsar=1[mv];[1:a]afade=t=out:st={a.fade_start}:d={f:.2f}[ma];"
                 "[iv][ia][mv][ma]concat=n=2:v=1:a=1[m][a];"
                 f"[2:v]split[sa][sb];[sa]crop={side}:{H}:0:0[L];[sb]crop={side}:{H}:{side}:0[R];"
                 f"color=c={BGHEX}:s={CW}x{H}:r={FPS}:d={total:.3f}[bg];[bg][L]overlay=0:0[b1];[b1][R]overlay={CW-side}:0,format=yuv420p,"
                 f"fade=t=in:st=0:d=0.7,fade=t=out:st={ft:.3f}:d={f-0.1:.2f}[base];"
                 f"[base][m]overlay={side}:0:eof_action=endall,setsar=1[v]")
        proc = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-loop", "1", "-framerate", str(FPS), "-t", str(a.intro), "-i", card, "-i", a.input,
                                 "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{2*side}x{H}", "-framerate", str(FPS), "-i", "pipe:0",
                                 "-filter_complex", graph, "-map", "[v]", "-map", "[a]"] + enc, stdin=subprocess.PIPE)
        write_bars(proc, levels, side, H, a.bars_opacity)
        if proc.wait(): sys.exit("ffmpeg failed")
    print("wrote", a.output, "| intermediates:", d)

if __name__ == "__main__":
    main()
