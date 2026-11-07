#!/usr/bin/env python3
"""Draw the electrical schematic of the casino sign as an SVG file.

The pins and the LED counts are read from the CONFIGURATION section of
../sketch.ino, so the schematic always matches the sketch. Run it after changing
any strip length, pin or the text:

    python3 tools/generate_schematic.py

Output: wiring-diagram.svg in the project folder.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_diagram as gd  # reuses read_config() and the path of sketch.ino
import sign_data

OUTPUT = Path(__file__).resolve().parent.parent / "wiring-diagram.svg"

RED, BLACK, BLUE, ORANGE, GREY = "#c62828", "#222222", "#1565c0", "#ef6c00", "#666666"
GREEN = "#2e7d32"                                # I2C lines
FONT = "font-family='DejaVu Sans, Arial, sans-serif'"


class Svg:
    def __init__(self, width, height):
        self.width, self.height = width, height
        self.items = []

    def add(self, text):
        self.items.append(text)

    def line(self, x1, y1, x2, y2, color=BLACK, width=2, dash=None):
        extra = " stroke-dasharray='%s'" % dash if dash else ""
        self.add("<line x1='%g' y1='%g' x2='%g' y2='%g' stroke='%s' stroke-width='%g'%s/>"
                 % (x1, y1, x2, y2, color, width, extra))

    def path(self, d, color=BLACK, width=2, fill="none"):
        self.add("<path d='%s' stroke='%s' stroke-width='%g' fill='%s' stroke-linejoin='round'/>"
                 % (d, color, width, fill))

    def rect(self, x, y, w, h, fill="#ffffff", stroke=BLACK, width=2, dash=None, rx=0):
        extra = " stroke-dasharray='%s'" % dash if dash else ""
        self.add("<rect x='%g' y='%g' width='%g' height='%g' rx='%g' fill='%s' stroke='%s' stroke-width='%g'%s/>"
                 % (x, y, w, h, rx, fill, stroke, width, extra))

    def dot(self, x, y, color=BLACK, r=4):
        self.add("<circle cx='%g' cy='%g' r='%g' fill='%s'/>" % (x, y, r, color))

    def circle(self, x, y, r=3.5, color=BLACK):
        self.add("<circle cx='%g' cy='%g' r='%g' fill='#ffffff' stroke='%s' stroke-width='2'/>" % (x, y, r, color))

    def text(self, x, y, s, size=13, anchor="start", color=BLACK, weight="normal", italic=False):
        s = s.replace("&", "&amp;").replace("<", "&lt;")
        style = " font-style='italic'" if italic else ""
        self.add("<text x='%g' y='%g' font-size='%g' text-anchor='%s' fill='%s' font-weight='%s'%s %s>%s</text>"
                 % (x, y, size, anchor, color, weight, style, FONT, s))

    def ground(self, x, y, color=BLACK):
        """Ground symbol hanging from (x, y)."""
        self.line(x, y, x, y + 8, color)
        self.line(x - 9, y + 8, x + 9, y + 8, color)
        self.line(x - 6, y + 12, x + 6, y + 12, color)
        self.line(x - 3, y + 16, x + 3, y + 16, color)

    def hline_hop(self, x1, x2, y, hops, color, width=2):
        """Horizontal wire from x1 to x2 with small hops over vertical wires at x positions `hops`."""
        r = 6
        d = "M %g %g" % (x1, y)
        direction = 1 if x2 > x1 else -1
        for hx in sorted(hops, reverse=direction < 0):
            d += " L %g %g A %g %g 0 0 %d %g %g" % (hx - direction * r, y, r, r, 1 if direction > 0 else 0,
                                                     hx + direction * r, y)
        d += " L %g %g" % (x2, y)
        self.path(d, color, width)

    def resistor(self, x, y, label):
        """Horizontal IEC resistor symbol with its left end at x and centre line y."""
        self.line(x, y, x + 8, y, BLUE)
        self.rect(x + 8, y - 8, 44, 16, fill="#ffffff", stroke=BLUE)
        self.line(x + 52, y, x + 60, y, BLUE)
        self.text(x + 30, y - 13, label, size=12, anchor="middle", color=BLUE)

    def render_a4(self, landscape, margin_mm=8.0):
        """SVG that is exactly one A4 page (width/height in mm); the drawing is scaled to fit
        inside the margins and centred, so it prints at 100 % without any printer scaling."""
        pw, ph = (297.0, 210.0) if landscape else (210.0, 297.0)
        # content units per mm so that the whole drawing fits inside the printable area
        k = max(self.width / (pw - 2 * margin_mm), self.height / (ph - 2 * margin_mm))
        vb_w, vb_h = pw * k, ph * k
        vb_x, vb_y = (self.width - vb_w) / 2, (self.height - vb_h) / 2
        self.mm_per_unit = 1.0 / k
        return ("<svg xmlns='http://www.w3.org/2000/svg' width='%gmm' height='%gmm' viewBox='%g %g %g %g'>\n"
                "<rect x='%g' y='%g' width='%g' height='%g' fill='#ffffff'/>\n%s\n</svg>\n"
                % (pw, ph, vb_x, vb_y, vb_w, vb_h, vb_x, vb_y, vb_w, vb_h, "\n".join(self.items)))

    def render(self):
        return ("<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' viewBox='0 0 %d %d'>\n"
                "<rect width='%d' height='%d' fill='#ffffff'/>\n%s\n</svg>\n"
                % (self.width, self.height, self.width, self.height, self.width, self.height,
                   "\n".join(self.items)))


def main():
    cfg = gd.read_config(gd.SKETCH.read_text())
    strips = sign_data.strip_info(cfg)
    n = len(strips)
    total_strips, total_pieces, total_leds = sign_data.totals(strips)

    pin_y0, pin_dy = 250, 66                     # y of the first data pin and the pin spacing
    last_y = pin_y0 + (n - 1) * pin_dy
    ard_x0, ard_x1 = 300, 500                    # Arduino block
    ard_y0, ard_y1 = 195, max(last_y + 45, 1390)
    res_x = 540
    blk_x0, blk_x1 = 690, 940                    # LED strip blocks
    v5_x, gnd_x = 995, 1035                      # vertical +5 V and GND buses
    rail5_y, railg_y = 72, 112
    height = ard_y1 + 160
    width = 1200
    svg = Svg(width, height)

    svg.text(width / 2, 26, "Casino sign - electrical schematic (Arduino Mega 2560 + %d WS2812B strips)" % n,
             size=17, anchor="middle", weight="bold")

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

    # +5 V rail: PSU V+ -> fuse -> right to the +5 V bus
    svg.line(220, rail5_y, 260, rail5_y, RED)
    svg.rect(260, rail5_y - 8, 50, 16, fill="#ffffff", stroke=RED)
    svg.text(285, rail5_y - 14, "F1  10 A fuse", size=12, anchor="middle", color=RED)
    # +5 V rail runs to the +5 V bus
    svg.line(310, rail5_y, v5_x, rail5_y, RED, 2.5)
    svg.text(v5_x + 10, rail5_y + 4, "+5 V", size=14, color=RED, weight="bold")

    # GND rail from PSU V- to the GND bus (hops over the +5 V bus)
    svg.hline_hop(220, gnd_x, railg_y, [v5_x], BLACK, 2.5)
    svg.text(gnd_x + 10, railg_y + 4, "GND", size=14, weight="bold")

    # Capacitor C1 between the rails
    cx = 640
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

    # ---- Data pins -> resistors -> strips ------------------------------------------------
    for i, strip in enumerate(strips):
        pin, name, leds, desc, feed = strip["pin"], strip["name"], strip["leds"], strip["summary"], strip["feed"]
        y = pin_y0 + i * pin_dy
        svg.text(ard_x1 - 8, y + 4, "D%d" % pin, size=12, anchor="end")
        svg.line(ard_x1, y, res_x, y, BLUE)
        svg.resistor(res_x, y, "R%d  330 ohm" % (i + 1))
        svg.line(res_x + 60, y, blk_x0, y, BLUE)
        svg.circle(blk_x0, y, 3.5, BLUE)

        svg.rect(blk_x0, y - 27, blk_x1 - blk_x0, 54, fill="#fff8e1")
        svg.text(blk_x0 + 12, y - 8, "%s" % name, size=13, weight="bold")
        svg.text(blk_x0 + 12, y + 8, "%d LEDs, %s" % (leds, "WS2812B"), size=11)
        svg.text(blk_x0 + 12, y + 22, desc, size=10, color=GREY)
        svg.text(blk_x0 - 8, y - 7, "DIN", size=10, anchor="end", color=BLUE)
        svg.text(blk_x1 - 4, y - 14, "+5V", size=10, anchor="end", color=RED)
        svg.text(blk_x1 - 4, y + 19, "GND", size=10, anchor="end")
        svg.text(blk_x1 - 4, y + 3, feed, size=9, anchor="end", color=GREY, italic=True)

        # +5 V terminal (top right) to the +5 V bus, GND terminal (bottom right) to the GND bus
        y5, yg = y - 17, y + 17
        svg.line(blk_x1, y5, v5_x, y5, RED)
        svg.dot(v5_x, y5, RED)
        svg.hline_hop(blk_x1, gnd_x, yg, [v5_x], BLACK)
        svg.dot(gnd_x, yg)

    # vertical buses
    svg.line(v5_x, rail5_y, v5_x, last_y - 17, RED, 2.5)
    svg.line(gnd_x, railg_y, gnd_x, last_y + 17, BLACK, 2.5)
    svg.dot(v5_x, rail5_y, RED)
    svg.dot(gnd_x, railg_y)

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
                 ["arrow direction: handle on the GND side", "= LOW = arrow points left"])
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
    svg.rect(ard_x0, ard_y1 + 18, 340, 62, fill="#ffffff", stroke=GREY, dash="5 4")
    svg.text(ard_x0 + 10, ard_y1 + 38, "USB-B cable to a computer or USB charger", size=11)
    svg.text(ard_x0 + 10, ard_y1 + 53, "powers ONLY the Arduino. The LED supply +5 V", size=11)
    svg.text(ard_x0 + 10, ard_y1 + 68, "is NOT connected to the Arduino 5V pin.", size=11)

    # ---- Legend and notes -----------------------------------------------------------------
    ny = ard_y1 + 108
    svg.line(40, ny - 4, 80, ny - 4, RED, 2.5)
    svg.text(88, ny, "+5 V power", size=11)
    svg.line(190, ny - 4, 230, ny - 4, BLACK, 2.5)
    svg.text(238, ny, "GND", size=11)
    svg.line(290, ny - 4, 330, ny - 4, BLUE, 2)
    svg.text(338, ny, "data (via 330 ohm)", size=11)
    svg.line(470, ny - 4, 510, ny - 4, ORANGE, 2)
    svg.text(518, ny, "inputs", size=11)
    svg.line(580, ny - 4, 620, ny - 4, GREEN, 2)
    svg.text(628, ny, "I2C", size=11)
    svg.path("M 680 %g L 690 %g A 6 6 0 0 1 702 %g L 712 %g" % (ny - 4, ny - 4, ny - 4, ny - 4))
    svg.text(720, ny, "hop = lines cross without a connection; dot = connection", size=11)

    # Notes go into the empty space right of the Arduino, below the strip blocks.
    notes = [
        "Total: %d LEDs in %d strips (%d pieces). The positions and LED index ranges are in layout-diagram.svg." % (total_leds, total_strips, total_pieces),
        "1. Common ground: PSU V-, Arduino GND and the GND of every LED strip piece are connected together.",
        "2. Put each 330 ohm resistor close to the first LED of its strip. Keep data wires short.",
        "3. Feed +5 V and GND into every strip piece from the buses (thick wire, at least 1 mm2). Where the diagram says "
        "'start and end', feed the strip at both ends. The DIN end of a strip is the end the arrow printed on the strip points away from.",
        "4. Strips made of several pieces (frame edges, arrowhead pieces, letter groups) are chained: DOUT of one piece to DIN of the next, "
        "and each piece gets its own +5 V and GND connection.",
        "5. The Mega's 5 V logic drives WS2812B directly: no level shifter. Fit F1 right after the supply.",
        "6. SW3 and SW4: pins 1, 2, 4, 8 go to D22-D25 and D26-D29, the common pin C goes to GND. A position counts in binary (0..F). "
        "If your switch gives the complement (15 - position), set SPEED_BIT_IS_LOW to false in sketch.ino.",
        "7. All inputs (SW1 to SW5) use the internal pull-ups (INPUT_PULLUP), so no resistors are needed for them.",
        "8. closedLight (6 white LEDs) sits BEHIND the plate under the CLOSED hole text and is lit only while SW5 says closed.",
        "9. LCD1 (I2C backpack) uses the Mega's I2C pins D20 (SDA) and D21 (SCL) plus the Arduino 5V and GND pins "
        "(about 30 mA with the backlight). Do not connect the Arduino 5V pin to the LED supply.",
    ]
    import textwrap
    note_x, note_y = ard_x1 + 40, last_y + 75
    svg.text(note_x, note_y, "Notes", size=12, weight="bold")
    line_no = 0
    for note in notes:
        for k, line in enumerate(textwrap.wrap(note, 92, subsequent_indent="    ")):
            svg.text(note_x, note_y + 22 + line_no * 16, line, size=11.5)
            line_no += 1
        line_no += 0.4
    assert note_y + 22 + line_no * 16 < ard_y1, "notes do not fit into the free space"

    OUTPUT.write_text(svg.render_a4(landscape=False))
    print("A4 portrait, text height of a 12-unit label: %.2f mm" % (12 * svg.mm_per_unit))
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
