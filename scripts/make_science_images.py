#!/usr/bin/env python3
"""Regenerate the figures used by docs/science.md and docs/index.md.

  python3 scripts/make_science_images.py --dump /path/to/probe_out

Writes to docs/assets/images/science/. Everything is computed from the same constants and formulas the
page documents: material curves are parsed from Source/Physics.cpp, light spectra / Fresnel / Beer-Lambert /
envelope / chorus / pitch / wavefolder / blend maths are the formulas printed on the page, and the Deformer
figure uses the real engine dump (scripts/docs-tools/dump_probe.cpp, see make_material_images.py).
Needs numpy and matplotlib. --dump is only needed for the Deformer figure and the envelope averages.
"""
import argparse
import json
import os
import re
import sys

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "docs", "assets", "images", "science")
sys.path.insert(0, HERE)
import wavetable_stack as ws  # noqa: E402
from make_material_images import NAMES, ACCENT  # noqa: E402

BG, PANEL, GRID, MID, TEXT = "#0d1117", "#111820", "#1f2d3d", "#4a6075", "#d0e4f0"
CYAN, AMETH, GREEN, AMBER, RED, BLUE = "#a8d8f0", "#b57bee", "#4ecb8d", "#f5b942", "#e84b6a", "#5b9ef5"
DPI = 200

plt.rcParams.update({
    "figure.facecolor": BG, "axes.facecolor": PANEL, "savefig.facecolor": BG,
    "axes.edgecolor": GRID, "axes.labelcolor": MID, "xtick.color": MID, "ytick.color": MID,
    "text.color": TEXT, "axes.titlecolor": TEXT, "grid.color": GRID, "grid.linewidth": 0.6,
    "font.size": 8.5, "axes.titlesize": 9.5, "axes.titleweight": "bold", "axes.labelsize": 8,
    "legend.facecolor": BG, "legend.edgecolor": GRID, "legend.fontsize": 7.5, "axes.grid": True,
    "font.family": "DejaVu Sans",
})

WL = np.linspace(380, 780, 50)  # the engine's 50-point wavelength grid


# ---------------------------------------------------------------- data
def load_materials():
    src = open(os.path.join(ROOT, "Source", "Physics.cpp"), encoding="utf-8").read()
    out = {}
    for m in re.finditer(r'Material\("(\w+)",\s*\{([^}]*)\},\s*\{([^}]*)\},\s*"#[0-9A-Fa-f]{6}",\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f',
                         src):
        name, wl, tr, n, k, met = m.groups()
        f = lambda s: np.array([float(x) for x in re.findall(r"[\d.]+(?=f)", s)])
        out[name.lower()] = dict(wl=f(wl), t=f(tr), n=float(n), k=float(k), metal=float(met))
    assert len(out) == 13, len(out)
    return out


def on_grid(mat):
    return np.interp(WL, mat["wl"], mat["t"])


def wl_rgb(wl):
    """Approximate visible-spectrum colour for a wavelength in nm (Dan Bruton)."""
    wl = float(wl)
    if wl < 440: r, g, b = (440 - wl) / 60, 0, 1
    elif wl < 490: r, g, b = 0, (wl - 440) / 50, 1
    elif wl < 510: r, g, b = 0, 1, (510 - wl) / 20
    elif wl < 580: r, g, b = (wl - 510) / 70, 1, 0
    elif wl < 645: r, g, b = 1, (645 - wl) / 65, 0
    else: r, g, b = 1, 0, 0
    f = 0.3 + 0.7 * min(1, (wl - 380) / 40, (780 - wl) / 80 + 0.0001) if wl < 420 or wl > 700 else 1
    return (r * f, g * f, b * f)


def strip(ax, alpha=0.22):
    """Faint visible-spectrum gradient behind a wavelength axis."""
    img = np.array([[wl_rgb(w) for w in np.linspace(380, 780, 200)]])
    ax.imshow(img, extent=[380, 780, *ax.get_ylim()], aspect="auto", alpha=alpha, zorder=0)
    ax.set_xlim(380, 780)


def save(fig, name):
    os.makedirs(OUT, exist_ok=True)
    fig.savefig(os.path.join(OUT, name), dpi=DPI)
    plt.close(fig)
    print("wrote", name)


