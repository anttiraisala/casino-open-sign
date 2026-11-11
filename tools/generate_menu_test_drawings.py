#!/usr/bin/env python3
"""Generate the drawings and the Wokwi circuit of the settings menu test (menu_test/).

    python3 tools/generate_menu_test_drawings.py

Output in menu_test/: diagram.json (Wokwi), wiring-diagram.svg (electrical schematic, A4 portrait) and layout-diagram.svg
(the pins on the Mega, A4 landscape). The PDFs are made by tools/export_pdf.py. The pins are read from menu_test/sketch.ino.
"""

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import BLACK, BLUE, GREEN, GREY, ORANGE, RED, Svg

OUT = Path(__file__).resolve().parent.parent / "menu_test"
TEXT = (OUT / "sketch.ino").read_text()


def pin_array(name):
    m = re.search(name + r"\[4\]\s*=\s*\{([^}]*)\}", TEXT)
    return [int(v) for v in m.group(1).split(",")]


def pin_const(name):
    return int(re.search(name + r"\s*=\s*(\d+)", TEXT).group(1))


PROGRAM = pin_array("PROGRAM_PINS")
MENU = pin_array("MENU_PINS")
OK, BACK, SHIFT = pin_const("PIN_OK"), pin_const("PIN_BACK"), pin_const("PIN_SHIFT")
ARROW, CASINO = pin_const("PIN_ARROW_SWITCH"), pin_const("PIN_CASINO_SWITCH")


def wokwi():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -170, "left": 20,
         "attrs": {"text": "Settings menu test: program switch + menu switch + OK/BACK/SHIFT + 2 slide switches + LCD"}},
        {"type": "wokwi-text", "id": "t2", "top": -152, "left": 20,
         "attrs": {"text": "Wokwi has no hex rotary switch: a DIP bank stands in (ON = leg connected to C = bit set)"}},
        {"type": "wokwi-dip-switch-8", "id": "prog", "top": -120, "left": 20, "attrs": {}},
        {"type": "wokwi-dip-switch-8", "id": "menu", "top": -120, "left": 200, "attrs": {}},
        {"type": "wokwi-pushbutton", "id": "ok", "top": -120, "left": 400, "attrs": {"color": "green"}},
        {"type": "wokwi-pushbutton", "id": "back", "top": -120, "left": 470, "attrs": {"color": "red"}},
        {"type": "wokwi-pushbutton", "id": "shift", "top": -120, "left": 540, "attrs": {"color": "blue"}},
        {"type": "wokwi-slide-switch", "id": "arrow", "top": -60, "left": 400, "attrs": {}},
        {"type": "wokwi-slide-switch", "id": "casino", "top": -60, "left": 480, "attrs": {}},
        {"type": "wokwi-lcd1602", "id": "lcd", "top": 480, "left": 20, "attrs": {"pins": "i2c"}},
    ]
    c = []
    for k, pin in enumerate(PROGRAM):
        c += [["mega:%d" % pin, "prog:%db" % (k + 1), "orange", []], ["mega:GND.1", "prog:%da" % (k + 1), "black", []]]
    for k, pin in enumerate(MENU):
        c += [["mega:%d" % pin, "menu:%db" % (k + 1), "orange", []], ["mega:GND.1", "menu:%da" % (k + 1), "black", []]]
    for name, pin in (("ok", OK), ("back", BACK), ("shift", SHIFT)):
        c += [["mega:%d" % pin, "%s:1.l" % name, "orange", []], ["mega:GND.1", "%s:2.l" % name, "black", []]]
    for name, pin in (("arrow", ARROW), ("casino", CASINO)):
        c += [["mega:%d" % pin, "%s:2" % name, "orange", []], ["mega:GND.1", "%s:3" % name, "black", []]]
    c += [["mega:5V", "lcd:VCC", "red", []], ["mega:GND.1", "lcd:GND", "black", []],
          ["mega:20", "lcd:SDA", "green", []], ["mega:21", "lcd:SCL", "green", []]]
    doc = {"version": 1, "author": "Casino sign project - settings menu test", "editor": "wokwi",
           "parts": parts, "connections": c, "dependencies": {}}
    (OUT / "diagram.json").write_text(json.dumps(doc, indent=2) + "\n")
    print("Wrote %s" % (OUT / "diagram.json"))


