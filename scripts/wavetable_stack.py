"""Bitwig-style "wavetable stack" renderer for the Elements docs.

Takes a list of single-cycle waveforms (the frames) and draws them as a stack of
line plots receding into depth. Pure rendering: the waveforms themselves come
from the real engine (see docs/README in CLAUDE.md for the dump probe).
"""
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import to_rgb

BG = "#0d1117"


def _mix(c, w, bg=BG):
    c, b = np.array(to_rgb(c)), np.array(to_rgb(bg))
    return tuple(b + (c - b) * w)


def align_phase(frames, front=6, smooth=16):
    """Rotate all frames by one common phase offset so the cycle boundary (where every curve starts and
    stops) lands on a gentle slope for the front frames. Without this, steep edges at phase 0 leave
    vertical stubs / loose ends that look like broken lines. Purely cosmetic: a cycle has no true origin."""
    f = np.asarray(frames, dtype=float)
    sm = np.array([np.convolve(np.r_[w[-smooth:], w, w[:smooth]], np.ones(smooth) / smooth, "same")[smooth:-smooth]
                   for w in f])
    slope = np.abs(np.gradient(sm, axis=1))
    r = int(np.argmin(slope[:front].max(axis=0)))
    return np.roll(f, -r, axis=1)


def draw_stack(ax, frames, color, points=360, amp=0.55, dx=0.020, dy=0.060, width=2.2,
               fade_min=0.10, width_min=0.20, hide_behind=False):
    """frames[0] is drawn at the FRONT (bottom), frames[-1] at the back (top)."""
    n = len(frames)
    for k in range(n - 1, -1, -1):          # back to front (painter's algorithm)
        w = np.asarray(frames[k], dtype=float)
        idx = np.linspace(0, len(w), points, endpoint=False)
        y = np.interp(idx, np.arange(len(w)), w, period=len(w))
        y = np.append(y, y[0])
        x = np.linspace(0, 1, points + 1)
        ox, oy = k * dx, k * dy
        X, Y = x * 0.78 + ox, y * amp * dy * 5 + oy
        if hide_behind:                     # opaque fill = hidden-line look (cuts the back curves); off by default
            ax.fill_between(X, Y, oy - 0.02, color=BG, zorder=2 * (n - k), lw=0)
        t = k / max(n - 1, 1)               # 0 = front, 1 = back; opacity and thickness fall off linearly
        ax.plot(X, Y, color=_mix(color, 1.0 - t * (1.0 - fade_min)), lw=width * (1.0 - t * (1.0 - width_min)),
                zorder=2 * (n - k) + 1, solid_capstyle="round")


def render(frames, color, path, size=(3.2, 2.1), dpi=300, **kw):
    frames = align_phase(frames)
    kw.setdefault("points", 720)
    fig = plt.figure(figsize=size, dpi=dpi, facecolor=BG)
    ax = fig.add_axes([0.02, 0.04, 0.96, 0.92], facecolor=BG)
    draw_stack(ax, frames, color, **kw)
    ax.set_xlim(-0.02, 0.99)
    ax.set_ylim(-0.19, 0.74)
    ax.axis("off")
    fig.savefig(path, facecolor=BG)
    plt.close(fig)
