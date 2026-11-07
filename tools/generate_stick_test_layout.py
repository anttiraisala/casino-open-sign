#!/usr/bin/env python3
"""Draw the layout of the 10-stick arrow used in the stick test (numbers, pins, dimensions).

    python3 tools/generate_stick_test_layout.py

Output: stick_test/layout-diagram.svg (A4 landscape; the PDF is made by tools/export_pdf.py).
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREY, RED, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "stick_test" / "layout-diagram.svg"
OUTPUT_DIRECT = Path(__file__).resolve().parent.parent / "stick_test_direct" / "layout-diagram.svg"
S = 3.0                      # drawing units per mm
STICK_LEN, STICK_W = 80.0, 10.0
RAIL_GAP = 30.0              # gap between the top and the bottom rail (mm), centre to centre = 40
PINS = ["D2", "D3", "D4", "D5", "D6", "D7", "D8", "A0", "A1", "A2"]
FILL = {1: "#ffcdd2", 8: "#ffcdd2", 4: "#ffcdd2", 5: "#ffcdd2", 6: "#ffcdd2", 7: "#ffcdd2"}


def stick(svg, cx, cy, angle_deg, n):
    """Draw one stick (80 x 10 mm) centred at (cx, cy) mm, rotated by angle_deg; 10 LEDs along it."""
    a = math.radians(angle_deg)
    ux, uy = math.cos(a), -math.sin(a)          # along the stick (y axis points down on screen)
    vx, vy = -uy, ux                             # across
    pts = []
    for sl, sw in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        pts.append((cx + sl * STICK_LEN / 2 * ux + sw * STICK_W / 2 * vx,
                    cy + sl * STICK_LEN / 2 * uy + sw * STICK_W / 2 * vy))
    svg.path("M " + " L ".join("%g %g" % (X * S, Y * S) for X, Y in pts) + " Z",
             fill=FILL.get(n, "#fff8e1"), color="#546e7a", width=1.6)
    for k in range(10):
        t = (k - 4.5) * STICK_LEN / 10
        svg.circle((cx + t * ux) * S, (cy + t * uy) * S, 2.6, "#78909c")
    svg.text((cx + 0) * S, (cy) * S + 5, str(n), size=18, anchor="middle", weight="bold", color=BLACK)
    # data-in end marker: LED 1 end (left end for the horizontal sticks, top for the vertical ones)
    svg.circle((cx - STICK_LEN / 2 * ux) * S, (cy - STICK_LEN / 2 * uy) * S, 5, BLUE)


def build(direct):
    svg = Svg(1100, 780)
    svg.text(550, 32, "Stick test - layout of the 10-stick arrow (design B, barbed head)", size=19, anchor="middle",
             weight="bold")
    svg.text(550, 50, "The test needs only the 10 sticks connected; they do not have to be fixed in this shape.",
             size=12, anchor="middle", color=GREY)

    ox, oy = 130, 130                    # offset of the drawing in drawing units
    g = svg.items
    # coordinates in mm, origin at the arrow's left tip; arrow centre line at y = 45
    w30 = math.cos(math.radians(30)) * STICK_LEN
    h30 = math.sin(math.radians(30)) * STICK_LEN
    cx0 = w30                           # left cap x
    cap_x_l, cap_x_r = cx0, cx0 + 2 * STICK_LEN
    yc = 45.0
    sticks = [
        (1, cap_x_l, yc, 90),
        (2, cap_x_l - w30 / 2, yc - h30 / 2, 30),
        (3, cap_x_l - w30 / 2, yc + h30 / 2, -30),
        (4, cap_x_l + 5 + STICK_LEN / 2, yc - RAIL_GAP / 2 - STICK_W / 2, 0),
        (5, cap_x_l + 5 + 1.5 * STICK_LEN + 2, yc - RAIL_GAP / 2 - STICK_W / 2, 0),
        (6, cap_x_l + 5 + STICK_LEN / 2, yc + RAIL_GAP / 2 + STICK_W / 2, 0),
        (7, cap_x_l + 5 + 1.5 * STICK_LEN + 2, yc + RAIL_GAP / 2 + STICK_W / 2, 0),
        (8, cap_x_r + 12, yc, 90),
        (9, cap_x_r + 12 + w30 / 2, yc - h30 / 2, -30),
        (10, cap_x_r + 12 + w30 / 2, yc + h30 / 2, 30),
    ]
    sub = Svg(1100, 780)
    for n, x, y, ang in sticks:
        stick(sub, x, y, ang, n)
    for item in sub.items:               # shift into place
        g.append("<g transform='translate(%g %g)'>%s</g>" % (ox, oy, item))

    # dimension line
    total_w = cap_x_r + 12 + w30
    yd = (yc + 60) * S + oy
    svg.line(ox, yd, ox + total_w * S, yd, BLACK, 1.4)
    svg.text(ox + total_w * S / 2, yd + 18, "about %d cm wide, 9 cm high (the exact size depends on the gaps)"
             % round(total_w / 10), size=12, anchor="middle")
    svg.text(ox, yd + 40, "Blue dot = data-in end (LED 1) of the stick. Each stick: 80 x 10 x 10 mm, 10 LEDs.",
             size=12)

    # pin table
    ty = yd + 78
    svg.text(24, ty, "Stick -> Arduino data pin (sketch order)", size=14, weight="bold")
    roles = ["left cap", "left upper wing", "left lower wing", "top rail, left", "top rail, right",
             "bottom rail, left", "bottom rail, right", "right cap", "right upper wing", "right lower wing"]
    for i in range(10):
        col, row = i // 5, i % 5
        svg.text(40 + col * 520, ty + 26 + row * 22, "Stick %2d  %-4s  %s" % (i + 1, PINS[i], roles[i]), size=13,
                 color=BLACK)
    svg.text(24, ty + 160,
             "Wings are at 30 degrees: with 80 mm sticks the wing spans 69 mm along the shaft and 40 mm across it.",
             size=12, color=GREY)
    if direct:
        svg.text(24, ty + 182, "Stick 11  A3   spare, not part of the arrow: the Grove Base Shield for the Mega has 11 connectors with a data pin,",
                 size=12)
        svg.text(24, ty + 200, "so this test also drives an 11th stick if you have one (set ACTIVE_STICKS in sketch.ino).", size=12)
    svg.height = ty + (225 if direct else 190)
    return svg.render_a4(landscape=True)


def main():
    OUTPUT.write_text(build(False))
    print("Wrote %s" % OUTPUT)
    # the direct stick test (stick_test_direct) uses the same arrow and pins plus a spare 11th stick
    OUTPUT_DIRECT.write_text(build(True).replace("Stick test - layout", "Direct stick test - layout"))
    print("Wrote %s" % OUTPUT_DIRECT)


if __name__ == "__main__":
    main()