def hex_switch(svg, x, y, title, subtitle, pins, ard_x0):
    svg.text(x, y - 26, title, size=13, weight="bold")
    svg.text(x, y - 10, subtitle, size=10, color=GREY)
    svg.rect(x + 20, y, 130, 130, fill="#f3f3f3")
    svg.circle(x + 70, y + 50, 24, BLACK)
    svg.line(x + 70, y + 50, x + 70, y + 30, BLACK, 2.5)
    svg.dot(x + 70, y + 50, BLACK, 3)
    svg.text(x + 70, y + 100, "0 .. F", size=12, anchor="middle")
    for k, (weight, pin) in enumerate(zip((1, 2, 4, 8), pins)):
        yy = y + 20 + k * 24
        svg.text(x + 140, yy + 5, "leg %d" % weight, size=14, anchor="end", weight="bold")
        svg.circle(x + 150, yy, 3.5, ORANGE)
        svg.line(x + 150, yy, ard_x0, yy, ORANGE)
        svg.text(ard_x0 + 8, yy + 4, "D%d" % pin, size=13)
    yc = y + 130
    svg.circle(x + 100, yc, 3.5, ORANGE)
    svg.line(x + 100, yc, x + 100, yc + 10, ORANGE)
    svg.ground(x + 100, yc + 10)
    svg.text(x + 122, yc + 5, "C (common) -> GND", size=12, weight="bold")


def button(svg, x, y, name, pin, ard_x0):
    svg.text(x, y - 6, name, size=12, weight="bold")
    svg.circle(x + 40, y + 10, 3.5, ORANGE)
    svg.line(x + 40, y + 10, ard_x0, y + 10, ORANGE)
    svg.text(ard_x0 + 8, y + 14, "D%d" % pin, size=13)
    svg.line(x + 40, y + 10, x + 40, y + 22, BLACK)
    svg.line(x + 30, y + 22, x + 50, y + 22, BLACK, 2)
    svg.circle(x + 40, y + 32, 3.5, ORANGE)
    svg.line(x + 40, y + 32, x + 40, y + 40, ORANGE)
    svg.ground(x + 40, y + 40)


def schematic():
    width = 940
    svg = Svg(width, 1200)
    svg.text(width / 2, 22, "Settings menu test - wiring", size=17, anchor="middle", weight="bold")
    ard_x0, ard_x1 = 600, 910
    svg.rect(ard_x0, 50, ard_x1 - ard_x0, 800, fill="#e8f0fe")
    svg.text((ard_x0 + ard_x1) / 2, 74, "Arduino Mega 2560", size=14, anchor="middle", weight="bold")
    svg.text((ard_x0 + ard_x1) / 2, 92, "powered over USB for this test", size=11, anchor="middle", color=GREY)

    hex_switch(svg, 40, 140, "PSW  program hex rotary switch", "absolute position 0-F = the program number", PROGRAM, ard_x0)
    hex_switch(svg, 40, 340, "MSW  menu hex rotary switch", "free-running: moves the menu / changes a value", MENU, ard_x0)

    svg.text(40, 520, "Push buttons (to GND, internal pull-ups; no resistors)", size=13, weight="bold")
    # buttons stacked on the left
    for i, (name, pin) in enumerate((("OK", OK), ("BACK", BACK), ("SHIFT (hold while turning = x10)", SHIFT))):
        button(svg, 60, 560 + i * 70, name, pin, ard_x0)
    # slide switches
    svg.text(40, 790, "Slide switches (middle contact to the pin, one side contact to GND)", size=13, weight="bold")
    for i, (name, pin, low) in enumerate((("Arrow switch", ARROW, "handle on the GND side = arrow points left"),
                                          ("Casino switch", CASINO, "handle on the GND side = casino closed"))):
        y = 820 + i * 60
        svg.text(60, y, name, size=12, weight="bold")
        svg.text(60, y + 16, low, size=10, color=GREY)
        svg.circle(330, y + 6, 3.5, ORANGE)
        svg.line(330, y + 6, ard_x0, y + 6, ORANGE)
        svg.text(ard_x0 + 8, y + 10, "D%d" % pin, size=13)
        svg.text(340, y - 6, "middle contact", size=10, color=GREY)

    # LCD
    ly = 960
    svg.text(40, ly - 20, "LCD  2x16 character LCD, I2C backpack (address 0x27)", size=13, weight="bold")
    svg.rect(60, ly, 150, 100, fill="#f3f3f3")
    svg.text(135, ly + 54, "LCD 16 x 2", size=13, anchor="middle", weight="bold")
    for k, (name, target, color) in enumerate((("GND", None, BLACK), ("VCC", "5V pin", RED),
                                               ("SDA", "D20 (SDA)", GREEN), ("SCL", "D21 (SCL)", GREEN))):
        y = ly + 14 + k * 24
        svg.text(202, y + 4, name, size=11, anchor="end")
        svg.circle(210, y, 3.5, color)
        if target is None:
            svg.line(210, y, 226, y, BLACK)
            svg.ground(226, y)
        else:
            svg.line(210, y, ard_x0, y, color)
            svg.text(ard_x0 + 8, y + 4, target, size=13)
    svg.text(ard_x0 + 8, 114, "Pin 13: onboard LED = sign of life", size=11)
    svg.height = 1090
    (OUT / "wiring-diagram.svg").write_text(svg.render_a4(landscape=False))
    print("Wrote %s" % (OUT / "wiring-diagram.svg"))


