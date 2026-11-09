#!/usr/bin/env python3
"""Generate the files of the "Strip frame demo test" (strip_frame_demo_test/):

    python3 tools/generate_strip_frame_demo_test.py

Output: diagram.json (Wokwi), wiring-diagram.svg (electrical schematic, A4 portrait) and layout-diagram.svg
(frame route and what to look for, A4 landscape). The PDFs are made by tools/export_pdf.py.
"""

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREY, ORANGE, RED, Svg

OUT = Path(__file__).resolve().parent.parent / "strip_frame_demo_test"
FRAME_PIXELS = 16
SINGLE_PIXELS = 1
# (name, colour) of the D10 pulse palette, same order as PALETTE in sketch.ino
PALETTE = [("red", "#e53935"), ("orange", "#fb8c00"), ("yellow", "#fdd835"), ("green", "#43a047"),
           ("cyan", "#00acc1"), ("blue", "#1e88e5"), ("violet", "#8e24aa"), ("white", "#ffffff")]


def diagram_json():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -60, "left": 140,
         "attrs": {"text": "Strip frame demo: D9 frame strip 16 pixels (red, green, blue lights), D10 single pixel pulse"}},
        {"type": "wokwi-text", "id": "t2", "top": -42, "left": 140,
         "attrs": {"text": "Simulation only: the strips are powered from the Mega 5V. In real life they get 12 V from the supply."}},
        {"type": "wokwi-resistor", "id": "r1", "top": 232, "left": 120, "attrs": {"value": "330"}},
        {"type": "wokwi-resistor", "id": "r2", "top": 262, "left": 120, "attrs": {"value": "330"}},
        {"type": "wokwi-led-strip", "id": "strip1", "top": 330, "left": 60, "rotate": 0,
         "attrs": {"pixels": str(FRAME_PIXELS), "pixelSize": "3535"}},
        {"type": "wokwi-led-strip", "id": "strip2", "top": 400, "left": 60, "rotate": 0,
         "attrs": {"pixels": str(SINGLE_PIXELS), "pixelSize": "3535"}},
    ]
    conns = []
    for k, pin in enumerate((9, 10)):
        conns.append(["mega:%d" % pin, "r%d:1" % (k + 1), "green", []])
        conns.append(["r%d:2" % (k + 1), "strip%d:DIN" % (k + 1), "green", []])
        conns.append(["mega:5V", "strip%d:VDD" % (k + 1), "red", []])
        conns.append(["mega:GND.1", "strip%d:VSS" % (k + 1), "black", []])
    return {"version": 1, "author": "Casino sign project - Strip frame demo test", "editor": "wokwi",
            "parts": parts, "connections": conns, "dependencies": {}}


