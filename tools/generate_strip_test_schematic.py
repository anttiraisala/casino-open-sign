#!/usr/bin/env python3
"""Draw the wiring of the strip test (strip_test/sketch.ino) as an A4 SVG schematic.

    python3 tools/generate_strip_test_schematic.py

Output: strip_test/wiring-diagram.svg (the PDF is made by tools/export_pdf.py).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, ORANGE, RED, GREY, Svg

OUTPUT = Path(__file__).resolve().parent.parent / "strip_test" / "wiring-diagram.svg"


def main():
    svg = Svg(900, 860)
    svg.text(450, 34, "LED strip test - wiring (Arduino Mega 2560 + a short piece of the 12 V strip)", size=17,
             anchor="middle", weight="bold")

    # Arduino
    svg.rect(60, 100, 200, 420, fill="#e8f0fe")
    svg.text(160, 128, "Arduino Mega 2560", size=15, anchor="middle", weight="bold")
    svg.text(160, 146, "powered from USB", size=11, anchor="middle", color=GREY)
    for y, label in ((200, "D6"), (300, "D3"), (400, "GND")):
        svg.text(252, y + 4, label, size=12, anchor="end")

    # D6 -> resistor -> strip DIN
    svg.line(260, 200, 340, 200, BLUE)
    svg.resistor(340, 200, "R1  330 ohm")
    svg.line(400, 200, 560, 200, BLUE)
    svg.circle(560, 200, 3.5, BLUE)

    # Strip piece
    svg.rect(560, 160, 260, 130, fill="#fff8e1")
    svg.text(580, 184, "WS2811 12 V strip piece", size=13, weight="bold")
    svg.text(580, 202, "3 to 6 pieces of 10 cm, cut only", size=10.5, color=GREY)
    svg.text(580, 216, "at the marked cut lines", size=10.5, color=GREY)
    svg.text(556, 196, "DIN", size=10, anchor="end", color=BLUE)
    svg.text(556, 231, "+12 V", size=10, anchor="end", color=RED)
    svg.text(556, 266, "GND", size=10, anchor="end")
    svg.text(580, 252, "DIN end = the end the printed", size=10, color=GREY)
    svg.text(580, 266, "arrows point away from", size=10, color=GREY)

    # 12 V supply
    svg.rect(560, 390, 260, 100, fill="#f3f3f3")
    svg.text(580, 418, "12 V DC supply, 1 A or more", size=13, weight="bold")
    svg.text(580, 436, "(plug-in adapter, NOT the Arduino)", size=10.5, color=GREY)
    svg.text(546, 414, "+", size=14, anchor="end", color=RED, weight="bold")
    svg.text(546, 459, "-", size=14, anchor="end", weight="bold")
    # + 12 V to the strip
    svg.circle(560, 410, 3.5, RED)
    svg.line(560, 410, 520, 410, RED, 2.5)
    svg.line(520, 410, 520, 235, RED, 2.5)
    svg.line(520, 235, 560, 235, RED, 2.5)
    svg.circle(560, 235, 3.5, RED)
    # GND: supply -, strip GND and Arduino GND all together
    svg.circle(560, 455, 3.5, BLACK)
    svg.line(560, 455, 490, 455, BLACK, 2.5)
    svg.line(490, 455, 490, 270, BLACK, 2.5)
    svg.line(490, 270, 560, 270, BLACK, 2.5)
    svg.circle(560, 270, 3.5, BLACK)
    svg.line(260, 400, 490, 400, BLACK, 2.5)
    svg.dot(490, 400, BLACK, 4)

    # Push button D3
    svg.line(260, 300, 330, 300, ORANGE)
    svg.circle(330, 300, 3.5, ORANGE)
    svg.line(334, 288, 416, 288, BLACK, 2.5)
    svg.line(375, 288, 375, 274, BLACK, 2)
    svg.circle(420, 300, 3.5, ORANGE)
    svg.line(420, 300, 440, 300, ORANGE)
    svg.line(440, 300, 440, 330, ORANGE)
    svg.ground(440, 330)
    svg.text(330, 268, "optional push button", size=11, weight="bold")
    svg.text(330, 254, "(next setting)", size=10, color=GREY)

    notes = [
        "Connections",
        "1. Arduino D6 -> 330 ohm resistor -> DIN of the strip piece (the end where the printed arrows point away from).",
        "2. The strip's +12 V comes from the 12 V supply, NEVER from the Arduino 5V pin. Do not connect 12 V to any Arduino pin.",
        "3. GND of the 12 V supply, GND of the strip and GND of the Arduino are connected together (common ground).",
        "4. The Arduino is powered from USB. The push button (D3 to GND) is optional: it moves to the next setting.",
        "5. Optional: a 1000 uF capacitor rated 25 V or more across the strip's +12 V and GND (plus to +12 V, stripe = minus).",
        "6. The built-in LED (pin 13) blinks the setting number 1..12 at the start of each setting; the Serial Monitor (115200)",
        "   prints it too. Tell which setting numbers show red, green, blue in order and how many LEDs light up in each step.",
        "7. If nothing lights: check the strip direction (DIN end), the common GND and that the 12 V supply is on.",
    ]
    for k, line in enumerate(notes):
        svg.text(40, 580 + k * 20, line, size=12, weight="bold" if k == 0 else "normal")
    OUTPUT.write_text(svg.render_a4(landscape=False))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
