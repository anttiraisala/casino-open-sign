#!/usr/bin/env python3
"""Draw the 1:1 drill template of the stick arrow as landscape PDFs (exact millimetres).

    python3 tools/generate_arrow_drill_template.py

Output (in arrow_sticks/):
  drill-template-A3.pdf        one A3 landscape page (420 x 297 mm), the whole arrow at 1:1
  drill-template-A4-tiled.pdf  two A4 landscape pages (297 x 210 mm), 1:1; one half of the arrow on each page, cut at the marked line and join
Print at 100 % ("actual size", no fit to page) and check the 100 mm scale bar with a ruler.
"""

import sys
from pathlib import Path

from reportlab.lib.colors import HexColor, black, white
from reportlab.lib.units import mm
from reportlab.pdfgen import canvas

sys.path.insert(0, str(Path(__file__).resolve().parent))
import arrow_geometry as g

OUT = Path(__file__).resolve().parent.parent / "arrow_sticks"
THIN = 0.25 * mm


def draw_poly(c, pts, fill=None, stroke=black, width=THIN):
    p = c.beginPath()
    p.moveTo(pts[0][0] * mm, pts[0][1] * mm)
    for x, y in pts[1:]:
        p.lineTo(x * mm, y * mm)
    p.close()
    c.setLineWidth(width)
    if fill:
        c.setFillColor(HexColor(fill))
    c.setStrokeColor(stroke if not isinstance(stroke, str) else HexColor(stroke))
    c.drawPath(p, stroke=1, fill=1 if fill else 0)


def draw_arrow(c, sticks):
    """Draws the arrow in template coordinates (mm, y down) on a canvas whose origin is already at the arrow's
    top-left corner and whose y axis points DOWN (the caller flips it)."""
    for s in sticks:
        draw_poly(c, s.body(), fill=s.fill, stroke=s.stroke, width=0.35 * mm)
        draw_poly(c, s.window(), fill="#ffffff", stroke=s.stroke, width=THIN)
        # LED positions (small dots): 10 per stick
        c.setFillColor(HexColor(s.stroke))
        for k in range(1, g.LEDS + 1):
            x, y = s.led_centre(k)
            c.circle(x * mm, y * mm, 0.5 * mm, stroke=0, fill=1)
        # DIN end marker: filled circle 2.5 mm from the end, on the body outside the LED window
        din_x = s.cx + s.din_dir[0] * (g.BODY_L / 2 - 2.5)
        din_y = s.cy + s.din_dir[1] * (g.BODY_L / 2 - 2.5)
        c.setFillColor(black)
        c.circle(din_x * mm, din_y * mm, 1.1 * mm, stroke=0, fill=1)
        # centre mark: crosshair and cable hole circle (drilling position)
        c.setStrokeColor(black)
        c.setLineWidth(0.2 * mm)
        c.line((s.cx - 7) * mm, s.cy * mm, (s.cx + 7) * mm, s.cy * mm)
        c.line(s.cx * mm, (s.cy - 7) * mm, s.cx * mm, (s.cy + 7) * mm)
        c.setDash(1.2, 0.8)
        c.circle(s.cx * mm, s.cy * mm, g.CABLE_HOLE_D / 2 * mm, stroke=1, fill=0)
        c.setDash()
    # names, rotated along each stick, in the 3.5 mm band beside the LED window (towards the DIN end of the stick)
    import math
    for s in sticks:
        c.saveState()
        c.translate(s.cx * mm, s.cy * mm)
        ang = math.degrees(math.atan2(s.din_dir[1], s.din_dir[0]))
        # draw the text so that it reads from left to right: rotate by theta, flipped when upside down
        theta = s.theta % 360
        rot = theta if (theta <= 90 or theta > 270) else theta - 180
        c.rotate(rot)                       # (the canvas y axis points down, so positive angles turn clockwise)
        # offset along the stick axis towards the DIN end, in the text's own frame
        t = math.radians(rot)
        ux, uy = math.cos(t), math.sin(t)
        along = (s.din_dir[0] * ux + s.din_dir[1] * uy) * 20.0
        c.setFillColor(black)
        c.setFont("Helvetica-Bold", 6.5)
        c.scale(1, -1)                      # the text must not be mirrored by the flipped y axis
        c.drawCentredString(along * mm, -3.6 * mm, s.name)
        c.restoreState()


