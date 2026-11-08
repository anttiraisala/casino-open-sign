#!/usr/bin/env python3
"""Generate the files of the "LED strip knight rider" test (strip_knight_rider_test/):

    python3 tools/generate_strip_knight_rider_test.py

Output: diagram.json (Wokwi), wiring-diagram.svg (electrical schematic, A4 portrait) and layout-diagram.svg
(what to look for, A4 landscape). The PDFs are made by tools/export_pdf.py.
"""

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREY, ORANGE, RED, Svg

OUT = Path(__file__).resolve().parent.parent / "strip_knight_rider_test"
PINS = (("D9", 9), ("D10", 10), ("D11", 11))
PIXELS = 50


def diagram_json():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -60, "left": 140,
         "attrs": {"text": "LED strip knight rider: D9 red 1/s, D10 blue 4/s, D11 green 10/s (50 pixels each)"}},
        {"type": "wokwi-text", "id": "t2", "top": -42, "left": 140,
         "attrs": {"text": "Simulation only: the strips are powered from the Mega 5V. In real life they get 12 V from the supply."}},
    ]
    conns = []
    for k, (label, pin) in enumerate(PINS):
        parts.append({"type": "wokwi-resistor", "id": "r%d" % (k + 1), "top": 232 + 30 * k, "left": 120, "attrs": {"value": "330"}})
        parts.append({"type": "wokwi-led-strip", "id": "strip%d" % (k + 1), "top": 330 + 70 * k, "left": 60, "rotate": 0,
                      "attrs": {"pixels": str(PIXELS), "pixelSize": "3535"}})
        conns.append(["mega:%d" % pin, "r%d:1" % (k + 1), "green", []])
        conns.append(["r%d:2" % (k + 1), "strip%d:DIN" % (k + 1), "green", []])
        conns.append(["mega:5V", "strip%d:VDD" % (k + 1), "red", []])
        conns.append(["mega:GND.1", "strip%d:VSS" % (k + 1), "black", []])
    return {"version": 1, "author": "Casino sign project - LED strip knight rider test", "editor": "wokwi",
            "parts": parts, "connections": conns, "dependencies": {}}


def wiring():
    svg = Svg(960, 860)
    svg.text(480, 34, "LED strip knight rider - wiring (Arduino Mega 2560 + three 12 V WS2811 strips)", size=17,
             anchor="middle", weight="bold")
    # Arduino
    svg.rect(60, 100, 200, 460, fill="#e8f0fe")
    svg.text(160, 128, "Arduino Mega 2560", size=15, anchor="middle", weight="bold")
    svg.text(160, 146, "powered from USB", size=11, anchor="middle", color=GREY)
    ys = (164, 254, 344)
    for (label, _), y in zip(PINS, ys):
        svg.text(252, y + 4, label, size=12, anchor="end")
    svg.text(252, 444, "GND", size=12, anchor="end")
    for k, y in enumerate(ys):
        svg.line(260, y, 340, y, BLUE)
        svg.resistor(340, y, "R%d  330 ohm" % (k + 1))
        svg.line(400, y, 560, y, BLUE)
        svg.circle(560, y, 3.5, BLUE)
        top = y - 14
        svg.rect(560, top, 260, 64, fill="#fff8e1")
        svg.text(580, top + 18, "Strip %d: %s light" % (k + 1, ("red", "blue", "green")[k]), size=13, weight="bold")
        svg.text(580, top + 36, "WS2811 12 V, 5 m, DIN end", size=10.5, color=GREY)
        svg.text(556, y - 6, "DIN", size=10, anchor="end", color=BLUE)
        # +12 V and GND terminals on the right side
        svg.circle(820, y + 18, 3.5, RED)
        svg.text(814, y + 14, "+12 V", size=10, anchor="end", color=RED)
        svg.circle(820, y + 36, 3.5, BLACK)
        svg.text(814, y + 32, "GND", size=10, anchor="end")
        svg.line(820, y + 18, 860, y + 18, RED, 2.5)
        svg.line(820, y + 36, 890, y + 36, BLACK, 2.5)
        svg.dot(860, y + 18, RED, 4)
        svg.dot(890, y + 36, BLACK, 4)
    svg.line(860, ys[0] + 18, 860, 500, RED, 2.5)
    svg.line(890, ys[0] + 36, 890, 610, BLACK, 2.5)
    # 12 V supply
    svg.rect(580, 470, 240, 100, fill="#f3f3f3")
    svg.text(600, 498, "12 V DC supply", size=13, weight="bold")
    svg.text(600, 516, "(plug-in adapter, NOT the Arduino)", size=10.5, color=GREY)
    svg.text(600, 534, "5 m of strip: up to about 35 W", size=10.5, color=GREY)
    svg.circle(820, 500, 3.5, RED)
    svg.text(814, 496, "+", size=14, anchor="end", color=RED, weight="bold")
    svg.line(820, 500, 860, 500, RED, 2.5)
    svg.circle(820, 545, 3.5, BLACK)
    svg.text(814, 541, "-", size=14, anchor="end", weight="bold")
    svg.line(820, 545, 890, 545, BLACK, 2.5)
    svg.dot(890, 545, BLACK, 4)
    # common ground to the Arduino
    svg.line(890, 610, 300, 610, BLACK, 2.5)
    svg.line(300, 610, 300, 444, BLACK, 2.5)
    svg.line(300, 444, 260, 444, BLACK, 2.5)
    svg.dot(300, 610, BLACK, 4)
    notes = [
        "Connections",
        "1. Arduino D9, D10, D11 -> a 330 ohm resistor each -> DIN of strip 1, 2, 3 (the end where the printed arrows point away from).",
        "   One strip is enough: connect it to D9 (red, slow), D10 (blue) or D11 (green, fast).",
        "2. The +12 V of every strip comes from the 12 V supply, NEVER from an Arduino pin. No 12 V on any Arduino pin.",
        "3. GND of the 12 V supply, GND of all strips and GND of the Arduino are connected together (common ground).",
        "4. The Arduino is powered from USB (the Serial Monitor, 115200 baud, shows one DBG line per second).",
        "5. Optional: a 1000 uF capacitor rated 25 V or more across +12 V and GND at the strip start (plus to +12 V).",
        "6. The built-in LED (pin 13) blinks while the program runs.",
        "7. Brightness is limited to 80 of 255 in the sketch (TEST_LEVEL); one lit pixel per strip needs very little current.",
    ]
    for k, line in enumerate(notes):
        svg.text(40, 650 + k * 20, line, size=12, weight="bold" if k == 0 else "normal")
    (OUT / "wiring-diagram.svg").write_text(svg.render_a4(landscape=False))


