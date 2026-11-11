#!/usr/bin/env python3
"""Generate the drawings and the Wokwi circuit of the hex rotary switch test (hex_rotary_test/).

    python3 tools/generate_hex_rotary_test.py

Output in hex_rotary_test/: diagram.json (Wokwi), wiring-diagram.svg (electrical schematic, A4 portrait) and
layout-diagram.svg (the pins on the Mega and the switch legs, A4 landscape). The PDFs are made by tools/export_pdf.py.
The pins are read from hex_rotary_test/sketch.ino, so the drawings always follow the sketch.
"""

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREEN, GREY, ORANGE, RED, Svg

OUT = Path(__file__).resolve().parent.parent / "hex_rotary_test"
SKETCH = OUT / "sketch.ino"


def read_pins():
    """The four switch pins (legs 1, 2, 4, 8) from the sketch."""
    text = SKETCH.read_text()
    m = re.search(r"HEX_PINS\[4\]\s*=\s*\{([^}]*)\}", text)
    pins = [int(v) for v in m.group(1).split(",")]
    assert len(pins) == 4
    return pins


def wokwi(pins):
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -120, "left": 20,
         "attrs": {"text": "Hex rotary switch test: A (2..51) follows the switch, shown on the LCD"}},
        {"type": "wokwi-text", "id": "t2", "top": -102, "left": 20,
         "attrs": {"text": "Wokwi has no hex rotary switch: the DIP bank 1-4 stands in (ON = leg connected to C = bit set)"}},
        {"type": "wokwi-text", "id": "t3", "top": -84, "left": 20,
         "attrs": {"text": "Switch 1 = leg 1 (D%d), 2 = leg 2 (D%d), 3 = leg 4 (D%d), 4 = leg 8 (D%d)" % tuple(pins)}},
        {"type": "wokwi-dip-switch-8", "id": "hex", "top": -50, "left": 20, "attrs": {}},
        {"type": "wokwi-lcd1602", "id": "lcd", "top": 480, "left": 20, "attrs": {"pins": "i2c"}},
    ]
    connections = []
    for k, pin in enumerate(pins):
        n = k + 1
        connections.append(["mega:%d" % pin, "hex:%db" % n, "orange", []])
        connections.append(["mega:GND.1", "hex:%da" % n, "black", []])
    connections += [
        ["mega:5V", "lcd:VCC", "red", []],
        ["mega:GND.1", "lcd:GND", "black", []],
        ["mega:20", "lcd:SDA", "green", []],
        ["mega:21", "lcd:SCL", "green", []],
    ]
    doc = {"version": 1, "author": "Casino sign project - hex rotary switch test", "editor": "wokwi",
           "parts": parts, "connections": connections, "dependencies": {}}
    (OUT / "diagram.json").write_text(json.dumps(doc, indent=2) + "\n")
    print("Wrote %s" % (OUT / "diagram.json"))


