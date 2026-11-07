#!/usr/bin/env python3
"""Draw the layout of the strip test: the strip piece, the cut marks and what to look for.

    python3 tools/generate_strip_test_layout.py

Output: strip_test/layout-diagram.svg (A4 landscape; the PDF is made by tools/export_pdf.py).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREY, RED, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "strip_test" / "layout-diagram.svg"
X0, SEG, N = 150, 150, 6            # left edge, width of one 10 cm pixel segment, number of segments drawn
LED_W, LED_H = 34, 26
OFF, RED_C, GREEN_C, BLUE_C, WHITE_C = "#cfd8dc", "#e53935", "#43a047", "#1e88e5", "#ffffff"


def strip(svg, y, colours, lit_leds=None, label=None):
    """One strip row. colours: colour of each of the N pixels (3 LEDs each) or None = dark.
    lit_leds: if given, a set of (pixel, led) pairs that are lit white (overrides colours)."""
    svg.rect(X0 - 10, y - 6, N * SEG + 20, 62, fill="#fff8e1", stroke="#8d6e63", width=1.6, rx=4)
    for p in range(N):
        for k in range(3):
            x = X0 + p * SEG + 22 + k * 40
            if lit_leds is not None:
                c = WHITE_C if (p, k) in lit_leds else OFF
            else:
                c = colours[p] or OFF
            svg.rect(x, y + 10, LED_W, LED_H, fill=c, stroke="#546e7a", width=1.4, rx=3)
    if label:
        svg.text(X0 - 20, y + 30, label, size=12, anchor="end", weight="bold")


def cuts(svg, y):
    for p in range(1, N):
        x = X0 + p * SEG
        svg.line(x, y - 14, x, y + 62, RED, 1.6, dash="5 4")
    svg.text(X0 + SEG, y - 18, "cut only at these marks (every 10 cm)", size=11, color=RED)


def main():
    svg = Svg(1100, 778)
    svg.text(550, 32, "LED strip test - layout of the piece and what to look for", size=19, anchor="middle", weight="bold")
    svg.text(550, 50, "A piece of 3 to 6 pixel segments (10 cm each) is enough. Each segment has 3 LEDs.", size=12,
             anchor="middle", color=GREY)

    # --- A: the piece
    y = 112
    svg.text(24, 82, "A  The strip piece (start end = DIN end on the left)", size=14, weight="bold")
    strip(svg, y, [None] * N)
    cuts(svg, y)
    for p in range(N):
        svg.text(X0 + p * SEG + SEG / 2, y + 80, "pixel %d" % (p + 1), size=12, anchor="middle")
    svg.line(X0, y + 98, X0 + N * SEG, y + 98, BLACK, 1.4)
    svg.line(X0 + N * SEG - 10, y + 93, X0 + N * SEG, y + 98, BLACK, 1.4)
    svg.line(X0 + N * SEG - 10, y + 103, X0 + N * SEG, y + 98, BLACK, 1.4)
    svg.text(X0 + N * SEG / 2, y + 116, "data direction (the arrows printed on the strip point this way)", size=11,
             anchor="middle", color=GREY)
    # pads on the left
    for k, (lab, col) in enumerate((("+12 V", RED), ("DIN", BLUE), ("GND", BLACK))):
        yy = y + 8 + k * 16
        svg.circle(X0 - 22, yy + 6, 3.5, col)
        svg.line(X0 - 22, yy + 6, X0 - 70, yy + 6, col, 1.4)
        svg.text(X0 - 76, yy + 10, lab, size=11, anchor="end", color=col)
    svg.text(24, y + 134, "DIN <- Arduino D6 via 330 ohm.  +12 V <- 12 V supply.  GND <- supply minus and Arduino GND.", size=11)

    # --- B: colour test
    y = 298
    svg.text(24, y - 30, "B  Colour test (the first 3 seconds of every setting)", size=14, weight="bold")
    strip(svg, y, [RED_C, GREEN_C, BLUE_C, None, None, None])
    svg.text(X0 + 0 * SEG + SEG / 2, y + 80, "red", size=12, anchor="middle", weight="bold")
    svg.text(X0 + 1 * SEG + SEG / 2, y + 80, "green", size=12, anchor="middle", weight="bold")
    svg.text(X0 + 2 * SEG + SEG / 2, y + 80, "blue", size=12, anchor="middle", weight="bold")
    svg.text(X0 + 3.5 * SEG, y + 80, "dark", size=12, anchor="middle", color=GREY)
    svg.text(24, y + 108, "The setting number (blinks of the built-in LED, also in the Serial Monitor) where you see red, green, blue "
             "in this order is the colour order to use.", size=11)
    svg.text(24, y + 124, "Wrong settings show other colours (for example green, red, blue), nothing, white, or flicker.", size=11)

    # --- C: walking pixel
    y = 470
    svg.text(24, y - 30, "C  Walking white pixel (after the colour test): how many LEDs light up in one step?", size=14, weight="bold")
    three = {(1, 0), (1, 1), (1, 2)}
    one = {(1, 0)}
    strip(svg, y, [None] * N, lit_leds=three, label="3 LEDs  ")
    svg.text(X0, y + 76, "one chip drives 3 LEDs (probable): the whole 10 cm segment lights up", size=11, color=GREY)
    strip(svg, y + 86, [None] * N, lit_leds=one, label="1 LED  ")
    svg.text(X0, y + 162, "every LED has its own chip: only one LED lights up", size=11, color=GREY)
    svg.text(24, y + 196, "The white spot moves one step every half second along the piece. Tell me which of the two pictures matches "
             "what you see (3 LEDs or 1 LED at a time).", size=11)
    svg.text(24, y + 212, "Also tell: do all pixels work, does anything flicker, and what is printed on the strip or the reel.", size=11)

    OUTPUT.write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
