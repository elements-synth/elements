#!/usr/bin/env python3
"""Render an Elements-style intro card PNG.
  python3 scripts/video/make_intro_card.py out.png "The Deformer" "Subtitle" [version] [image.png|-] [W H]
"""
import sys
from PIL import Image, ImageDraw, ImageFont
BG0, BG1, BORDER, MID, TEXT, ACCENT, AMETHYST = (13,17,23),(17,24,32),(31,45,61),(74,96,117),(208,228,240),(168,216,240),(181,123,238)
out, title, sub = sys.argv[1:4]
ver = sys.argv[4] if len(sys.argv) > 4 else "v1.0.0"
pic = sys.argv[5] if len(sys.argv) > 5 and sys.argv[5] != "-" else None
W, H = (int(sys.argv[6]), int(sys.argv[7])) if len(sys.argv) > 7 else (1428, 1080)
img = Image.new("RGB", (W, H), BG0)
px = img.load()
for y in range(H):                       # same top-to-bottom gradient as the docs
    t = y / (H - 1)
    c = tuple(int(BG1[i] + (BG0[i] - BG1[i]) * t) for i in range(3))
    for x in range(W): px[x, y] = c
d = ImageDraw.Draw(img)
logo = Image.open("Source/elements-logo.png").convert("RGBA")
lw = int(W * 0.31); logo = logo.resize((lw, int(logo.height * lw / logo.width)), Image.LANCZOS)
bold = ImageFont.truetype("Source/JetBrainsMono-Bold.ttf", int(W * 0.0235))
reg = ImageFont.truetype("Source/JetBrainsMono-Regular.ttf", int(W * 0.012))
verf = ImageFont.truetype("Source/JetBrainsMono-Regular.ttf", int(W * 0.0105))
gap1, gap2, gap3, rule = int(H * 0.025), int(H * 0.06), int(H * 0.025), int(H * 0.04)
if pic:
    pi = Image.open(pic).convert("RGBA"); pi = pi.crop(pi.getbbox())
    ph = int(H * 0.30); pi = pi.resize((int(pi.width * ph / pi.height), ph), Image.LANCZOS)
    gap_pic = int(H * 0.04)
th, sh, vh = int(bold.size * 1.2), int(reg.size * 1.2), int(verf.size * 1.2)
total = logo.height + gap1 + vh + gap2 + (pi.height + gap_pic if pic else 0) + th + rule + gap3 + sh
y = (H - total) // 2
def centred(text, y, font, fill):
    d.text(((W - d.textlength(text, font=font)) / 2, y), text, font=font, fill=fill)
img.paste(logo, ((W - lw) // 2, y), logo); y += logo.height + gap1
centred(ver, y, verf, AMETHYST); y += vh + gap2
if pic:
    img.paste(pi, ((W - pi.width) // 2, y), pi); y += pi.height + gap_pic
centred(title, y, bold, AMETHYST); y += th + rule // 2
d.line([(W * 0.46, y), (W * 0.54, y)], fill=ACCENT, width=2); y += rule // 2 + gap3
centred(sub, y, reg, MID)
d.rectangle([14, 14, W - 15, H - 15], outline=BORDER, width=1)
img.save(out)