def wiring():
    svg = Svg(960, 800)
    svg.text(480, 34, "Strip frame demo test - wiring (Arduino Mega 2560 + two 12 V WS2811 strip pieces)", size=17,
             anchor="middle", weight="bold")
    svg.rect(60, 100, 200, 360, fill="#e8f0fe")
    svg.text(160, 128, "Arduino Mega 2560", size=15, anchor="middle", weight="bold")
    svg.text(160, 146, "powered from USB", size=11, anchor="middle", color=GREY)
    ys = (190, 300)
    for label, y in zip(("D9", "D10"), ys):
        svg.text(252, y + 4, label, size=12, anchor="end")
    svg.text(252, 404, "GND", size=12, anchor="end")
    blocks = (("Frame strip: 16 pixels", "1.6 m, pixel 1 = lower left"),
              ("Single pixel: 1 pixel", "one chip = 3 LEDs = 10 cm"))
    for k, y in enumerate(ys):
        svg.line(260, y, 340, y, BLUE)
        svg.resistor(340, y, "R%d  330 ohm" % (k + 1))
        svg.line(400, y, 560, y, BLUE)
        svg.circle(560, y, 3.5, BLUE)
        top = y - 14
        svg.rect(560, top, 260, 64, fill="#fff8e1")
        svg.text(580, top + 18, blocks[k][0], size=13, weight="bold")
        svg.text(580, top + 34, "WS2811 12 V, DIN end", size=10.5, color=GREY)
        svg.text(580, top + 48, blocks[k][1], size=9.5, color=GREY)
        svg.text(556, y - 6, "DIN", size=10, anchor="end", color=BLUE)
        svg.circle(820, y + 18, 3.5, RED)
        svg.text(814, y + 14, "+12 V", size=10, anchor="end", color=RED)
        svg.circle(820, y + 36, 3.5, BLACK)
        svg.text(814, y + 32, "GND", size=10, anchor="end")
        svg.line(820, y + 18, 860, y + 18, RED, 2.5)
        svg.line(820, y + 36, 890, y + 36, BLACK, 2.5)
        svg.dot(860, y + 18, RED, 4)
        svg.dot(890, y + 36, BLACK, 4)
    svg.line(860, ys[0] + 18, 860, 450, RED, 2.5)
    svg.line(890, ys[0] + 36, 890, 560, BLACK, 2.5)
    # 12 V supply
    svg.rect(580, 420, 240, 100, fill="#f3f3f3")
    svg.text(600, 448, "12 V DC supply", size=13, weight="bold")
    svg.text(600, 466, "(plug-in adapter, NOT the Arduino)", size=10.5, color=GREY)
    svg.text(600, 484, "1.7 m of strip: 12 V 1 A is plenty", size=10.5, color=GREY)
    svg.circle(820, 450, 3.5, RED)
    svg.text(814, 446, "+", size=14, anchor="end", color=RED, weight="bold")
    svg.line(820, 450, 860, 450, RED, 2.5)
    svg.circle(820, 495, 3.5, BLACK)
    svg.text(814, 491, "-", size=14, anchor="end", weight="bold")
    svg.line(820, 495, 890, 495, BLACK, 2.5)
    svg.dot(890, 495, BLACK, 4)
    # common ground to the Arduino
    svg.line(890, 560, 300, 560, BLACK, 2.5)
    svg.line(300, 560, 300, 404, BLACK, 2.5)
    svg.line(300, 404, 260, 404, BLACK, 2.5)
    svg.dot(300, 560, BLACK, 4)
    notes = [
        "Connections",
        "1. Arduino D9 -> 330 ohm resistor R1 -> DIN of the frame strip (16 pixels, the end where the printed arrows point away from).",
        "2. Arduino D10 -> 330 ohm resistor R2 -> DIN of the single-pixel piece (one chip, 3 LEDs, cut from the same roll).",
        "3. The +12 V of both strips comes from the 12 V supply, NEVER from an Arduino pin. No 12 V on any Arduino pin.",
        "4. GND of the 12 V supply, GND of both strips and GND of the Arduino are connected together (common ground).",
        "5. The Arduino is powered from USB (the Serial Monitor, 115200 baud, shows one DBG line per second).",
        "6. Optional: a 1000 uF capacitor rated 25 V or more across +12 V and GND at the strip start (plus to +12 V).",
        "7. The built-in LED (pin 13) blinks while the program runs.",
        "8. Brightness peaks at 80 of 255 in the sketch (TEST_LEVEL); at most three lit pixels need very little current.",
    ]
    for k, line in enumerate(notes):
        svg.text(40, 610 + k * 20, line, size=12, weight="bold" if k == 0 else "normal")
    (OUT / "wiring-diagram.svg").write_text(svg.render_a4(landscape=False))


OFF = "#cfd8dc"
RED_C, GREEN_C, BLUE_C = "#e53935", "#43a047", "#1e88e5"


def perimeter_point(d, x0, y0, w, h):
    """Point at distance d from the lower-left corner, counter-clockwise (bottom edge to the right first)."""
    d %= 2 * (w + h)
    if d < w:
        return x0 + d, y0 + h
    d -= w
    if d < h:
        return x0 + w, y0 + h - d
    d -= h
    if d < w:
        return x0 + w - d, y0
    d -= w
    return x0, y0 + d


def mix(lights):
    """Colour of a pixel from a set of light colours (additive)."""
    r = 1 if "r" in lights else 0
    g = 1 if "g" in lights else 0
    b = 1 if "b" in lights else 0
    table = {(1, 0, 0): RED_C, (0, 1, 0): GREEN_C, (0, 0, 1): BLUE_C, (1, 1, 0): "#fdd835", (0, 1, 1): "#00bcd4",
             (1, 0, 1): "#d81b60", (1, 1, 1): "#ffffff"}
    return table.get((r, g, b), OFF)