def fresnel_R(theta, n):
    ct = np.cos(theta)
    st = np.sin(theta) / n
    ctt = np.sqrt(1 - st ** 2)
    rs = (ct - n * ctt) / (ct + n * ctt)
    rp = (n * ct - ctt) / (n * ct + ctt)
    return 0.5 * (rs ** 2 + rp ** 2)


def schlick(theta, f0):
    return f0 + (1 - f0) * (1 - np.cos(theta)) ** 5


LIGHTS = {"Sunset": (650, 70, 0.15, 0.85, AMBER), "Daylight": (550, 100, 0.20, 0.80, CYAN),
          "LED Cool": (470, 75, 0.15, 0.85, BLUE)}


def light(name, wl=WL):
    c, s, b, p, _ = LIGHTS[name]
    return b + p * np.exp(-((wl - c) ** 2) / (2 * s ** 2))


def spectral_mod(theta_deg, wl=WL, metal=False):
    a = theta_deg / 90.0
    pos = (wl - 380) / 400.0
    if metal:
        return (1 - 0.3 * a) ** (0.5 + 1.0 * (1 - pos))
    return (1 - a) ** (1 + 5 * (1 - pos))


# ---------------------------------------------------------------- figures
def fig_fresnel(M):
    fig, ax = plt.subplots(figsize=(7.4, 3.3))
    th = np.radians(np.linspace(0, 89.5, 400))
    for n, label, col in [(1.33, "Water  n 1.33", GREEN), (1.55, "Amber  n 1.55", AMBER),
                          (1.77, "Ruby / Sapphire  n 1.77", RED), (2.42, "Diamond  n 2.42", "#ffffff")]:
        ax.plot(np.degrees(th), fresnel_R(th, n), color=col, lw=1.8, label=label)
    g = M["gold"]
    f0 = ((g["n"] - 1) ** 2 + g["k"] ** 2) / ((g["n"] + 1) ** 2 + g["k"] ** 2)
    ax.plot(np.degrees(th), schlick(th, f0), color="#d4a843", lw=1.8, ls="--", label=f"Gold  n+ik (Schlick, F₀ {f0:.2f})")
    ax.set_xlabel("angle of incidence θ (degrees)")
    ax.set_ylabel("reflectance R  (T = 1 − R)")
    ax.set_xlim(0, 90); ax.set_ylim(0, 1.02)
    ax.set_title("Fresnel reflectance: every surface turns mirror-like at grazing angles")
    ax.legend(loc="upper left")
    fig.tight_layout()
    save(fig, "fresnel.png")


def fig_angle_shaping():
    fig, axs = plt.subplots(1, 2, figsize=(8.4, 3.2), sharey=True)
    cols = plt.cm.viridis(np.linspace(0.95, 0.15, 6))
    for ax, metal, ttl in [(axs[0], False, "Dielectrics (section 2)"), (axs[1], True, "Metals (section 4)")]:
        ax.set_ylim(0, 1.05)
        for a, c in zip([0, 30, 45, 60, 70, 80], cols):
            ax.plot(WL, spectral_mod(a, metal=metal), color=c, lw=1.8, label=f"{a}°")
        strip(ax)
        ax.set_title(ttl); ax.set_xlabel("wavelength (nm)  —  blue ←  → red")
    axs[0].set_ylabel("spectralMod(λ)")
    axs[0].legend(title="θ", loc="upper left", ncol=2)
    fig.suptitle("Angle-dependent colour shift: dielectrics lose blue first, metals barely change", y=0.99, fontsize=10, fontweight="bold")
    fig.tight_layout()
    save(fig, "angle-shaping.png")


def fig_beer(M):
    fig, axs = plt.subplots(1, 2, figsize=(8.4, 3.2), sharey=True)
    for ax, name, col in [(axs[0], "ruby", RED), (axs[1], "emerald", GREEN)]:
        t = on_grid(M[name])
        ax.set_ylim(0, 1.02)
        for th, lw, al in [(0.1, 1.2, 0.45), (0.5, 1.6, 0.7), (1.0, 2.2, 1.0), (2.0, 1.6, 0.7)]:
            ax.plot(WL, t ** th, color=col, lw=lw, alpha=al, ls={0.1: ":", 0.5: "--", 1.0: "-", 2.0: "-."}[th], label=f"Thickness {th:g}")
        ax.legend(loc="center right" if name == "emerald" else "upper left", fontsize=7)
        strip(ax)
        ax.set_title(f"{name.title()}: T(λ)^thickness")
        ax.set_xlabel("wavelength (nm)")
    axs[0].set_ylabel("effective transmission")
    fig.suptitle("Beer-Lambert: thickness sharpens a material's character (Ruby and Emerald)", y=0.99, fontsize=10, fontweight="bold")
    fig.tight_layout()
    save(fig, "beer-lambert.png")


