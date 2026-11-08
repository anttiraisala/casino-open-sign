#!/usr/bin/env python3
"""Draw the indicative layout of the stick arrow (names, colours, DIN ends, shield connectors) as an A4 SVG.

    python3 tools/generate_arrow_layout.py

Output: arrow_sticks/layout-diagram.svg (the PDF is made by tools/export_pdf.py). For exact dimensions use the drill
template (drill-template-A3.pdf), which is made from the same geometry (tools/arrow_geometry.py).
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, GREY, Svg
import arrow_geometry as geo

OUTPUT = Path(__file__).resolve().parent.parent / "arrow_sticks" / "layout-diagram.svg"
S = 2.75        # drawing units per mm (indicative scale, about 1:1.3 on A4 landscape)


def poly(pts, fill, stroke, width=1.4):
    return "<path d='M %s Z' fill='%s' stroke='%s' stroke-width='%g'/>" % (
        " L ".join("%.2f %.2f" % (x * S, y * S) for x, y in pts), fill, stroke, width)


def main():
    sticks, w, h = geo.shifted(geo.build())
    svg = Svg(1100, 780)
    svg.text(550, 30, "Stick arrow - layout (indicative; exact dimensions: drill-template-A3.pdf)", size=18,
             anchor="middle", weight="bold")
    svg.text(550, 49, "10 Grove RGB LED sticks, each 83 x 10 mm with 74 x 4 mm LED window, 2 mm between the sticks; "
             "arrow %.1f x %.1f mm" % (w, h), size=12, anchor="middle", color=GREY)
    ox, oy = (1100 - w * S) / 2, 100.0
    g = svg.items
    for s in sticks:
        parts = [poly(s.body(), s.fill, s.stroke), poly(s.window(), "#ffffff", s.stroke, 1.0)]
        for k in range(1, geo.LEDS + 1):
            x, y = s.led_centre(k)
            parts.append("<circle cx='%.2f' cy='%.2f' r='1.6' fill='%s'/>" % (x * S, y * S, s.stroke))
        dx = s.cx + s.din_dir[0] * (geo.BODY_L / 2 - 2.5)
        dy = s.cy + s.din_dir[1] * (geo.BODY_L / 2 - 2.5)
        parts.append("<circle cx='%.2f' cy='%.2f' r='3.2' fill='#000000'/>" % (dx * S, dy * S))
        parts.append("<circle cx='%.2f' cy='%.2f' r='2.4' fill='#ffffff' stroke='#000000' stroke-width='1'/>" % (s.cx * S, s.cy * S))
        g.append("<g transform='translate(%.2f %.2f)'>%s</g>" % (ox, oy, "".join(parts)))
    # names: leader text placed outside the sticks (above or below the arrow) with a thin line to the stick
    labels = {
        "L-WING-UP": (-1, 0), "R-WING-UP": (1, 0), "L-WING-DN": (-1, 1), "R-WING-DN": (1, 1),
        "L-CAP": (0, 0), "R-CAP": (0, 0), "RAIL-UP-L": (0, 0), "RAIL-UP-R": (0, 0), "RAIL-DN-L": (0, 1), "RAIL-DN-R": (0, 1),
    }
    for s in sticks:
        above = labels[s.name][1] == 0
        if s.name in ("L-CAP", "R-CAP"):
            ly = oy + (-14 if True else 0)
            lx = ox + s.cx * S
            svg.text(lx, ly, "%s  (%s)" % (s.name, s.connector), size=12, anchor="middle", weight="bold")
            svg.line(lx, ly + 4, lx, oy + (s.cy - geo.BODY_L / 2) * S - 2, GREY, 1)
        elif "RAIL" in s.name:
            x = ox + s.cx * S
            if above:
                svg.text(x, oy + (s.cy - 12) * S - 6, "%s  (%s)" % (s.name, s.connector), size=12, anchor="middle", weight="bold")
            else:
                svg.text(x, oy + (s.cy + 12) * S + 12, "%s  (%s)" % (s.name, s.connector), size=12, anchor="middle", weight="bold")
        else:
            x = ox + s.cx * S
            if above:
                y = oy + (s.cy - 22) * S - 8 if "L-" in s.name or "R-" in s.name else 0
                svg.text(x, oy - 14, "%s  (%s)" % (s.name, s.connector), size=12, anchor="middle", weight="bold")
                svg.line(x, oy - 10, x, oy + (s.cy - 6) * S, GREY, 1)
            else:
                yy = oy + h * S + 22
                svg.text(x, yy, "%s  (%s)" % (s.name, s.connector), size=12, anchor="middle", weight="bold")
                svg.line(x, yy - 14, x, oy + (s.cy + 6) * S, GREY, 1)
    # legend / table
    ty = oy + h * S + 70
    svg.text(40, ty, "Stick names, positions and Grove Base Shield connectors", size=14, weight="bold")
    for i, s in enumerate(sticks):
        col, row = i // 5, i % 5
        x = 40 + col * 520
        y = ty + 26 + row * 22
        svg.rect(x, y - 11, 16, 12, fill=s.fill, stroke=s.stroke)
        svg.text(x + 24, y, "%-10s %-4s %s" % (s.name, s.connector, s.description), size=13)
    ly = ty + 26 + 5 * 22 + 8
    svg.items.append("<circle cx='46' cy='%g' r='4' fill='#000000'/>" % (ly - 4))
    svg.text(58, ly, "Black dot = the ball of the stick: mount each stick with its ball here (left half: dots left, right half: dots right, caps: up).", size=12)
    svg.text(58, ly + 18, "Small circle = middle of the stick (Grove connector, cable hole).", size=12)
    svg.text(40, ly + 38, "Connector A3 of the shield carries the CLOSED light stick (not part of the arrow).", size=12, color=GREY)
    svg.text(40, ly + 56, "The sign sketch drives the arrow; this drawing and the wiring only fix the names and the connectors.", size=12, color=GREY)
    svg.height = ly + 70
    OUTPUT.write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
