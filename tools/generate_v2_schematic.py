#!/usr/bin/env python3
"""Draw the electrical schematic of Casino Open Sign V2 as an SVG file (casino_open_sign_v2/wiring-diagram.svg).

Pins and pixel counts are read from ../casino_open_sign_v2/sketch.ino. Run:

    python3 tools/generate_v2_schematic.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import v2_data as v2
from generate_schematic import Svg, RED, BLACK, BLUE, ORANGE, GREY, GREEN

OUTPUT = Path(__file__).resolve().parent.parent / "casino_open_sign_v2" / "wiring-diagram.svg"


def main():
    cfg = v2.config()
    strips = v2.strip_info(cfg)
    sticks = v2.all_stick_info(cfg)              # the 10 arrow sticks + the CLOSED light stick (11 shield connectors)
    total_strips, total_leds = len(strips), sum(s["leds"] for s in strips)
    stick_leds = sum(st["leds"] for st in sticks)
    # One row per data line: the sticks first (shield connectors D2..D8, A0..A3), then the strips.
    rows = []
    for st in sorted(sticks, key=lambda s: v2.connector_sort_key(s["connector"])):
        rows.append(dict(label=st["connector"], name=st["name"], leds=st["leds"], is_stick=True,
                         summary="Grove stick: %s" % st["desc"], feed="feed: Grove cable"))
    for strip in strips:
        rows.append(dict(label="D%d" % strip["pin"], name=strip["name"], leds=strip["leds"], is_stick=False,
                         summary=strip["summary"], feed=strip["feed"]))
    n = len(rows)

    pin_y0, pin_dy = 250, 66                     # y of the first data pin and the pin spacing
    last_y = pin_y0 + (n - 1) * pin_dy
    ard_x0, ard_x1 = 300, 500                    # Arduino block
    ard_y0, ard_y1 = 195, max(last_y + 45 + 700, 1390)
    res_x = 540
    blk_x0, blk_x1 = 690, 940                    # LED strip blocks
    v12_x, gnd_x = 995, 1035                     # vertical +12 V and GND buses (the 12 V strips only)
    rail5_y, railg_y = 72, 112
    height = ard_y1 + 176
    width = 1290
    svg = Svg(width, height)

    svg.text(640, 26, "Casino Open Sign V2 - wiring (Mega 2560, Grove shield with %d sticks, %d WS2811 12 V strips)"
             % (len(sticks), len(strips)), size=16, anchor="middle", weight="bold")

    # ---- Power supply, fuse, capacitor, rails -----------------------------------------
    svg.rect(90, 38, 130, 112, fill="#f3f3f3")
    svg.text(155, 70, "Mean Well", size=12, anchor="middle")
    svg.text(155, 87, "LRS-50-5", size=13, anchor="middle", weight="bold")
    svg.text(155, 104, "5 V / 10 A", size=12, anchor="middle")
    svg.text(155, 121, "PSU1", size=11, anchor="middle", color=GREY)
    for y, label in ((60, "L"), (88, "N"), (116, "PE")):
        svg.line(52, y, 90, y, BLACK)
        svg.text(46, y + 4, label, size=12, anchor="end")
    svg.text(155, 30, "230 V AC mains", size=12, anchor="middle", weight="bold")
    svg.text(90, 166, "mains side: inside an enclosure,", size=10, color=GREY)
    svg.text(90, 178, "wired by a qualified person", size=10, color=GREY)
    svg.text(228, rail5_y - 6, "V+", size=11)
    svg.text(228, railg_y + 14, "V-", size=11)

    # +5 V rail: PSU1 V+ -> fuse -> the Mega 5V pins (and C1); the sticks get their power through the shield
    svg.line(220, rail5_y, 260, rail5_y, RED)
    svg.rect(260, rail5_y - 8, 50, 16, fill="#ffffff", stroke=RED)
    svg.text(285, rail5_y - 14, "F1  10 A fuse", size=12, anchor="middle", color=RED)
    svg.line(310, rail5_y, 640, rail5_y, RED, 2.5)
    svg.text(655, rail5_y + 4, "+5 V (Mega, shield, sticks, LCD)", size=12, color=RED, weight="bold")

    # GND rail from PSU1 V- to the GND bus (hops over the Mega 5V line and the +12 V bus)
    svg.hline_hop(220, gnd_x, railg_y, [360, v12_x], BLACK, 2.5)
    svg.text(gnd_x + 8, railg_y - 8, "GND", size=14, weight="bold")

    # PSU2 12 V (strips only), fuse F2, +12 V bus; V- joins the GND bus (common ground)
    svg.rect(1130, 38, 130, 112, fill="#f3f3f3")
    svg.text(1195, 70, "Mean Well", size=12, anchor="middle")
    svg.text(1195, 87, "LRS-50-12", size=13, anchor="middle", weight="bold")
    svg.text(1195, 104, "12 V / 4.2 A", size=12, anchor="middle")
    svg.text(1195, 121, "PSU2 (strips only)", size=11, anchor="middle", color=GREY)
    svg.text(1195, 30, "230 V AC mains (PSU2)", size=11, anchor="middle", weight="bold")
    svg.text(1125, rail5_y - 14, "V+", size=11, anchor="end")
    svg.text(1125, railg_y + 14, "V-", size=11, anchor="end")
    svg.line(1130, rail5_y, 1100, rail5_y, RED, 2.5)
    svg.rect(1050, rail5_y - 8, 50, 16, fill="#ffffff", stroke=RED)
    svg.text(1066, rail5_y - 14, "F2  5 A fuse", size=12, anchor="middle", color=RED)
    svg.line(1050, rail5_y, v12_x, rail5_y, RED, 2.5)
    svg.dot(v12_x, rail5_y, RED)
    svg.text(v12_x - 8, rail5_y + 22, "+12 V", size=14, color=RED, weight="bold", anchor="end")
    svg.line(1130, railg_y, gnd_x, railg_y, BLACK, 2.5)
    svg.dot(gnd_x, railg_y)

    # Capacitor C1 between the 5 V rail and GND
    cx = 540
    svg.dot(cx, rail5_y, RED)
    svg.dot(cx, railg_y)
    svg.line(cx, rail5_y, cx, 86, RED)
    svg.line(cx - 11, 86, cx + 11, 86, BLACK, 3)
    svg.path("M %g 96 Q %g 90 %g 96" % (cx - 11, cx, cx + 11), BLACK, 3)
    svg.line(cx, 96, cx, railg_y, BLACK)
    svg.text(cx - 16, 84, "+", size=14, anchor="end", weight="bold", color=RED)
    svg.text(cx + 18, 84, "C1  1000 uF / 16 V", size=12)
    svg.text(cx + 18, 99, "(stripe on the body = minus)", size=10, color=GREY)

    # ---- Arduino block ------------------------------------------------------------------
    svg.rect(ard_x0, ard_y0, ard_x1 - ard_x0, ard_y1 - ard_y0, fill="#e8f0fe")
    svg.text((ard_x0 + ard_x1) / 2, ard_y0 + 27, "Arduino Mega 2560", size=15, anchor="middle", weight="bold")
    # GND pin on the top edge
    gx = 420
    svg.text(gx, ard_y0 + 60, "GND", size=12, anchor="middle")
    svg.line(gx, ard_y0, gx, railg_y, BLACK)
    svg.dot(gx, railg_y)
    svg.text(gx + 8, ard_y0 - 8, "any GND pin", size=10, color=GREY)
    # 5V pins on the top edge: the LED supply feeds the Mega 5V pins (the sticks get their power through the shield)
    fx5 = 360
    svg.text(fx5, ard_y0 + 60, "5V", size=12, anchor="middle")
    svg.line(fx5, ard_y0, fx5, rail5_y, RED, 2.5)
    svg.dot(fx5, rail5_y, RED)
    svg.text(fx5 - 8, ard_y0 - 8, "both 5V pins (long header)", size=10, color=GREY, anchor="end")
    svg.text(gx + 8, ard_y0 - 20, "GND: common with PSU1 V- and PSU2 V-", size=10, color=GREY)

    # ---- Data pins -> resistors -> strips ------------------------------------------------
    resistor_no = 0
    for i, row in enumerate(rows):
        label, name, leds, desc, feed = row["label"], row["name"], row["leds"], row["summary"], row["feed"]
        y = pin_y0 + i * pin_dy
        svg.text(ard_x1 - 8, y + 4, label, size=12, anchor="end")
        if row["is_stick"]:
            # Grove connector of the shield: the signal wire goes straight to the stick (no resistor)
            svg.line(ard_x1, y, blk_x0, y, BLUE)
            svg.text(res_x, y - 8, "Grove SIG, no resistor", size=10, color=GREY)
        else:
            resistor_no += 1
            svg.line(ard_x1, y, res_x, y, BLUE)
            svg.resistor(res_x, y, "R%d  330 ohm" % resistor_no)
            svg.line(res_x + 60, y, blk_x0, y, BLUE)
        svg.circle(blk_x0, y, 3.5, BLUE)

        svg.rect(blk_x0, y - 27, blk_x1 - blk_x0, 54, fill="#fff8e1")
        svg.text(blk_x0 + 12, y - 8, "%s" % name, size=13, weight="bold")
        svg.text(blk_x0 + 12, y + 8, "%d %s, %s" % (leds, "LEDs" if row["is_stick"] else "pixels", "WS2813 (stick)" if row["is_stick"] else "WS2811 12 V"), size=11)
        svg.text(blk_x0 + 12, y + 22, desc, size=10, color=GREY)
        svg.text(blk_x0 - 8, y - 7, "DIN", size=10, anchor="end", color=BLUE)
        if row["is_stick"]:
            # The sticks take +5 V and GND only through the Grove cable of the shield: no wire to the supply buses.
            svg.text(blk_x1 - 4, y - 14, "+5V, GND: via the shield", size=10, anchor="end", color=GREY)
            svg.text(blk_x1 - 4, y + 3, "(Grove cable)", size=9, anchor="end", color=GREY, italic=True)
            continue
        svg.text(blk_x1 - 4, y - 14, "+12V", size=10, anchor="end", color=RED)
        svg.text(blk_x1 - 4, y + 19, "GND", size=10, anchor="end")
        svg.text(blk_x1 - 4, y + 3, feed, size=9, anchor="end", color=GREY, italic=True)

        # +5 V terminal (top right) to the +5 V bus, GND terminal (bottom right) to the GND bus
        y5, yg = y - 17, y + 17
        svg.line(blk_x1, y5, v12_x, y5, RED)
        svg.dot(v12_x, y5, RED)
        svg.hline_hop(blk_x1, gnd_x, yg, [v12_x], BLACK)
        svg.dot(gnd_x, yg)

    # vertical buses
    c2_y = last_y + 62
    svg.line(v12_x, rail5_y, v12_x, c2_y, RED, 2.5)
    svg.line(gnd_x, railg_y, gnd_x, c2_y, BLACK, 2.5)
    # Capacitor C2 across +12 V and GND at the strips
    svg.line(v12_x, c2_y, v12_x + 12, c2_y, RED)
    svg.line(v12_x + 12, c2_y - 11, v12_x + 12, c2_y + 11, BLACK, 3)
    svg.path("M %g %g Q %g %g %g %g" % (v12_x + 24, c2_y - 11, v12_x + 18, c2_y, v12_x + 24, c2_y + 11), BLACK, 3)
    svg.line(v12_x + 24, c2_y, gnd_x, c2_y, BLACK)
    svg.dot(v12_x, c2_y, RED)
    svg.dot(gnd_x, c2_y)
    svg.text(v12_x + 12, c2_y + 28, "C2  1000 uF / 25 V (plus to +12 V)", size=11, anchor="middle")

    # ---- Inputs (left of the Arduino) -----------------------------------------------------
    def slide_switch(sw_y, pin, title, lines):
        svg.text(ard_x0 + 8, sw_y + 4, "D%d" % pin, size=12)
        svg.line(ard_x0, sw_y, 222, sw_y, ORANGE)
        svg.circle(222, sw_y, 3.5, ORANGE)
        svg.circle(140, sw_y + 22, 3.5, ORANGE)                 # contact 1 (GND side)
        svg.circle(140, sw_y - 22, 3.5, ORANGE)                 # contact 3 (not connected)
        svg.line(222, sw_y, 144, sw_y + 20, ORANGE, 2.5)        # lever, shown toward GND
        svg.line(222, sw_y, 144, sw_y - 20, GREY, 1.5, dash="4 3")  # lever, other position
        svg.line(140, sw_y + 26, 140, sw_y + 44, ORANGE)
        svg.ground(140, sw_y + 44)
        svg.text(226, sw_y - 8, "2 (common)", size=10, color=GREY)
        svg.text(132, sw_y + 17, "1", size=10, anchor="end", color=GREY)
        svg.text(132, sw_y - 19, "3", size=10, anchor="end", color=GREY)
        svg.text(40, sw_y - 82, title, size=12, weight="bold")
        for k, line in enumerate(lines):
            svg.text(40, sw_y - 67 + k * 12, line, size=10, color=GREY)
        svg.text(40, sw_y + 90, "pin 3 stays unconnected", size=10, color=GREY)

    slide_switch(345, cfg["PIN_ARROW_SWITCH"], "SW1  slide switch (SPDT)",
                 ["arrow direction: handle on the GND side", "= LOW = arrow points left", "mount: back view, GND contact on the right"])
    slide_switch(1090, cfg["PIN_CASINO_CLOSED_SWITCH"], "SW5  slide switch (SPDT)",
                 ["casino closed: handle on the GND side = LOW =", "CLOSED (OPEN letters off, CLOSED light on);",
                  "handle on the other side = casino open"])

    bt_y = 540
    svg.text(ard_x0 + 8, bt_y + 4, "D%d" % cfg["PIN_MODE_BUTTON"], size=12)
    svg.line(ard_x0, bt_y, 222, bt_y, ORANGE)
    svg.circle(222, bt_y, 3.5, ORANGE)
    svg.circle(140, bt_y, 3.5, ORANGE)
    svg.line(134, bt_y - 14, 228, bt_y - 14, BLACK, 2.5)
    svg.line(181, bt_y - 14, 181, bt_y - 28, BLACK, 2)
    svg.line(140, bt_y + 4, 140, bt_y + 30, ORANGE)
    svg.ground(140, bt_y + 30)
    svg.text(40, bt_y - 58, "SW2  push button (momentary)", size=12, weight="bold")
    svg.text(40, bt_y - 43, "each press = next letter mode", size=10, color=GREY)

    # ---- Hex rotary switches (speed) -----------------------------------------------------------
    def hex_switch(y0, title, subtitle, pins):
        svg.text(40, y0 - 26, title, size=12, weight="bold")
        svg.text(40, y0 - 12, subtitle, size=10, color=GREY)
        svg.rect(60, y0, 140, 110, fill="#f3f3f3")
        svg.circle(104, y0 + 52, 22, BLACK)
        svg.line(104, y0 + 52, 104, y0 + 34, BLACK, 2.5)
        svg.dot(104, y0 + 52, BLACK, 3)
        svg.text(104, y0 + 95, "0 .. F", size=11, anchor="middle")
        for k, (weight, pin) in enumerate(zip((1, 2, 4, 8), pins)):
            y = y0 + 25 + k * 24
            svg.text(192, y + 4, str(weight), size=11, anchor="end")
            svg.circle(200, y, 3.5, ORANGE)
            svg.line(200, y, ard_x0, y, ORANGE)
            svg.text(ard_x0 + 8, y + 4, "D%d" % pin, size=12)
        svg.text(150, y0 + 102, "C", size=11, anchor="middle")
        svg.line(150, y0 + 110, 150, y0 + 122, ORANGE)
        svg.circle(150, y0 + 110, 3.5, ORANGE)
        svg.ground(150, y0 + 122)

    hex_switch(650, "SW3  hex rotary switch 0-F", "speed of the frame and the arrow", cfg["PIN_SPEED_FRAME_ARROW"])
    hex_switch(832, "SW4  hex rotary switch 0-F", "speed of the letters", cfg["PIN_SPEED_LETTERS"])

    # ---- I2C LCD (below SW5) --------------------------------------------------------------------
    ly0 = 1290
    svg.text(40, ly0 - 40, "LCD1  2x16 character LCD, I2C backpack", size=12, weight="bold")
    svg.text(40, ly0 - 26, "shows mode, speeds, casino state, arrow", size=10, color=GREY)
    svg.text(40, ly0 - 14, "(I2C address 0x27, try 0x3F if blank)", size=10, color=GREY)
    svg.rect(60, ly0, 140, 96, fill="#f3f3f3")
    svg.text(100, ly0 + 54, "LCD", size=12, anchor="middle", weight="bold")
    svg.text(100, ly0 + 70, "16 x 2", size=11, anchor="middle")
    for k, (pin, target, color) in enumerate((("GND", None, BLACK), ("VCC", "5V", RED),
                                              ("SDA", "D20", GREEN), ("SCL", "D21", GREEN))):
        y = ly0 + 14 + k * 24
        svg.text(192, y + 4, pin, size=11, anchor="end")
        svg.circle(200, y, 3.5, color)
        if target is None:
            svg.line(200, y, 214, y, BLACK)
            svg.ground(214, y)
        else:
            svg.line(200, y, ard_x0, y, color)
            svg.text(ard_x0 + 8, y + 4, target + (" (SDA)" if pin == "SDA" else " (SCL)" if pin == "SCL" else " pin"),
                     size=12)

    # ---- USB note ---------------------------------------------------------------------------
    svg.rect(ard_x0, ard_y1 + 18, 420, 78, fill="#ffffff", stroke=RED, dash="5 4")
    svg.text(ard_x0 + 10, ard_y1 + 38, "USB-B cable (upload, Serial Monitor): ONLY while PSU1 (5 V) is", size=11, weight="bold")
    svg.text(ard_x0 + 10, ard_y1 + 53, "switched OFF. PSU1 feeds the Mega 5V pins (the sticks get their", size=11)
    svg.text(ard_x0 + 10, ard_y1 + 68, "power through the Grove shield), so with USB plugged in as well", size=11)
    svg.text(ard_x0 + 10, ard_y1 + 83, "two 5 V sources would be connected together. Switch PSU2 off too.", size=11)

    # ---- Legend and notes -----------------------------------------------------------------
    ny = ard_y1 + 124
    svg.line(40, ny - 4, 80, ny - 4, RED, 2.5)
    svg.text(88, ny, "+5 V / +12 V power", size=11)
    svg.line(230, ny - 4, 270, ny - 4, BLACK, 2.5)
    svg.text(278, ny, "GND", size=11)
    svg.line(330, ny - 4, 370, ny - 4, BLUE, 2)
    svg.text(378, ny, "data (via 330 ohm)", size=11)
    svg.line(500, ny - 4, 540, ny - 4, ORANGE, 2)
    svg.text(548, ny, "inputs", size=11)
    svg.line(610, ny - 4, 650, ny - 4, GREEN, 2)
    svg.text(658, ny, "I2C", size=11)
    svg.path("M 710 %g L 720 %g A 6 6 0 0 1 732 %g L 742 %g" % (ny - 4, ny - 4, ny - 4, ny - 4))
    svg.text(750, ny, "hop = no connection; dot = connection", size=11)

    # Notes go into the empty space right of the Arduino, below the strip blocks.
    notes = [
        "Total: %d pixels (%d LEDs) in %d WS2811 12 V strips plus %d Grove sticks with %d LEDs each (10 arrow sticks + the CLOSED light). Positions: layout-diagram.svg, ../arrow_sticks/." % (total_leds, 3 * total_leds, total_strips, len(sticks), cfg["LEDS_PER_STICK"]),
        "1. Two supplies: PSU1 (LRS-50-5, 5 V) feeds the Mega 5V pins (both), the Grove shield, the sticks and the LCD; PSU2 (LRS-50-12, 12 V) feeds ONLY the two strips. Never connect +12 V to an Arduino pin or to the 5 V parts.",
        "2. Common ground: PSU1 V-, PSU2 V-, Arduino GND (and the shield) and the GND of both strips and all sticks are connected together.",
        "3. D11 = frame strip, 18 pixels, pixel 1 in the lower-left corner, counter-clockwise (front view): 1-6 bottom, 7-9 right going up, 10-15 top right to left, 16-18 left going down. Feed +12 V at the start and the end.",
        "4. D12 = letters strip, 10 pixels (one per letter): 1-6 C A S I N O, then 7-10 N E P O. OPEN is wired in reverse order on the strip; the program maps the letters. D13 stays the status LED.",
        "5. Put each 330 ohm resistor close to the first pixel of its strip. Keep data wires short. The sticks plug straight into the Grove shield (no resistors, no wire from a supply).",
        "6. The Mega's 5 V data signal into a 12 V WS2811 strip normally works (as in the strip tests) but is not verified for the final sign; the 74HCT125 buffer is listed as insurance.",
        "7. SW3 and SW4: pins 1, 2, 4, 8 go to D22-D25 and D26-D29, the common pin C goes to GND. A position counts in binary (0..F). "
        "If your switch gives the complement (15 - position), set SPEED_BIT_IS_LOW to false in sketch.ino.",
        "8. All inputs (SW1 to SW5) use the internal pull-ups (INPUT_PULLUP), so no resistors are needed for them.",
        "9. CLOSED (the 11th Grove stick, 10 white LEDs, shield connector A0) sits BEHIND the plate under the CLOSED hole text and is lit only while SW5 says closed.",
        "10. LCD1 (I2C backpack) uses the Mega's I2C pins D20 (SDA) and D21 (SCL) plus the Mega 5V and GND pins (about 30 mA with the backlight).",
        "11. Arrow sticks: the 10 Grove RGB LED sticks go into the Grove Base Shield connectors D2-D8 and A1-A3 (A0 carries the CLOSED light stick; the connector of each stick is listed in the data rows above; the shield's 3V3/5V switch "
        "must be on 5 V). Connect the +5 V bus to BOTH 5V pins of the Mega's long header and the GND bus to the Mega GND pins, as in stick_test_direct (tested on real hardware).",
        "12. Pin 13 (the LED on the Mega board) blinks while the program runs; the Serial Monitor (115200 baud) shows one line per second.",
    ]
    import textwrap
    note_x, note_y = ard_x1 + 40, last_y + 100
    svg.text(note_x, note_y, "Notes", size=12, weight="bold")
    line_no = 0
    for note in notes:
        for k, line in enumerate(textwrap.wrap(note, 112, subsequent_indent="    ")):
            svg.text(note_x, note_y + 22 + line_no * 16, line, size=11.5)
            line_no += 1
        line_no += 0.4
    assert note_y + 22 + line_no * 16 < ard_y1, "notes do not fit into the free space"

    OUTPUT.write_text(svg.render_a4(landscape=False))
    print("A4 portrait, text height of a 12-unit label: %.2f mm" % (12 * svg.mm_per_unit))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
