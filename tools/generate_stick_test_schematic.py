#!/usr/bin/env python3
"""Draw the wiring of the stick test (stick_test/sketch.ino) as an A4 SVG schematic.

    python3 tools/generate_stick_test_schematic.py

Output: stick_test/wiring-diagram.svg (the PDF is made by tools/export_pdf.py).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, ORANGE, RED, GREY, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "stick_test" / "wiring-diagram.svg"

PINS = ["D2", "D3", "D4", "D5", "D6", "D7", "D8", "A0", "A1", "A2"]    # data pins of sticks 1..10
ROLES = ["left cap", "left upper wing", "left lower wing", "top rail, left", "top rail, right",
         "bottom rail, left", "bottom rail, right", "right cap", "right upper wing", "right lower wing"]


def capacitor(svg, x, y_top, y_bottom, label):
    """Polarised capacitor between a +5 V wire at y_top and a 0 V wire at y_bottom (vertical, plus on top)."""
    mid = (y_top + y_bottom) / 2
    svg.line(x, y_top, x, mid - 6, RED)
    svg.line(x - 12, mid - 6, x + 12, mid - 6, BLACK, 2.5)
    svg.line(x - 12, mid + 6, x + 12, mid + 6, BLACK, 2.5)
    svg.line(x, mid + 6, x, y_bottom, BLACK)
    svg.text(x + 16, mid - 8, "+", size=13, color=RED, weight="bold")
    svg.text(x - 16, mid + 4, label, size=11, anchor="end")


def main():
    width = 940
    svg = Svg(width, 1160)
    svg.text(width / 2, 16, "Stick test - wiring (Arduino Mega + 10 Grove RGB LED sticks on a separate 5 V supply)",
             size=16, anchor="middle", weight="bold")

    v5_x, gnd_x = 840, 880
    rail5_y, railg_y = 72, 112

    # ---- 5 V supply, fuse, capacitor, rails ------------------------------------------
    svg.rect(520, 44, 130, 100, fill="#f3f3f3")
    svg.text(585, 70, "Mean Well", size=12, anchor="middle")
    svg.text(585, 88, "LRS-50-5", size=13, anchor="middle", weight="bold")
    svg.text(585, 105, "5 V / 10 A", size=12, anchor="middle")
    svg.text(585, 122, "PSU1", size=11, anchor="middle", color=GREY)
    for y, label in ((60, "L"), (88, "N"), (116, "PE")):
        svg.line(482, y, 520, y, BLACK)
        svg.text(476, y + 4, label, size=12, anchor="end")
    svg.text(585, 40, "230 V AC mains", size=12, anchor="middle", weight="bold")
    svg.text(655, rail5_y - 6, "V+", size=11)
    svg.text(655, railg_y + 14, "V-", size=11)
    svg.line(650, rail5_y, 670, rail5_y, RED, 2.5)
    svg.rect(670, rail5_y - 8, 50, 16, fill="#ffffff", stroke=RED)
    svg.text(695, rail5_y - 14, "F1  10 A fuse", size=12, anchor="middle", color=RED)
    svg.line(720, rail5_y, v5_x, rail5_y, RED, 2.5)
    svg.text(v5_x + 10, rail5_y + 4, "+5 V", size=14, color=RED, weight="bold")
    svg.hline_hop(650, gnd_x, railg_y, [v5_x], BLACK, 2.5)
    svg.text(gnd_x + 10, railg_y + 4, "0 V", size=14, weight="bold")
    capacitor(svg, 780, rail5_y, railg_y, "C1  1000 uF 25 V")
    svg.dot(780, rail5_y, RED, 4)
    svg.dot(780, railg_y, BLACK, 4)

    # ---- Arduino ------------------------------------------------------------------------
    row0, dy = 190, 60
    last_y = row0 + 9 * dy
    d22_y, gnd_y = last_y + 120, last_y + 170
    svg.rect(60, 150, 180, gnd_y - 150 + 50, fill="#e8f0fe")
    svg.text(150, 176, "Arduino Mega 2560", size=15, anchor="middle", weight="bold")
    svg.text(150, 194 + 0, "+ Grove Base Shield", size=11, anchor="middle", color=GREY)
    svg.text(150, 210 + 0, "powered from USB only", size=11, anchor="middle", color=GREY)

    for i in range(10):
        yc = row0 + i * dy + 30
        svg.text(232, yc + 4, PINS[i], size=12, anchor="end")
        svg.line(240, yc, 390, yc, BLUE)
        svg.resistor(390, yc, "R%d" % (i + 1))
        svg.line(450, yc, 520, yc, BLUE)
        svg.circle(520, yc, 3.5, BLUE)
        # stick block
        svg.rect(520, yc - 24, 220, 48, fill="#fff8e1")
        svg.text(530, yc - 6, "Stick %d - %s" % (i + 1, ROLES[i]), size=12, weight="bold")
        svg.text(530, yc + 12, "yellow = DIN   red = +5 V   black = GND", size=10, color=GREY)
        svg.text(516, yc - 8, "DIN", size=10, anchor="end", color=BLUE)
        svg.circle(740, yc - 10, 3.5, RED)
        svg.circle(740, yc + 10, 3.5, BLACK)
        svg.line(740, yc - 10, v5_x, yc - 10, RED, 2)
        svg.dot(v5_x, yc - 10, RED, 3.5)
        svg.hline_hop(740, gnd_x, yc + 10, [v5_x], BLACK, 2)
        svg.dot(gnd_x, yc + 10, BLACK, 3.5)
    svg.line(v5_x, rail5_y, v5_x, row0 + 9 * dy + 20, RED, 2.5)
    svg.line(gnd_x, railg_y, gnd_x, gnd_y, BLACK, 2.5)

    # ---- optional button on D22 -----------------------------------------------------------
    svg.text(232, d22_y + 4, "D22", size=12, anchor="end")
    svg.line(240, d22_y, 330, d22_y, ORANGE)
    svg.circle(330, d22_y, 3.5, ORANGE)
    svg.line(334, d22_y - 12, 416, d22_y - 12, BLACK, 2.5)
    svg.line(375, d22_y - 12, 375, d22_y - 26, BLACK, 2)
    svg.circle(420, d22_y, 3.5, ORANGE)
    svg.line(420, d22_y, 440, d22_y, ORANGE)
    svg.line(440, d22_y, 440, gnd_y, ORANGE)
    svg.dot(440, gnd_y, BLACK, 4)
    svg.text(330, d22_y - 36, "optional push button (skip phase)", size=11, weight="bold")

    # ---- the one ground link ---------------------------------------------------------------
    svg.text(232, gnd_y + 4, "GND", size=12, anchor="end")
    svg.line(240, gnd_y, gnd_x, gnd_y, BLACK, 2.5)
    svg.dot(gnd_x, gnd_y, BLACK, 4)
    svg.text(560, gnd_y - 8, "the only ground link: Arduino GND to the 0 V rail", size=11, color=GREY)

    notes = [
        "Connections",
        "1. The sticks are powered from the separate 5 V supply PSU1 through the 10 A fuse F1. The Arduino 5V pin does NOT power the sticks.",
        "2. Each stick: red wire = +5 V rail, black wire = 0 V rail (thick wire for the rails). Only the yellow wire goes to the Arduino.",
        "3. Take the red and black wire terminals out of the shield-side Grove connector of every stick cable (see README.txt),",
        "   so that the shield and the stick supply are not connected to each other through the cables.",
        "4. Data: Arduino pin -> 330 ohm resistor (recommended) -> yellow wire (DIN). Stick n goes to the n-th pin: D2..D8, A0..A2.",
        "5. The Arduino and the 5 V supply share ground through ONE wire: Arduino GND to the 0 V rail.",
        "6. C1: 1000 uF, 25 V electrolytic capacitor across the rails (plus = +5 V, stripe = minus), close to the sticks.",
        "7. Switch the 5 V supply ON first, then plug in the USB cable. The USB cable powers only the Arduino.",
        "8. Optional push button between D22 and GND: skips to the next phase.",
        "9. The mains side of the supply must be in an enclosure and wired by a person who knows how (nothing here is mains).",
    ]
    base = gnd_y + 90
    for k, line in enumerate(notes):
        svg.text(40, base + k * 19, line, size=12, weight="bold" if k == 0 else "normal")
    svg.height = base + len(notes) * 19 + 20
    OUTPUT.write_text(svg.render_a4(landscape=False))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