def layout():
    svg = Svg(1100, 760)
    svg.text(550, 32, "Strip frame demo test - frame route and what you should see", size=19, anchor="middle", weight="bold")
    svg.text(550, 50, "D9: 16 pixels (1 pixel = 1 chip = 3 LEDs = 10 cm) around the frame. D10: one pixel (3 LEDs) pulsing.",
             size=12, anchor="middle", color=GREY)
    # --- frame with 16 pixels, counter-clockwise from the lower-left corner
    x0, y0, w, h = 150, 110, 560, 240
    svg.rect(x0 - 24, y0 - 24, w + 48, h + 48, fill="#ffffff", stroke="#8d6e63", width=3, rx=6)
    svg.rect(x0 + 6, y0 + 6, w - 12, h - 12, fill="#fafafa", stroke="#bdbdbd", width=1.2, rx=3)
    svg.text(x0 + w / 2, y0 + h / 2 - 6, "SIGN (front view)", size=16, anchor="middle", color=GREY, weight="bold")
    svg.text(x0 + w / 2, y0 + h / 2 + 14, "strip on the frame, running counter-clockwise", size=12, anchor="middle", color=GREY)
    # snapshot: red on 3, green on 3 (yellow), blue on 10; one more mixed pixel to explain the colours
    snap = {0: set(), 2: {"r", "g"}, 6: {"b"}, 7: {"g", "b"}, 11: {"r"}, 12: {"r", "g", "b"}}
    step = 2 * (w + h) / FRAME_PIXELS
    for i in range(FRAME_PIXELS):
        px, py = perimeter_point(step * i + step / 2, x0 - 12, y0 - 12, w + 24, h + 24)
        svg.circle(px, py, 14, "#546e7a")
        svg.rect(px - 11, py - 11, 22, 22, fill=mix(snap.get(i, set())), stroke="#546e7a", width=1.4, rx=11)
        tx, ty = perimeter_point(step * i + step / 2, x0 - 12, y0 - 12, w + 24, h + 24)
        # number outside the frame
        ox = -30 if tx < x0 else (30 if tx > x0 + w else 0)
        oy = -26 if ty < y0 else (30 if ty > y0 + h else 0)
        svg.text(tx + ox, ty + oy + 4, str(i + 1), size=13, anchor="middle", weight="bold")
    # direction arrow along the bottom edge
    svg.line(x0 + 30, y0 + h + 62, x0 + 190, y0 + h + 62, GREY, 2)
    svg.path("M%g,%g l-12,-6 v12 z" % (x0 + 190 + 12, y0 + h + 62), GREY, 1, GREY)
    svg.text(x0 + 30, y0 + h + 82, "pixel 1 at the lower-left corner, then to the right (counter-clockwise), pixel 16 on the left side",
             size=11, color=GREY)
    # legend
    lx, ly = 760, 100
    svg.text(lx, ly, "D9: three Knight Rider lights", size=14, weight="bold")
    rows = [(RED_C, "RED   2 pixels/s (500 ms per step)"), (GREEN_C, "GREEN 1.3 x faster (385 ms)"),
            (BLUE_C, "BLUE  1.5 x faster (333 ms)")]
    for k, (c, t) in enumerate(rows):
        svg.rect(lx, ly + 16 + k * 24, 20, 16, fill=c, stroke="#546e7a", width=1.2, rx=3)
        svg.text(lx + 30, ly + 29 + k * 24, t, size=12)
    svg.text(lx, ly + 108, "Each bounces 1 -> 16 -> 1 (30 steps per cycle).", size=11, color=GREY)
    svg.text(lx, ly + 124, "Colours add where lights overlap:", size=11, color=GREY)
    mixes = [("#fdd835", "red + green"), ("#00bcd4", "green + blue"), ("#d81b60", "red + blue"), ("#ffffff", "all three")]
    for k, (c, t) in enumerate(mixes):
        svg.rect(lx, ly + 136 + k * 22, 20, 14, fill=c, stroke="#546e7a", width=1.2, rx=3)
        svg.text(lx + 30, ly + 148 + k * 22, "= " + t, size=11.5)
    svg.text(lx, ly + 242, "Cycle lengths: red 15 s, green 11.55 s, blue 9.99 s.", size=11, color=GREY)
    svg.text(lx, ly + 258, "Pixels turn around at both ends; LED 13 blinks.", size=11, color=GREY)
    # --- D10 pulse
    py0 = 470
    svg.text(24, py0, "D10: one pixel (3 LEDs), a pulse dark -> bright -> dark every 3 s, a new colour at every pulse", size=14, weight="bold")
    bx = 40
    for k, (name, c) in enumerate(PALETTE):
        cx = bx + k * 128
        svg.rect(cx, py0 + 18, 112, 40, fill="#fff8e1", stroke="#8d6e63", width=1.4, rx=4)
        for j, level in enumerate(("#eceff1", c, "#eceff1")):
            svg.rect(cx + 8 + j * 34, py0 + 26, 28, 24, fill=level, stroke="#546e7a", width=1.2, rx=3)
        svg.text(cx + 56, py0 + 76, "%d: %s" % (k + 1, name), size=12, anchor="middle")
    svg.text(24, py0 + 106, "Each box: dark (start) - bright (peak, 80 of 255 on the strongest channel, after 1.5 s) - dark again. Order: red, orange, yellow, green, cyan, blue, violet, white, then again.", size=11, color=GREY)
    svg.text(24, py0 + 124, "The LED on pin 13 is shortly inverted at every turnaround of a frame light and at every new pulse colour.", size=11, color=GREY)
    svg.text(24, py0 + 142, "Wrong colours: change COLOR_ORDER in the sketch (see README).", size=11, color=GREY)
    (OUT / "layout-diagram.svg").write_text(svg.render_a4(landscape=True))


def main():
    OUT.mkdir(exist_ok=True)
    (OUT / "diagram.json").write_text(json.dumps(diagram_json(), indent=2) + "\n")
    wiring()
    layout()
    print("Wrote %s" % OUT)


if __name__ == "__main__":
    main()