def fig_geometry():
    fig, ax = plt.subplots(figsize=(7.4, 3.1))
    th = np.linspace(0, 90, 300)
    c = np.cos(np.radians(th))
    for lab, y, col, ls in [("Cube  cos⁴θ", c ** 4, RED, "-"), ("Torus, Dodecahedron, Teapot  cos²θ", c ** 2, AMBER, "-"),
                            ("Deformed sphere  cosθ", c, GREEN, "-")]:
        ax.plot(th, y, color=col, lw=2, ls=ls, label=lab)
    ax.axvline(10, color=CYAN, lw=1.4, ls=":", label="Undeformed sphere: one normal at a fixed 10°")
    ax.set_xlabel("angle between a face normal and the light (degrees)")
    ax.set_ylabel("face weight w")
    ax.set_xlim(0, 90); ax.set_ylim(0, 1.03)
    ax.set_title("How much each face counts: sharper weights make rotation more dramatic")
    ax.legend(loc="upper right")
    fig.tight_layout()
    save(fig, "geometry-weights.png")


def spectrum_pipeline(M, name="ruby", src="Daylight", theta=30.0, thick=1.0):
    mat = M[name]
    L = light(src); T = on_grid(mat)
    R = fresnel_R(np.radians(theta), mat["n"]); F = (1 - R) * spectral_mod(theta)
    S = L * T * F
    S = S ** thick
    S = S / S.max()          # peak-normalised for display
    return L, T, F, S


def body_amps(S, H=20):
    h = np.arange(1, H + 1)
    pos = 1 - (h - 1) / (H - 1)
    raw = np.interp(pos * 49, np.arange(50), S)
    emph = raw ** 3
    roll = 1 / (1 + (h - 1) * 0.05)
    return h, raw, emph, emph * roll


def cycle(amps, x):
    return sum(a * np.sin(2 * np.pi * (i + 1) * x) for i, a in enumerate(amps))


def fig_pipeline(M):
    L, T, F, S = spectrum_pipeline(M)
    h, raw, emph, fin = body_amps(S)
    fig = plt.figure(figsize=(9.4, 4.6))
    w, op = 0.19, 0.055
    xs = [0.045 + i * (w + op) for i in range(4)]
    top = [(L, "Light", "Daylight", CYAN), (T, "Material", "Ruby", RED), (F, "Fresnel", "30°", AMETH), (S, "Spectrum", "S(λ)", GREEN)]
    for i, (y, ttl, sub, col) in enumerate(top):
        ax = fig.add_axes([xs[i], 0.57, w, 0.30])
        ax.plot(WL, y, color=col, lw=1.8)
        ax.set_ylim(0, 1.08); strip(ax, 0.30)
        ax.set_xticks([400, 600, 780]); ax.set_yticks([0, 0.5, 1] if i == 0 else [])
        ax.tick_params(labelsize=6.5)
        ax.set_title(f"{ttl}\n{sub}", fontsize=8.5, color=col, linespacing=1.2)
        if i < 3:
            fig.text(xs[i] + w + op / 2, 0.72, "×" if i < 2 else "=", ha="center", va="center", fontsize=17, color=MID)
    # harmonics
    ax = fig.add_axes([0.045, 0.09, 0.30, 0.32])
    cols = [wl_rgb(780 - (k - 1) / 19 * 400) for k in h]
    ax.bar(h, fin, color=cols, width=0.75)
    ax.set_xticks([1, 5, 10, 15, 20]); ax.set_xlim(0.3, 20.7); ax.tick_params(labelsize=6.5)
    ax.set_title("Harmonic amplitudes  A³ · rolloff", fontsize=8.5, color=TEXT)
    ax.set_xlabel("harmonic  (1 = red … 20 = violet)", fontsize=7)
    ax2 = fig.add_axes([0.45, 0.09, 0.50, 0.32])
    x = np.linspace(0, 2, 800, endpoint=False)
    body = cycle(fin, x); body = body / np.abs(body).max() * 1.6
    ax2.plot(x, np.tanh(body), color=RED, lw=2.0)
    ax2.set_ylim(-1.15, 1.15); ax2.set_xlim(0, 2); ax2.set_xticks([]); ax2.set_yticks([])
    ax2.set_title("Wavetable (two cycles):  Σ sin  →  tanh", fontsize=8.5, color=TEXT)
    ax2.axhline(0, color=GRID, lw=0.8)
    for a, b in [(0.355, 0.435)]:
        fig.text(0.395, 0.25, "→", ha="center", va="center", fontsize=22, color=MID)
    fig.text(0.25, 0.505, "↓", ha="center", va="center", fontsize=0.1, color=BG)
    fig.text(0.5, 0.965, "From light to sound: Daylight × Ruby × Fresnel at 30° (formulas of this page, Thickness 1)",
             ha="center", fontsize=10, fontweight="bold")
    save(fig, "pipeline.png")


