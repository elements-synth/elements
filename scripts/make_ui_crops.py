#!/usr/bin/env python3
"""Cut the UI screenshots used by docs/parameters.md.

  python3 scripts/make_ui_crops.py closed.png geometries.png materials.png

Three full-window screenshots of the plugin taken at the same size/position:
  closed      both accordion panels closed (Chorus, Lights, Filter, Amp Env, Output visible)
  geometries  GEOMETRIES panel open (Geo, Thickness, Deform, Noise, Freq, Rate)
  materials   MATERIALS panel open (MAT A/B, Blend, Mix, Detune, Depth)
Crop boxes are in the coordinates of a 1999-px-wide view of the screenshot; they are scaled to the real size.
"""
import os
import sys
from PIL import Image

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "docs", "assets", "images", "ui")
REF_W = 1999.0
CROPS = {  # name: (source, (x0, y0, x1, y1))
    "chorus":   ("closed",     (55, 232, 450, 460)),
    "lights":   ("closed",     (55, 1295, 1400, 1398)),
    "filter":   ("closed",     (1432, 535, 1946, 1050)),
    "amp-env":  ("closed",     (1432, 1065, 1946, 1275)),
    "deformer": ("geometries", (55, 228, 728, 575)),
    "blend":    ("materials",  (730, 228, 1405, 616)),
}


def main(closed, geometries, materials):
    src = {"closed": Image.open(closed).convert("RGB"), "geometries": Image.open(geometries).convert("RGB"),
           "materials": Image.open(materials).convert("RGB")}
    os.makedirs(OUT, exist_ok=True)
    k = src["closed"].width / REF_W
    for name, (s, box) in CROPS.items():
        im = src[s].crop(tuple(int(round(v * k)) for v in box))
        im.save(os.path.join(OUT, name + ".png"), optimize=True)
        print(name, im.size)
    # Output strip: octave selector + volume knob side by side
    c = src["closed"]
    a = c.crop(tuple(int(round(v * k)) for v in (40, 1425, 175, 1520)))
    b = c.crop(tuple(int(round(v * k)) for v in (1432, 1288, 1946, 1415)))
    h = max(a.height, b.height)
    out = Image.new("RGB", (a.width + 24 + b.width, h), (13, 17, 23))
    out.paste(a, (0, 0)); out.paste(b, (a.width + 24, 0))
    out.save(os.path.join(OUT, "output.png"), optimize=True)
    print("output", out.size)
    full = src["closed"].resize((1600, round(src["closed"].height * 1600 / src["closed"].width)), Image.LANCZOS)
    full.save(os.path.join(OUT, "overview.png"), optimize=True)
    print("overview", full.size)


if __name__ == "__main__":
    main(*sys.argv[1:4])