X0, SEG, N = 60, 130, 6
LED_W, LED_H = 30, 26
OFF, RED_C, GREEN_C, BLUE_C, WHITE_C = "#cfd8dc", "#e53935", "#43a047", "#1e88e5", "#ffffff"


def strip_row(svg, y, colours):
    """colours: list of N pixels, each a colour or None; each pixel has 3 LEDs of that colour."""
    svg.rect(X0 - 10, y - 6, N * SEG + 20, 62, fill="#fff8e1", stroke="#8d6e63", width=1.6, rx=4)
    for p in range(N):
        for k in range(3):
            svg.rect(X0 + p * SEG + 12 + k * 36, y + 10, LED_W, LED_H, fill=colours[p] or OFF, stroke="#546e7a", width=1.4, rx=3)


def layout():
    svg = Svg(1100, 700)
    svg.text(550, 32, "LED strip knight rider - what you should see", size=19, anchor="middle", weight="bold")
    svg.text(550, 50, "Three strips of 50 pixels (each pixel = one control chip = 3 LEDs = 10 cm). One light runs back and forth.",
             size=12, anchor="middle", color=GREY)
    rows = [("D9", "RED", RED_C, "1 pixel per second", "one cycle (end to end and back) = 98 s", 2),
            ("D10", "BLUE", BLUE_C, "4 pixels per second", "one cycle = 24.5 s", 3),
            ("D11", "GREEN", GREEN_C, "10 pixels per second", "one cycle = 9.8 s", 4)]
    y = 130
    for pin, name, col, speed, cycle, at in rows:
        svg.text(24, y - 34, "%s: %s light, %s (%s)" % (pin, name, speed, cycle), size=14, weight="bold")
        strip_row(svg, y, [col if p == at else None for p in range(N)])
        ax0, ax1 = X0 + 20, X0 + N * SEG - 20
        svg.line(ax0, y + 76, ax1, y + 76, GREY, 2)
        svg.path("M%g,%g l12,-6 v12 z" % (ax0, y + 76), GREY, 1, GREY)
        svg.path("M%g,%g l-12,-6 v12 z" % (ax1, y + 76), GREY, 1, GREY)
        svg.text((ax0 + ax1) / 2, y + 96, "pixel 1  <->  pixel 50 (only 6 of the 50 pixels are drawn)", size=11, anchor="middle", color=GREY)
        y += 190
    svg.text(24, y - 10, "At each end the light turns around at once (the end pixel is lit only once per turn). The LED on pin 13 is shortly inverted at a turnaround.", size=11)
    svg.text(24, y + 8, "If your strip has a different number of pixels, change STRIP_PIXELS in the sketch. Wrong colours: change COLOR_ORDER (see README).", size=11)
    (OUT / "layout-diagram.svg").write_text(svg.render_a4(landscape=True))


def main():
    OUT.mkdir(exist_ok=True)
    (OUT / "diagram.json").write_text(json.dumps(diagram_json(), indent=2) + "\n")
    wiring()
    layout()
    print("Wrote %s" % OUT)


if __name__ == "__main__":
    main()