def fig_harmonics(M):
    L, T, F, S = spectrum_pipeline(M, "ruby", "Daylight", 30)
    h, raw, emph, fin = body_amps(S)
    fig, axs = plt.subplots(1, 3, figsize=(9.4, 3.0))
    ax = axs[0]
    ax.plot(WL, S, color=GREEN, lw=1.8); ax.fill_between(WL, S, color=GREEN, alpha=0.15)
    ax.set_ylim(0, 1.08); strip(ax, 0.25)
    ax.set_title("1. Spectrum S(λ)"); ax.set_xlabel("wavelength (nm)")
    cols = [wl_rgb(780 - (k - 1) / 19 * 400) for k in h]
    axs[1].bar(h, raw, color=cols, width=0.75); axs[1].set_title("2. Sample it: one value per harmonic")
    axs[2].bar(h, raw, color=cols, width=0.75, alpha=0.18)
    axs[2].bar(h, fin, color=cols, width=0.75); axs[2].set_title("3. Cube it, apply rolloff")
    for ax in axs[1:]:
        ax.set_ylim(0, 1.08); ax.set_xlim(0.3, 20.7); ax.set_xticks([1, 5, 10, 15, 20]); ax.set_xlabel("harmonic")
    axs[2].annotate("faint = before A³", (13, 0.35), color=MID, fontsize=7.5)
    fig.tight_layout()
    save(fig, "harmonic-mapping.png")


def fig_light_spectra():
    fig, ax = plt.subplots(figsize=(7.4, 3.0))
    w = np.linspace(380, 780, 400)
    for n, (_, _, _, _, col) in LIGHTS.items():
        y = light(n, w)
        ax.plot(w, y, color=col, lw=2, label=n)
        ax.fill_between(w, y, color=col, alpha=0.12)
    ax.set_ylim(0, 1.08); strip(ax, 0.20)
    ax.set_xlabel("wavelength (nm)"); ax.set_ylabel("relative power")
    ax.set_title("The three light sources: Gaussians with a low floor")
    ax.legend(loc="upper right")
    fig.tight_layout()
    save(fig, "light-spectra.png")


def fig_pitch():
    fig, ax = plt.subplots(figsize=(7.4, 3.0))
    i = np.linspace(0, 1, 100)
    for n, col in [(1, CYAN), (2, AMBER), (3, RED)]:
        total = i + (n - 1) * 0.5
        dev = total - n * 0.5
        y = np.clip(dev / (n * 0.5) * 2.0, -2, 2)
        ax.plot(i, y, color=col, lw=2, label=f"{n} light{'s' if n > 1 else ''} on (this slider moves, the others stay at 0.5)")
    ax.axhline(0, color=MID, lw=0.8); ax.axvline(0.5, color=MID, lw=0.8)
    ax.set_xlabel("intensity of the light you move"); ax.set_ylabel("pitch offset (semitones)")
    ax.set_xlim(0, 1); ax.set_ylim(-2.2, 2.2)
    ax.set_title("Pitch follows light intensity: neutral at 0.5, ±2 semitones at the extremes")
    ax.legend(loc="upper left", fontsize=7)
    fig.tight_layout()
    save(fig, "pitch-vs-intensity.png")