def draw_scale_bar(c, x, y):
    """100 mm bar with ticks every 10 mm (canvas in mm-based flipped frame: y down)."""
    c.setStrokeColor(black)
    c.setLineWidth(0.3 * mm)
    c.line(x * mm, y * mm, (x + 100) * mm, y * mm)
    for i in range(11):
        c.line((x + i * 10) * mm, y * mm, (x + i * 10) * mm, (y + (3 if i % 5 == 0 else 2)) * mm)
    c.setFillColor(black)
    c.saveState()
    c.scale(1, -1)
    c.setFont("Helvetica", 7)
    c.drawString(x * mm, -(y - 2) * mm, "0")
    c.drawRightString((x + 100) * mm, -(y - 2) * mm, "100 mm")
    c.restoreState()


def text(c, x, y, s, size=8, bold=False, align="l"):
    """Text at (x, y) mm in the y-down frame."""
    c.saveState()
    c.scale(1, -1)
    c.setFillColor(black)
    c.setFont("Helvetica-Bold" if bold else "Helvetica", size)
    {"l": c.drawString, "r": c.drawRightString, "c": c.drawCentredString}[align](x * mm, -y * mm, s)
    c.restoreState()


def table(c, sticks, x, y, step=4.2):
    text(c, x, y, "Drilling positions: centre of each stick = centre of the cable hole (mm from the top-left corner of the arrow)", 8, True)
    heads = ("Stick", "x", "y", "angle", "Grove connector on the shield")
    cols = (0, 38, 58, 78, 100)
    for h, cx in zip(heads, cols):
        text(c, x + cx, y + step, h, 7.5, True)
    for i, s in enumerate(sticks):
        row = (s.name, "%.2f" % s.cx, "%.2f" % s.cy, "%.0f" % (s.theta % 180), s.connector)
        for v, cx in zip(row, cols):
            text(c, x + cx, y + step * (i + 2), v, 7.5)


def notes(c, x, y, w, h, lines, step=4.0):
    for i, line in enumerate(lines):
        text(c, x, y + i * step, line, 7.5, i == 0)


def common_notes(w, h):
    return [
        "Stick arrow, drill template 1:1  (%.2f x %.2f mm)" % (w, h),
        "Print at 100 % (actual size, do NOT fit to page) and check that the scale bar below measures 100 mm.",
        "Each stick: outer body 83 x 10 mm, LED window 74 x 4 mm, 2 mm free space between sticks.",
        "Black dot = the ball of the stick (the round marker on its board). Mount each stick with its ball at the dot (left half: dots left, right half: dots right, caps: up).",
        "Cross + dashed circle = middle of the stick = drill the cable hole (dia. %.0f mm, check against your Grove connector) here." % g.CABLE_HOLE_D,
    ]


def page_a3(path):
    sticks, w, h = g.shifted(g.build())
    pw, ph = 420.0, 297.0
    c = canvas.Canvas(str(path), pagesize=(pw * mm, ph * mm))
    c.setTitle("Stick arrow - drill template 1:1 (A3 landscape)")
    ox, oy = (pw - w) / 2, 70.0
    c.translate(0, ph * mm)
    c.scale(1, -1)                          # from here: y down, units are millimetres via the `mm` factor
    notes(c, 20, 18, pw - 40, 40, common_notes(w, h))
    # arrow, with its bounding box and dimension lines
    c.saveState()
    c.translate(ox * mm, oy * mm)
    c.setStrokeColor(HexColor("#999999"))
    c.setLineWidth(0.15 * mm)
    c.setDash(2, 2)
    c.rect(0, 0, w * mm, h * mm, stroke=1, fill=0)
    c.setDash()
    draw_arrow(c, sticks)
    c.restoreState()
    text(c, ox + w / 2, oy - 4, "%.2f mm" % w, 8, False, "c")
    text(c, ox - 3, oy + h / 2, "%.2f mm" % h, 8, False, "r")
    draw_scale_bar(c, 20, 275)
    table(c, sticks, 150, 190)
    text(c, pw - 20, ph - 8, "casino-open-sign / arrow_sticks - generated by tools/generate_arrow_drill_template.py", 6, False, "r")
    c.showPage()
    c.save()