def schematic(pins):
    width, height = 940, 1010
    svg = Svg(width, height)
    svg.text(width / 2, 22, "Hex rotary switch test - wiring", size=17, anchor="middle", weight="bold")

    ard_x0, ard_x1 = 540, 900
    ard_y0, ard_y1 = 70, 640
    svg.rect(ard_x0, ard_y0, ard_x1 - ard_x0, ard_y1 - ard_y0, fill="#e8f0fe")
    svg.text((ard_x0 + ard_x1) / 2, ard_y0 + 24, "Arduino Mega 2560", size=14, anchor="middle", weight="bold")
    svg.text((ard_x0 + ard_x1) / 2, ard_y0 + 42, "powered over USB for this test", size=11, anchor="middle", color=GREY)

    # --- hex rotary switch
    y0 = 120
    svg.text(40, y0 - 26, "SW  hex rotary switch 0-F", size=13, weight="bold")
    svg.text(40, y0 - 10, "6 legs: 1, 2, 4, 8 and two C (common, joined inside)", size=10, color=GREY)
    svg.rect(60, y0, 150, 150, fill="#f3f3f3")
    svg.circle(110, y0 + 62, 26, BLACK)
    svg.line(110, y0 + 62, 110, y0 + 40, BLACK, 2.5)
    svg.dot(110, y0 + 62, BLACK, 3)
    svg.text(110, y0 + 112, "0 .. F", size=12, anchor="middle")
    for k, (weight, pin) in enumerate(zip((1, 2, 4, 8), pins)):
        y = y0 + 25 + k * 28
        svg.text(198, y + 5, "leg %d" % weight, size=14, anchor="end", weight="bold")
        svg.circle(210, y, 3.5, ORANGE)
        svg.line(210, y, ard_x0, y, ORANGE)
        svg.text(ard_x0 + 8, y + 4, "D%d" % pin, size=13)
        svg.text(300, y - 5, "bit %d (weight %d)" % (k, weight), size=10, color=GREY)
    yc = y0 + 150
    svg.text(160, yc - 8, "C (common)", size=12, anchor="middle", weight="bold")
    svg.circle(160, yc, 3.5, ORANGE)
    svg.line(160, yc, 160, yc + 14, ORANGE)
    svg.ground(160, yc + 14)
    svg.text(160, yc + 46, "one C leg to GND (the other C leg is left free)", size=10, anchor="middle", color=GREY)
    svg.text(40, yc + 66, "No resistors: the Mega's internal pull-ups (INPUT_PULLUP) are used.", size=11)
    svg.text(40, yc + 82, "A leg connected to C reads LOW = bit set (sketch: HEX_BIT_IS_LOW).", size=11)

    # --- LCD
    ly0 = 440
    svg.text(40, ly0 - 40, "LCD  2x16 character LCD, I2C backpack", size=13, weight="bold")
    svg.text(40, ly0 - 24, "shows A, the switch position and the direction", size=10, color=GREY)
    svg.text(40, ly0 - 10, "(I2C address 0x27; the boot scan on the Serial Monitor lists the real one)", size=10, color=GREY)
    svg.rect(60, ly0, 150, 100, fill="#f3f3f3")
    svg.text(110, ly0 + 54, "LCD", size=13, anchor="middle", weight="bold")
    svg.text(110, ly0 + 72, "16 x 2", size=11, anchor="middle")
    for k, (name, target, color) in enumerate((("GND", None, BLACK), ("VCC", "5V pin", RED),
                                               ("SDA", "D20 (SDA)", GREEN), ("SCL", "D21 (SCL)", GREEN))):
        y = ly0 + 14 + k * 24
        svg.text(202, y + 4, name, size=11, anchor="end")
        svg.circle(210, y, 3.5, color)
        if target is None:
            svg.line(210, y, 226, y, BLACK)
            svg.ground(226, y)
        else:
            svg.line(210, y, ard_x0, y, color)
            svg.text(ard_x0 + 8, y + 4, target, size=13)

    # --- LED 13
    svg.text(ard_x0 + 8, ard_y1 - 18, "Pin 13: onboard LED = sign of life (blinks while the program runs)", size=11)

    # --- notes
    notes = [
        "Connections",
        "1. Switch leg 1 -> D%d, leg 2 -> D%d, leg 4 -> D%d, leg 8 -> D%d; one C leg -> GND (any Mega GND pin)." % tuple(pins),
        "2. LCD: GND -> GND, VCC -> 5V, SDA -> D20, SCL -> D21 (the Mega's own I2C pins; no pull-up resistors needed).",
        "3. Power: USB cable to the PC (upload, Serial Monitor at 115200 baud). No LED strips or sticks are connected.",
        "4. If the legs are not marked, look for the numbers 1, 2, 4, 8 and the two letters C on the switch body.",
        "5. If A goes the wrong way: set reverseDirection to true in sketch.ino. If position 0 shows as F: set HEX_BIT_IS_LOW",
        "   to false (the switch is complement-coded).",
        "6. If the LCD shows no text but its light is on: turn the blue contrast trimmer on the I2C backpack, and check that",
        "   the LCD VCC pin measures at least 4.75 V (see README.txt).",
    ]
    base = 700
    svg.line(40, 690, 80, 690, ORANGE, 2)
    svg.text(88, 694, "switch leg", size=11)
    svg.line(190, 690, 230, 690, GREEN, 2)
    svg.text(238, 694, "I2C (SDA / SCL)", size=11)
    svg.line(380, 690, 420, 690, RED, 2.5)
    svg.text(428, 694, "+5 V", size=11)
    svg.line(490, 690, 530, 690, BLACK, 2.5)
    svg.text(538, 694, "GND", size=11)
    for k, line in enumerate(notes):
        svg.text(40, base + 20 + k * 19, line, size=12, weight="bold" if k == 0 else "normal")
    svg.height = base + 20 + len(notes) * 19 + 20
    (OUT / "wiring-diagram.svg").write_text(svg.render_a4(landscape=False))
    print("Wrote %s" % (OUT / "wiring-diagram.svg"))