def fig_envelope(M, dump):
    entries = {e["id"]: e for e in json.load(open(os.path.join(dump, "manifest.json")))["entries"]}
    fig, axs = plt.subplots(1, 4, figsize=(9.6, 2.7), sharey=True)
    thick, inten = 1.0, 0.5
    for ax, name in zip(axs, ["diamond", "water", "ruby", "obsidian"]):
        e = entries[f"scene_m{NAMES.index(name)}_s1"]
        a = 0.5 + (0.005 - 0.5) * inten
        d = thick * (1 - e["avgTrans"]) * 1.5
        s = float(np.clip(e["ior"] / 2.42, 0.1, 0.95))
        r = e["ior"] * 0.2 * thick
        hold = 0.5
        t = [0, a, a + d, a + d + hold, a + d + hold + r]
        y = [0, 1, s, s, 0]
        col = ACCENT[NAMES.index(name)]
        ax.plot(t, y, color=col, lw=2.2); ax.fill_between(t, y, color=col, alpha=0.14)
        ax.set_title(f"{name.title()}\nIOR {e['ior']:.2f} · avg T {e['avgTrans']:.2f}", fontsize=8.5, color=col)
        ax.set_xlim(0, 3.0); ax.set_ylim(0, 1.1)
        ax.set_xlabel("seconds (note held for 0.5 s)", fontsize=7)
    axs[0].set_ylabel("level")
    fig.suptitle("Physical envelope from optical properties (Thickness 1.0, light intensity 0.5)", y=0.99, fontsize=10, fontweight="bold")
    fig.tight_layout()
    save(fig, "physical-envelope.png")


def fig_chorus():
    fig, axs = plt.subplots(1, 3, figsize=(9.4, 2.8), sharey=True)
    for ax, decay in zip(axs, [0.0, 0.3, 0.6]):
        for v in range(5):
            g = (1 - decay) ** v
            ax.vlines(v * 20, 0, g, color=AMETH if v else CYAN, lw=5, alpha=1 - 0.0 * v)
            ax.text(v * 20, g + 0.03, f"{g:.2f}", ha="center", fontsize=7, color=TEXT)
        ax.set_title(f"Decay {decay:g}"); ax.set_xlim(-8, 88); ax.set_ylim(0, 1.2)
        ax.set_xlabel("delay (ms), Spread 20 ms, Voices 5")
    axs[0].set_ylabel("copy gain  (1 − decay)ᵛ")
    fig.suptitle("Chorus taps: each copy is a delayed, quieter echo (cyan = the original)", y=0.99, fontsize=10, fontweight="bold")
    fig.tight_layout()
    save(fig, "chorus-taps.png")


def fig_wavefolder():
    fig, ax = plt.subplots(figsize=(7.0, 3.2))
    x = np.linspace(-1, 1, 600)
    for drive, col in [(0, MID), (0.25, BLUE), (0.5, GREEN), (0.75, AMBER), (1.0, RED)]:
        xp = x * (1 + 3 * drive)
        y = x + (np.sin(xp * np.pi / 2) - x) * drive
        ax.plot(x, y, color=col, lw=2, label=f"drive {drive:g}")
    ax.set_xlabel("input x"); ax.set_ylabel("output y"); ax.set_xlim(-1, 1); ax.set_ylim(-1.1, 1.1)
    ax.set_title("The wavefolder: more drive, more folds")
    ax.legend(loc="upper left", ncol=2)
    fig.tight_layout()
    save(fig, "wavefolder.png")


def fig_blend(M):
    def wave(name, x):
        _, _, _, S = spectrum_pipeline(M, name, "Daylight", 30)
        _, _, _, fin = body_amps(S)
        peak = np.abs(cycle(fin, np.linspace(0, 1, 800, endpoint=False))).max()
        return lambda p: np.tanh(cycle(fin, p) / peak * 1.6)
    x = np.linspace(0, 2, 8000, endpoint=False)
    A, B = wave("ruby", x), wave("emerald", x)
    a, b = A(x), B(x * 1.5)  # B a fifth up so the interaction is visible
    depth = 0.25
    panels = [("Oscillator A (Ruby)", a, RED), ("Oscillator B (Emerald), a fifth up", b, GREEN),
              ("Ring Mod:  A · B", a * b, AMETH),
              ("AM:  A · max(0, 1 + 3·depth·B)", a * np.maximum(0, 1 + 3 * depth * b), AMBER),
              ("XOR:  A − B", a - b, CYAN),
              ("FM:  A(phase + 4·depth·B)   (depth 0.04)", A(x + 4 * 0.04 * b), BLUE)]
    fig, axs = plt.subplots(3, 2, figsize=(9.0, 5.4), sharex=True)
    for ax, (ttl, y, col) in zip(axs.T.flat if False else axs.flat, panels):
        ax.plot(x, y, color=col, lw=1.6); ax.axhline(0, color=GRID, lw=0.7)
        ax.set_title(ttl, fontsize=8.5, color=col); ax.set_yticks([]); ax.set_xticks([])
        ym = max(1.05, np.abs(y).max() * 1.08); ax.set_ylim(-ym, ym); ax.set_xlim(0, 2)
    fig.suptitle("Blend modes on two real material waveforms (two cycles of A, depth 0.25; FM 0.04)", y=0.995, fontsize=10, fontweight="bold")
    fig.tight_layout()
    save(fig, "blend-modes.png")


