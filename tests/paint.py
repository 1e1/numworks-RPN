#!/usr/bin/env python3
"""Paint layout draw-ops (from render_host / render_screen_host) into a PNG.

Geometry comes from the C++ engine (authoritative); this only paints text and
rectangles with an approximate mono font so a human can eyeball the result.
Two modes:
  - "CANVAS W H" header  -> one fixed-size image (full screen)
  - "ITEM ..." blocks    -> stacked labelled previews
Ops: `T x y large fg bg text` and `R x y w h color` (colours as 6 hex digits)."""
import sys
from PIL import Image, ImageDraw, ImageFont

def font(size):
    for p in ("/System/Library/Fonts/Menlo.ttc",
              "/System/Library/Fonts/Supplemental/Andale Mono.ttf",
              "/Library/Fonts/Arial Unicode.ttf"):
        try:
            return ImageFont.truetype(p, size)
        except Exception:
            pass
    return ImageFont.load_default()

FL, FS = font(16), font(12)

def col(h):
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))

def paint_ops(d, ops, ox, oy):
    for op in ops:
        if op[0] == "T":
            _, x, y, large, fg, bg, s = op
            d.text((ox + x, oy + y - 1), s, font=(FL if large else FS), fill=fg)
        else:
            _, x, y, w, h, c = op
            d.rectangle([ox + x, oy + y, ox + x + w, oy + y + h], fill=c)

def parse(line):
    p = line.split()
    if p[0] == "T":
        return ("T", int(p[1]), int(p[2]), p[3] == "1", col(p[4]), col(p[5]),
                line.split(None, 6)[6].rstrip("\n"))
    return ("R", int(p[1]), int(p[2]), int(p[3]), int(p[4]), col(p[5]))

lines = [l for l in sys.stdin if l.strip()]

if lines and lines[0].startswith("CANVAS"):
    _, W, H = lines[0].split()
    W, H = int(W), int(H)
    img = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    paint_ops(d, [parse(l) for l in lines[1:]], 0, 0)
    img.save("/tmp/render.png")
    img.resize((W * 2, H * 2), Image.NEAREST).save("/tmp/render_2x.png")
    print("wrote /tmp/render.png (screen %dx%d)" % (W, H))
else:
    items, cur = [], None
    for l in lines:
        p = l.split()
        if p[0] == "ITEM":
            cur = [p[1], int(p[2]), int(p[3]) + int(p[4]), []]
        elif p[0] == "END":
            items.append(cur); cur = None
        else:
            cur[3].append(parse(l))
    PAD, LABELW, GAP = 12, 130, 14
    rowh = [max(it[2], 22) + GAP for it in items]
    W = LABELW + max((it[1] for it in items), default=100) + 3 * PAD
    H = sum(rowh) + PAD
    img = Image.new("RGB", (W, H), (250, 250, 248))
    d = ImageDraw.Draw(img)
    y = PAD
    for (label, w, h, ops), rh in zip(items, rowh):
        d.text((PAD, y + rh // 2 - 8), label, font=FS, fill=(150, 150, 155))
        d.rectangle([LABELW - 6, y - 4, W - PAD, y + h + 4], outline=(230, 230, 228))
        paint_ops(d, ops, LABELW, y)
        y += rh
    img.save("/tmp/render.png")
    img.resize((W * 2, H * 2), Image.NEAREST).save("/tmp/render_2x.png")
    print("wrote /tmp/render.png (%dx%d, %d items)" % (W, H, len(items)))