def layout():
    width, height = 1100, 700
    svg = Svg(width, height)
    svg.text(width / 2, 28, "Settings menu test - where the wires go (not to scale)", size=18, anchor="middle", weight="bold")
    svg.text(60, 66, "Mega 2560: long double-row header (from above)", size=12, weight="bold")
    used = {str(p): ("program leg %d" % w) for p, w in zip(PROGRAM, (1, 2, 4, 8))}
    used.update({str(p): ("menu leg %d" % w) for p, w in zip(MENU, (1, 2, 4, 8))})
    used.update({str(OK): "OK", str(BACK): "BACK", str(SHIFT): "SHIFT", str(ARROW): "arrow switch", str(CASINO): "casino switch"})
    rows = [("5V", "5V"), ("30", "31"), ("32", "33"), ("34", "35"), ("36", "37"), ("38", "39"), ("40", "41"), ("42", "43"),
            ("44", "45"), ("46", "47"), ("48", "49"), ("50", "51"), ("52", "53"), ("GND", "GND")]
    x_left, x_right, y0, dy = 230, 350, 110, 38
    svg.rect(x_left - 50, y0 - 22, (x_right - x_left) + 100, dy * len(rows) + 6, fill="#1b2a49", stroke=BLACK)
    for i, (a, b) in enumerate(rows):
        y = y0 + i * dy
        for x, label, side in ((x_left, a, "l"), (x_right, b, "r")):
            use = label in used
            gnd = label == "GND" and x == x_left
            fill = "#ffd54f" if use else ("#bdbdbd" if gnd else "#455a64")
            svg.rect(x - 14, y - 13, 28, 26, fill=fill, stroke=BLACK, width=1.5)
            svg.text(x, y + 4, label, size=11 if len(label) < 4 else 9, anchor="middle", color=BLACK if (use or gnd) else "#ffffff",
                     weight="bold" if use or gnd else "normal")
            if use:
                tx = x - 62 if side == "l" else x + 62
                svg.text(tx, y + 4, "-> " + used[label], size=12, anchor="end" if side == "l" else "start", color=ORANGE, weight="bold")
    svg.text(60, y0 + len(rows) * dy + 30, "Yellow = used pins; grey = GND (all C legs, buttons, slide switches and the LCD GND go to GND pins).", size=11)

    tx = 640
    svg.text(tx, 66, "What goes where", size=13, weight="bold")
    svg.rect(tx, 78, 420, 330, fill="#f3f3f3")
    table = [("Part", "Pin / contact", "Mega pin"),
             ("Program hex switch", "legs 1, 2, 4, 8", "D%d D%d D%d D%d" % tuple(PROGRAM)),
             ("", "C (one leg)", "GND"),
             ("Menu hex switch", "legs 1, 2, 4, 8", "D%d D%d D%d D%d" % tuple(MENU)),
             ("", "C (one leg)", "GND"),
             ("OK button", "one contact", "D%d" % OK), ("BACK button", "one contact", "D%d" % BACK), ("SHIFT button", "one contact", "D%d" % SHIFT),
             ("", "other contact", "GND"),
             ("Arrow slide switch", "middle contact", "D%d" % ARROW), ("Casino slide switch", "middle contact", "D%d" % CASINO),
             ("", "one side contact", "GND"),
             ("LCD backpack", "GND / VCC", "GND / 5V"), ("", "SDA / SCL", "D20 / D21")]
    for r, row in enumerate(table):
        y = 102 + r * 23
        for c, cell in enumerate(row):
            svg.text(tx + 12 + (0, 150, 290)[c], y, cell, size=11, weight="bold" if r == 0 else "normal")
        if r < len(table) - 1:
            svg.line(tx, y + 8, tx + 420, y + 8, GREY, 1)
    svg.text(tx, 436, "Rotary switch legs are marked 1, 2, 4, 8 and C (the order differs by maker).", size=10, color=GREY)
    svg.text(tx, 452, "A slide switch has three contacts in a row: the middle one is the common contact.", size=10, color=GREY)
    svg.text(tx, 468, "D20 (SDA) / D21 (SCL): communication header, or the SDA / SCL pins by AREF.", size=10, color=GREY)
    (OUT / "layout-diagram.svg").write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % (OUT / "layout-diagram.svg"))


if __name__ == "__main__":
    wokwi()
    schematic()
    layout()
