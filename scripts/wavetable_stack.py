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


def draw_stack(ax, frames, color, points=360, amp=0.55, dx=0.020, dy=0.060, width=2.2):
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
        ax.fill_between(X, Y, oy - 0.02, color=BG, zorder=2 * (n - k), lw=0)
        depth = 1.0 - 0.72 * (k / max(n - 1, 1))
        ax.plot(X, Y, color=_mix(color, depth), lw=width * (0.75 + 0.25 * depth),
                zorder=2 * (n - k) + 1, solid_capstyle="round")


def render(frames, color, path, size=(3.2, 2.1), dpi=150, **kw):
    fig = plt.figure(figsize=size, dpi=dpi, facecolor=BG)
    ax = fig.add_axes([0.02, 0.04, 0.96, 0.92], facecolor=BG)
    draw_stack(ax, frames, color, **kw)
    ax.set_xlim(-0.02, 0.99)
    ax.set_ylim(-0.19, 0.74)
    ax.axis("off")
    fig.savefig(path, facecolor=BG)
    plt.close(fig)
