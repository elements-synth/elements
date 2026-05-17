"""
Elements — Material Spectral Curves v2
All materials at 32 sample points, raw values (no contrast transform).
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

BG       = '#0d1117'
PANEL_BG = '#111820'
GRID_COL = '#1c2534'
TEXT_COL = '#d0e4f0'
DIM_COL  = '#4a6075'

ACCENT = {
    'Diamond':    '#a8d8f0',
    'Water':      '#7ec8e3',
    'Amber':      '#f5b942',
    'Ruby':       '#e84b6a',
    'Gold':       '#d4a843',
    'Emerald':    '#4ecb8d',
    'Amethyst':   '#b57bee',
    'Sapphire':   '#5b9ef5',
    'Copper':     '#cf7e46',
    'Obsidian':   '#6a7a8a',
    'Alexandrite':'#5b8a64',
    'Malachite':  '#2e7d52',
    'Neodymium':  '#9070c8',
}

WL16 = np.array([380,407,433,460,487,513,540,567,593,620,647,673,700,727,753,780],dtype=float)
WL32 = np.array([380,393,406,419,432,445,457,470,483,496,509,522,535,548,561,574,
                 587,600,613,626,638,651,664,677,690,703,716,728,741,754,767,780],dtype=float)

def up32(wl_in, T_in):
    """Upsample any curve to WL32 via linear interpolation."""
    return np.interp(WL32, wl_in, T_in)

# ── Raw 16-point data for simple materials ────────────────────────────────────
T16 = {
    'Diamond': np.array([0.92,0.94,0.95,0.96,0.97,0.97,0.97,0.97,0.96,0.96,0.96,0.95,0.95,0.95,0.94,0.94]),
    'Water':   np.array([0.989,0.995,0.995,0.991,0.986,0.963,0.954,0.933,0.874,0.759,0.716,0.645,0.535,0.186,0.074,0.018]),
    'Amber':   np.array([0.01,0.03,0.08,0.22,0.48,0.68,0.78,0.86,0.90,0.93,0.95,0.96,0.96,0.97,0.97,0.97]),
    'Gold':    np.array([0.01,0.01,0.02,0.03,0.04,0.12,0.58,0.80,0.91,0.95,0.97,0.97,0.98,0.98,0.98,0.98]),
    'Amethyst':np.array([0.88,0.84,0.78,0.68,0.45,0.20,0.09,0.14,0.22,0.32,0.42,0.50,0.55,0.52,0.50,0.48]),
    'Copper':  np.array([0.01,0.01,0.01,0.01,0.02,0.02,0.03,0.05,0.07,0.23,0.43,0.63,0.85,0.88,0.92,0.95]),
    'Obsidian':np.array([0.01,0.01,0.01,0.01,0.02,0.02,0.03,0.05,0.07,0.12,0.20,0.32,0.48,0.60,0.67,0.72]),
}

# ── All materials: 32-point transmission curves ───────────────────────────────
materials = {
    'Diamond': {
        'T': up32(WL16, T16['Diamond']),
        'src': 'Sellmeier (1923)  k≈0  near-flat visible',
    },
    'Water': {
        'T': up32(WL16, T16['Water']),
        'src': 'Pope & Fry (1997)  1m Beer-Lambert',
    },
    'Amber': {
        'T': up32(WL16, T16['Amber']),
        'src': 'PMC12196071  UV cutoff ~440nm',
    },
    'Ruby': {
        'T': np.array([0.02,0.02,0.03,0.06,0.10,0.12,0.13,0.10,0.08,0.05,0.03,0.02,0.02,0.02,0.04,0.08,
                       0.15,0.30,0.55,0.75,0.85,0.90,0.93,0.95,0.97,0.96,0.97,0.97,0.97,0.97,0.98,0.98]),
        'src': 'PMC9330567  Cr³⁺ 413/550nm',
    },
    'Gold': {
        'T': up32(WL16, T16['Gold']),
        'src': 'Palik (1985)  complex IOR',
    },
    'Emerald': {
        'T': np.array([0.04,0.07,0.09,0.07,0.04,0.12,0.28,0.48,0.65,0.78,0.87,0.90,0.89,0.88,0.82,0.70,
                       0.50,0.28,0.14,0.08,0.06,0.06,0.06,0.05,0.04,0.04,0.03,0.03,0.03,0.02,0.02,0.02]),
        'src': 'Wood & Nassau (1968)  Cr³⁺ 430/610nm',
    },
    'Amethyst': {
        'T': up32(WL16, T16['Amethyst']),
        'src': 'PMC7483767  [FeO₄]⁰ 545nm',
    },
    'Sapphire': {
        'T': np.array([0.88,0.82,0.85,0.86,0.83,0.78,0.75,0.68,0.58,0.46,0.34,0.24,0.16,0.11,0.08,0.07,
                       0.07,0.06,0.05,0.04,0.03,0.03,0.02,0.02,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01]),
        'src': 'GIA 2020  Fe²⁺–Ti⁴⁺ IVCT 570nm',
    },
    'Copper': {
        'T': up32(WL16, T16['Copper']),
        'src': 'Palik (1985)  complex IOR',
    },
    'Obsidian': {
        'T': up32(WL16, T16['Obsidian']),
        'src': 'Icarus 2021 / USGS  featureless',
    },
    'Alexandrite': {
        'T': np.array([0.07,0.09,0.10,0.08,0.05,0.08,0.22,0.45,0.62,0.72,0.74,0.70,0.60,0.42,0.25,0.16,
                       0.20,0.32,0.46,0.58,0.68,0.76,0.82,0.86,0.88,0.88,0.88,0.87,0.88,0.88,0.89,0.90]),
        'src': 'PMC7145866  Cr³⁺ 440/572nm',
    },
    'Malachite': {
        'T': np.array([0.08,0.12,0.18,0.25,0.34,0.45,0.56,0.66,0.74,0.81,0.86,0.89,0.90,0.88,0.84,0.78,
                       0.70,0.62,0.53,0.45,0.38,0.32,0.27,0.22,0.19,0.16,0.14,0.13,0.12,0.12,0.11,0.11]),
        'src': 'USGS splib07 K-M  Cu²⁺ LMCT+d-d',
    },
    'Neodymium': {
        'T': np.array([0.72,0.78,0.82,0.85,0.55,0.80,0.86,0.82,0.75,0.85,0.78,0.18,0.72,0.86,0.88,0.30,
                       0.06,0.52,0.78,0.38,0.72,0.85,0.78,0.30,0.68,0.82,0.84,0.72,0.38,0.76,0.86,0.88]),
        'src': 'Nd:glass laser lit.  f-f transitions',
    },
}

ORDER = ['Diamond','Water','Amber','Ruby','Gold',
         'Emerald','Amethyst','Sapphire','Copper','Obsidian',
         'Alexandrite','Malachite','Neodymium']

WL_PLOT = np.linspace(380, 780, 400)

def wl_to_rgb(wl):
    if   wl < 380: return (0.3,0.0,0.3)
    elif wl < 440: t=(wl-380)/60;  return (0.5*(1-t), 0,    0.5+0.3*t)
    elif wl < 490: t=(wl-440)/50;  return (0,          t,   1.0)
    elif wl < 510: t=(wl-490)/20;  return (0,          1.0, 1-t)
    elif wl < 580: t=(wl-510)/70;  return (t,          1.0, 0)
    elif wl < 645: t=(wl-580)/65;  return (1.0,        1-t, 0)
    else:          t=min((wl-645)/135,1); return (1.0,  0,   0)

def draw_bg(ax):
    for i in range(len(WL_PLOT)-1):
        r,g,b = wl_to_rgb(WL_PLOT[i])
        ax.axvspan(WL_PLOT[i], WL_PLOT[i+1],
                   color=(r*0.22, g*0.22, b*0.22), linewidth=0)

fig = plt.figure(figsize=(24, 15), facecolor=BG)
fig.text(0.5, 0.987,
         'Elements  ·  Material Spectral Curves  v2  (380–780 nm)'
         '   —   all 32 pts  |  raw values, no transform',
         ha='center', va='top', color=TEXT_COL,
         fontsize=12, fontweight='bold', fontfamily='monospace')

gs = gridspec.GridSpec(3, 5, figure=fig,
                       hspace=0.65, wspace=0.12,
                       left=0.03, right=0.98, top=0.94, bottom=0.04)

panel_pos = [(r,c) for r in range(3) for c in range(5) if not (r==2 and c>=3)]
axes = [fig.add_subplot(gs[r,c]) for r,c in panel_pos]
ax_all = fig.add_subplot(gs[2, 3:5])

for idx, name in enumerate(ORDER):
    ax  = axes[idx]
    mat = materials[name]
    col = ACCENT[name]

    T_raw = np.array(mat['T'])
    T_i   = np.interp(WL_PLOT, WL32, T_raw)

    draw_bg(ax)
    ax.fill_between(WL_PLOT, T_i, alpha=0.18, color=col)
    ax.plot(WL_PLOT, T_i, color=col, linewidth=1.6)
    ax.scatter(WL32, T_raw, color=col, s=5, zorder=5, alpha=0.6)

    ax.set_facecolor(PANEL_BG)
    ax.set_xlim(380,780); ax.set_ylim(-0.02,1.08)
    ax.set_xticks([400,500,600,700]); ax.set_yticks([0,0.5,1])
    ax.tick_params(colors=TEXT_COL, labelsize=6)
    for sp in ax.spines.values(): sp.set_color(GRID_COL)
    ax.grid(True, color=GRID_COL, linewidth=0.5, alpha=0.7)
    ax.set_title(name, color=col, fontsize=9, fontweight='bold',
                 fontfamily='monospace', pad=3)
    ax.text(0.5, -0.26, mat['src'], transform=ax.transAxes,
            ha='center', va='top', fontsize=4.6, color=DIM_COL,
            fontfamily='monospace')

# All Materials overview
draw_bg(ax_all)
for name in ORDER:
    T_i = np.interp(WL_PLOT, WL32, np.array(materials[name]['T']))
    ax_all.plot(WL_PLOT, T_i, color=ACCENT[name], linewidth=1.1, alpha=0.88)
ax_all.set_facecolor(PANEL_BG)
ax_all.set_xlim(380,780); ax_all.set_ylim(-0.02,1.08)
ax_all.set_xticks([400,500,600,700]); ax_all.set_yticks([0,0.5,1])
ax_all.tick_params(colors=TEXT_COL, labelsize=6)
for sp in ax_all.spines.values(): sp.set_color(GRID_COL)
ax_all.grid(True, color=GRID_COL, linewidth=0.5, alpha=0.7)
ax_all.set_title('All Materials  (raw)', color=TEXT_COL,
                  fontsize=9, fontweight='bold', fontfamily='monospace', pad=3)

OUT = '/Users/matiasderose/Documents/JUCE_Projects/Elements/elements_spectra_v2.png'
plt.savefig(OUT, dpi=150, bbox_inches='tight', facecolor=BG)
plt.close()
print(f"Saved: {OUT}")
