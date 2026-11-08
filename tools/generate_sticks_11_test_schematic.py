#!/usr/bin/env python3
"""Draw the wiring of the 11 sticks test (sticks_11_test/sketch.ino) as an A4 SVG schematic.

    python3 tools/generate_sticks_11_test_schematic.py

Output: sticks_11_test/wiring-diagram.svg (the PDF is made by tools/export_pdf.py).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, ORANGE, RED, GREY, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "sticks_11_test" / "wiring-diagram.svg"
CONN = ["D2", "D3", "D4", "D5", "D6", "D7", "D8", "A0", "A1", "A2", "A3"]
NAMES = ["L-CAP", "L-WING-UP", "L-WING-DN", "RAIL-UP-L", "RAIL-UP-R", "RAIL-DN-L", "RAIL-DN-R", "R-CAP", "R-WING-UP", "R-WING-DN", "CLOSED light"]


def main():
    width = 940
    svg = Svg(width, 1100)
    svg.text(width / 2, 16, "11 sticks - wiring (10 arrow sticks + the CLOSED light stick in the Grove Base Shield)",
             size=15, anchor="middle", weight="bold")

    # PSU, fuse, capacitor
    svg.text(150, 40, "230 V AC mains", size=12, anchor="middle", weight="bold")
    svg.rect(85, 48, 130, 100, fill="#f3f3f3")
    svg.text(150, 74, "Mean Well", size=12, anchor="middle")
    svg.text(150, 92, "LRS-50-5", size=13, anchor="middle", weight="bold")
    svg.text(150, 109, "5 V / 10 A", size=12, anchor="middle")
    svg.text(150, 126, "PSU1", size=11, anchor="middle", color=GREY)
    for y, label in ((64, "L"), (92, "N"), (120, "PE")):
        svg.line(47, y, 85, y, BLACK)
        svg.text(41, y + 4, label, size=12, anchor="end")
    rail5, railg = 76, 116
    svg.line(215, rail5, 260, rail5, RED, 2.5)
    svg.rect(260, rail5 - 8, 50, 16, fill="#ffffff", stroke=RED)
    svg.text(285, rail5 - 14, "F1  10 A fuse", size=12, anchor="middle", color=RED)
    svg.line(310, rail5, 420, rail5, RED, 2.5)
    svg.line(215, railg, 420, railg, BLACK, 2.5)
    # C1 across the supply wires
    cx = 360
    svg.dot(cx, rail5, RED, 4)
    svg.dot(cx, railg, BLACK, 4)
    svg.line(cx, rail5, cx, 90, RED)
    svg.line(cx - 12, 90, cx + 12, 90, BLACK, 2.5)
    svg.line(cx - 12, 102, cx + 12, 102, BLACK, 2.5)
    svg.line(cx, 102, cx, railg, BLACK)
    svg.text(cx + 16, 88, "+", size=13, color=RED, weight="bold")
    svg.text(cx, 134, "C1  1000 uF 25 V", size=11, anchor="middle")

    # Arduino + shield box
    top = 190
    svg.rect(420, top - 50, 460, 692, fill="#e8f0fe")
    svg.text(650, top - 26, "Arduino Mega 2560 with Grove Base Shield (switch on 5 V)", size=13, anchor="middle",
             weight="bold")
    svg.text(650, top - 10, "the Arduino is powered ONLY from the PSU (USB cable unplugged while PSU is on)", size=10,
             anchor="middle", color=GREY)
    # supply wires continue to the Mega header pins
    svg.line(420, rail5, 640, rail5, RED, 2.5)
    svg.line(420, railg, 640, railg, BLACK, 2.5)
    svg.circle(640, rail5, 3.5, RED)
    svg.circle(640, railg, 3.5, BLACK)
    svg.text(648, rail5 + 4, "Mega 5V pins (long header)", size=11, color=RED)
    svg.text(648, railg + 4, "Mega GND pins (long header)", size=11)

    for i in range(11):
        y = top + i * 52
        svg.rect(450, y, 230, 40, fill="#fff8e1")
        svg.text(460, y + 16, "Grove connector %s" % CONN[i], size=12, weight="bold")
        svg.text(460, y + 32, "SIG = pin %s" % CONN[i], size=10, color=BLUE)
        svg.line(680, y + 20, 720, y + 20, BLACK, 3)
        svg.rect(720, y, 140, 40, fill="#ffffff")
        svg.text(790, y + 16, "Stick %d: %s" % (i + 1, NAMES[i]), size=11, anchor="middle", weight="bold")
        svg.text(790, y + 32, "ordinary Grove cable", size=10, anchor="middle", color=GREY)

    # button and debug LED
    yb = top + 11 * 52 + 12
    svg.text(460, yb + 10, "D33 -> push button (optional) -> GND   press = flow <-> all on", size=11, color=ORANGE)
    svg.text(460, yb + 28, "Pin 13: onboard debug LED = sign of life (blinks while the program runs)", size=11)

    notes = [
        "Connections",
        "1. Each stick is plugged with its own unmodified Grove cable into the n-th connector: stick 1 D2 ... stick 7 D8,",
        "   stick 8 A0, 9 A1, 10 A2 (the 10 arrow sticks, as in the sign) and stick 11 = the CLOSED light in A3.",
        "2. The shield 3V3/5V switch must be on 5 V. The stick power (red/black wires) goes through the shield and the Grove connectors.",
        "3. PSU V+ -> 10 A fuse F1 -> the Mega 5V pins; PSU V- -> the Mega GND pins (use the 5V and GND pins at the end of the long",
        "   double header, away from the shield; use both 5V pins and both GND pins, thick wire 1.5 mm2). C1 across the supply wires.",
        "4. Power the Arduino ONLY from the PSU: do not plug the USB cable while the PSU is on (upload the sketch first with USB, unplug, then power).",
        "5. No Serial Monitor output without USB; the pin 13 LED shows that the program runs.",
        "6. Let it run and feel the shield, the connectors, the sticks and the wires now and then; also try the all-on mode (button). If anything gets hot, switch off.",
        "7. The mains side of the supply must be in an enclosure and wired by a person who knows how (nothing here is mains).",
    ]
    base = yb + 70
    for k, line in enumerate(notes):
        svg.text(40, base + k * 19, line, size=12, weight="bold" if k == 0 else "normal")
    svg.height = base + len(notes) * 19 + 20
    OUTPUT.write_text(svg.render_a4(landscape=False))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