def fig_materials(M):
    fig, axs = plt.subplots(3, 5, figsize=(10.4, 5.6), sharex=True, sharey=True)
    for i, ax in enumerate(axs.flat):
        if i < 13:
            name = NAMES[i]; t = M[name]
            ax.set_ylim(0, 1.05)
            ax.plot(t["wl"], t["t"], color=ACCENT[i], lw=1.8); ax.fill_between(t["wl"], t["t"], color=ACCENT[i], alpha=0.14)
            strip(ax, 0.20)
            ax.set_title(name.title() + ("  (reflectance)" if t["metal"] else ""), fontsize=8.5, color=ACCENT[i])
        elif i == 13:
            ax.set_ylim(0, 1.05)
            for j, name in enumerate(NAMES):
                ax.plot(M[name]["wl"], M[name]["t"], color=ACCENT[j], lw=1.0, alpha=0.9)
            strip(ax, 0.20); ax.set_title("All thirteen", fontsize=8.5)
        else:
            ax.axis("off")
        ax.tick_params(labelsize=6.5)
    for ax in axs[-1]:
        ax.set_xlabel("wavelength (nm)", fontsize=7)
    fig.suptitle("The spectral curves of all 13 materials (transmission; reflectance for Gold and Copper)", y=0.995, fontsize=10.5, fontweight="bold")
    fig.tight_layout()
    save(fig, "material-curves.png")


def fig_deform(dump, mat="diamond"):
    m = NAMES.index(mat)
    entries = json.load(open(os.path.join(dump, "manifest.json")))["entries"]
    de = sorted([e for e in entries if e["kind"] == "deform" and e["mat"] == m], key=lambda e: e["deform"])
    fig = plt.figure(figsize=(9.4, 3.5))
    ax = fig.add_axes([0.06, 0.16, 0.40, 0.70])
    cols = plt.cm.plasma(np.linspace(0.05, 0.9, len(de)))
    for e, c in zip(de, cols):
        ax.plot(WL, e["spectrum"], color=c, lw=1.4)
    ax.set_xlim(380, 780); ax.set_xlabel("wavelength (nm)"); ax.set_ylabel("spectrum level")
    ax.set_title("Spectrum as Deform rises 0 → 1 (purple → yellow)")
    ax2 = fig.add_axes([0.52, 0.04, 0.46, 0.86])
    frames = [np.fromfile(os.path.join(dump, f"tab_deform_m{m}_{k}.f32"), dtype=np.float32).reshape(10, 2048)[4] for k in range(len(de))]
    frames = ws.align_phase(frames)
    ws.draw_stack(ax2, frames, ACCENT[m], points=720, dx=0.020, dy=0.060, width=2.0)
    ax2.set_xlim(-0.02, 0.99); ax2.set_ylim(-0.19, 0.78); ax2.axis("off"); ax2.set_facecolor(BG)
    ax2.set_title("Waveform: front = Deform 0, back = Deform 1", color=TEXT)
    ax2.title.set_visible(True)
    save(fig, "deformer.png")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dump", default=None)
    a = ap.parse_args()
    M = load_materials()
    fig_fresnel(M); fig_angle_shaping(); fig_beer(M); fig_geometry()
    fig_pipeline(M); fig_harmonics(M); fig_light_spectra(); fig_pitch()
    fig_chorus(); fig_wavefolder(); fig_blend(M); fig_materials(M)
    if a.dump:
        fig_envelope(M, a.dump); fig_deform(a.dump)


if __name__ == "__main__":
    main()