def layout(pins):
    """The end of the Mega's long double header with the four switch pins and GND, next to the switch legs table."""
    width, height = 1100, 640
    svg = Svg(width, height)
    svg.text(width / 2, 28, "Hex rotary switch test - where the wires go (not to scale)", size=18, anchor="middle", weight="bold")

    # Left: the end of the long double-row header of the Mega (top view, board edge on the right).
    svg.text(60, 70, "Mega 2560: long double-row header (from above)", size=13, weight="bold")
    svg.text(60, 88, "One column holds the even pins, the other the odd pins (read the numbers on the board).", size=11, color=GREY)
    rows = [("5V", "5V"), ("22", "23"), ("...", "..."), ("32", "33"), ("34", "35"), ("36", "37"), ("38", "39"), ("...", "..."),
            ("52", "53"), ("GND", "GND")]
    x_left, x_right, y0, dy = 140, 260, 120, 44
    svg.rect(x_left - 50, y0 - 24, (x_right - x_left) + 100, dy * len(rows) + 8, fill="#1b2a49", stroke=BLACK)
    legs = {str(pins[0]): "leg 1", str(pins[1]): "leg 2", str(pins[2]): "leg 4", str(pins[3]): "leg 8"}
    for i, (a, b) in enumerate(rows):
        y = y0 + i * dy
        for x, label in ((x_left, a), (x_right, b)):
            use = label in legs
            gnd = label == "GND" and x == x_left
            fill = "#ffd54f" if use else ("#bdbdbd" if gnd else "#455a64")
            svg.rect(x - 14, y - 14, 28, 28, fill=fill, stroke=BLACK, width=1.5)
            svg.text(x, y + 4, label, size=11 if len(label) < 4 else 9, anchor="middle", color=BLACK if (use or gnd) else "#ffffff",
                     weight="bold" if use or gnd else "normal")
        for x, label, side in ((x_left, a, "l"), (x_right, b, "r")):
            if label in legs:
                tx = x - 24 if side == "l" else x + 24
                svg.text(tx, y + 4, "-> switch " + legs[label], size=12, anchor="end" if side == "l" else "start", color=ORANGE, weight="bold")
    svg.text(x_left - 24, y0 + (len(rows) - 1) * dy + 4, "-> switch C", size=12, anchor="end", color=ORANGE, weight="bold")
    svg.text(60, y0 + len(rows) * dy + 30, "Yellow = the four switch pins, grey = GND for the common leg C.", size=11)
    svg.text(60, y0 + len(rows) * dy + 48, "Any other GND pin of the Mega works as well.", size=11, color=GREY)

    # Right: legs table and the other connections.
    tx = 640
    svg.text(tx, 70, "Switch legs", size=13, weight="bold")
    svg.rect(tx, 82, 400, 214, fill="#f3f3f3")
    table = [("Switch leg", "Bit weight", "Mega pin"),
             ("1", "1", "D%d" % pins[0]), ("2", "2", "D%d" % pins[1]), ("4", "4", "D%d" % pins[2]), ("8", "8", "D%d" % pins[3]),
             ("C (one of two)", "common", "GND"), ("C (other)", "-", "free")]
    for r, row in enumerate(table):
        y = 108 + r * 30
        for c, cell in enumerate(row):
            svg.text(tx + 14 + c * 140, y, cell, size=13, weight="bold" if r == 0 else "normal")
        if r < len(table) - 1:
            svg.line(tx, y + 12, tx + 400, y + 12, GREY, 1)
    svg.text(tx, 330, "The legs of the switch are marked 1, 2, 4, 8 and C on its body.", size=11, color=GREY)
    svg.text(tx, 346, "The order of the legs on the case differs between makers: read the marks.", size=11, color=GREY)

    svg.text(tx, 390, "LCD (I2C backpack)", size=13, weight="bold")
    svg.rect(tx, 402, 400, 120, fill="#f3f3f3")
    lcd = [("GND", "GND"), ("VCC", "5V"), ("SDA", "D20"), ("SCL", "D21")]
    for r, (a, b) in enumerate(lcd):
        y = 430 + r * 26
        svg.text(tx + 14, y, a, size=13)
        svg.text(tx + 154, y, "-> Mega " + b, size=13)
    svg.text(tx, 546, "D20 (SDA) and D21 (SCL) are on the communication header; the pins", size=11, color=GREY)
    svg.text(tx, 562, "marked SDA and SCL next to AREF are the same two lines.", size=11, color=GREY)
    (OUT / "layout-diagram.svg").write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % (OUT / "layout-diagram.svg"))


def main():
    pins = read_pins()
    wokwi(pins)
    schematic(pins)
    layout(pins)


if __name__ == "__main__":
    main()