def page_a4_tiled(path):
    """Two A4 landscape pages, one half of the arrow on each. No stick crosses the middle line (it runs through the
    2 mm gap between the two rail bars), so each page carries its half completely. Cut each page along its dashed CUT
    LINE and butt the two cut edges together (crosses at both ends of the line must meet) or overlap the pages so that
    the two lines coincide."""
    sticks, w, h = g.shifted(g.build())
    pw, ph = 297.0, 210.0
    margin = 15.0
    join_x = w / 2                                  # the cut line, in arrow coordinates
    c = canvas.Canvas(str(path), pagesize=(pw * mm, ph * mm))
    c.setTitle("Stick arrow - drill template 1:1 (A4 landscape, 2 pages, half an arrow on each)")
    oy = 48.0
    for page in (1, 2):
        c.translate(0, ph * mm)
        c.scale(1, -1)
        # page 1 shows the left half with the arrow's left end at the left margin;
        # page 2 shows the right half with the arrow's right end at the right margin
        ox = margin if page == 1 else pw - margin - w
        x0, x1 = (0.0, join_x) if page == 1 else (join_x, w)      # arrow x range drawn on this page
        c.saveState()
        clip = c.beginPath()
        clip.rect((ox + x0 - 0.0) * mm, 0, (x1 - x0) * mm, ph * mm)
        c.clipPath(clip, stroke=0, fill=0)
        c.translate(ox * mm, oy * mm)
        c.setStrokeColor(HexColor("#999999"))
        c.setLineWidth(0.15 * mm)
        c.setDash(2, 2)
        c.rect(0, 0, w * mm, h * mm, stroke=1, fill=0)
        c.setDash()
        draw_arrow(c, sticks)
        c.restoreState()
        # cut line (dashed) with registration crosses at both ends and in the middle
        lx = ox + join_x
        c.setStrokeColor(black)
        c.setLineWidth(0.3 * mm)
        c.setDash(3, 2)
        c.line(lx * mm, (oy - 12) * mm, lx * mm, (oy + h + 12) * mm)
        c.setDash()
        for yy in (oy - 8, oy + h / 2, oy + h + 8):
            c.line((lx - 4) * mm, yy * mm, (lx + 4) * mm, yy * mm)
        side = "right" if page == 1 else "left"
        text(c, lx + (-2 if page == 1 else 2), oy - 14,
             "CUT LINE (page %d of 2): cut along it and discard the %s part" % (page, side), 6, True, "r" if page == 1 else "l")
        text(c, lx + (-2 if page == 1 else 2), oy - 10.5,
             "Butt the two cut edges together (the crosses must meet) and tape.", 6, False, "r" if page == 1 else "l")
        notes(c, margin, 10, pw - 2 * margin, 25, common_notes(w, h)[:4], 3.6)
        text(c, margin, 40, "This page: the %s half of the arrow (arrow x %.2f ... %.2f mm)." % ("LEFT" if page == 1 else "RIGHT", x0, x1), 7.5, True)
        draw_scale_bar(c, margin, oy + h + 24)
        if page == 1:
            table(c, sticks, margin + 120, oy + h + 17, 3.6)
        text(c, pw - margin, ph - 4, "page %d of 2" % page, 7, True, "r")
        c.showPage()
    c.save()


def main():
    OUT.mkdir(exist_ok=True)
    page_a3(OUT / "drill-template-A3.pdf")
    page_a4_tiled(OUT / "drill-template-A4-tiled.pdf")
    print("Wrote", OUT / "drill-template-A3.pdf", "and", OUT / "drill-template-A4-tiled.pdf")


if __name__ == "__main__":
    main()
