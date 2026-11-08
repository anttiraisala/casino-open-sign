#!/usr/bin/env python3
"""Draw the layout of the 11 sticks test (sticks_11_test/): which stick goes to which connector and where to feel for heat.

    python3 tools/generate_sticks_11_test_layout.py

Output: sticks_11_test/layout-diagram.svg (A4 landscape; the PDF is made by tools/export_pdf.py).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREY, RED, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "sticks_11_test" / "layout-diagram.svg"
ROWS = [("1", "L-CAP", "D2"), ("2", "L-WING-UP", "D3"), ("3", "L-WING-DN", "D4"), ("4", "RAIL-UP-L", "D5"),
        ("5", "RAIL-UP-R", "D6"), ("6", "RAIL-DN-L", "D7"), ("7", "RAIL-DN-R", "D8"), ("8", "R-CAP", "A0"),
        ("9", "R-WING-UP", "A1"), ("10", "R-WING-DN", "A2"), ("11", "CLOSED light", "A3")]


def main():
    svg = Svg(1100, 700)
    svg.text(550, 30, "11 sticks - which stick goes where, and what to feel", size=19, anchor="middle", weight="bold")
    svg.text(550, 48, "Sticks 1-10 form the arrow (same order and connectors as in the sign, see ../arrow_sticks/); stick 11 is the CLOSED light.",
             size=12, anchor="middle", color=GREY)
    svg.text(40, 90, "Stick -> Grove connector -> what it shows", size=14, weight="bold")
    for k, (n, name, conn) in enumerate(ROWS):
        y = 105 + k * 40
        closed = n == "11"
        svg.rect(40, y, 90, 30, fill="#fff8e1" if not closed else "#ffffff")
        svg.text(85, y + 20, "Stick " + n, size=13, anchor="middle", weight="bold")
        svg.line(130, y + 15, 190, y + 15, BLUE, 2.5)
        svg.rect(190, y, 80, 30, fill="#e8f0fe")
        svg.text(230, y + 20, conn, size=13, anchor="middle", weight="bold")
        svg.line(270, y + 15, 330, y + 15, BLACK, 2)
        svg.text(340, y + 20, name + (": steady white, level 80" if closed else ": flowing white wave, brightness 0..80"),
                 size=13, weight="bold" if closed else "normal")
        for i in range(10):
            col = "#ffffff" if closed else "#%02x%02x%02x" % tuple([int(60 + 195 * (0.5 + 0.5 * __import__("math").sin((i + k) * 0.7)))] * 3)
            svg.rect(700 + i * 30, y + 3, 24, 24, fill=col, stroke="#546e7a", width=1.2, rx=3)
    svg.text(700, 90, "LEDs of the stick (example moment)", size=12, color=GREY)
    svg.text(40, 570, "Where to feel for heat (switch the supply off first if something is hot):", size=14, weight="bold")
    checks = [
        "1. The Grove Base Shield and the Mega 5V pin area (where the supply wires go in).",
        "2. The Grove connectors and the cables, especially of sticks far from the supply and of the CLOSED light stick.",
        "3. The back of each stick (the chips) and the CLOSED light stick, which is lit all the time.",
        "4. The supply and the fuse holder. Try both modes: flow (default) and all on (button on D33).",
    ]
    for k, line in enumerate(checks):
        svg.text(40, 595 + k * 20, line, size=12)
    OUTPUT.write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
