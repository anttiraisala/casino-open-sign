#!/usr/bin/env python3
"""Generate the Wokwi diagram.json for the casino sign.

All LED counts and the sign text are read from the CONFIGURATION section of
../sketch.ino, so the simulated sign always matches the sketch. Run it after
changing any strip length or the text:

    python3 tools/generate_diagram.py

Wokwi has no drawing or background-image part, so the sign is built from the
LED strips themselves (plus a few text labels). Every LED strip part is drawn
at the place it would have on the real sign, with a gap of one LED between
neighbouring physical strips so that each strip can be seen on its own. On the
real sign the strips are mounted tightly together.

Layout (screen coordinates, y grows downwards, 1 LED = PITCH pixels):

  * Frame: a ring of four edges with an empty corner cell between the strips.
    The clockwise strip is the top edge + right edge, the counter-clockwise strip
    the left edge + bottom edge. Both have FRAME_HALF_LEDS LEDs.
  * Letters: "CASINO" above "OPEN". Behind every letter sits a group of single
    LEDs (any number per letter) drawn as a circle, with the letter written in
    the middle as a text label. All LEDs form one chain on one data pin.
  * CLOSED light: the 11th Grove stick (10 white LEDs, shield connector A3) under the OPEN row, behind the plate (the plate
    has a hole text "CLOSED" there).
  * Arrow: 10 Grove RGB LED sticks (10 LEDs each), drawn as 10-pixel strip parts in the real arrow shape
    (tools/arrow_geometry.py): two vertical caps, four rails and four wings at 30 degrees. Wokwi's LEDs are
    bigger than the real sticks' LEDs (16 px per LED against a 7.4 mm pitch), so the arrow is drawn BELOW the
    frame at Wokwi's LED size instead of inside it. On the real sign (50 cm wide) the arrow, 35 cm wide, sits
    inside the frame under the CLOSED text. The sketch lights only the head the arrow points at and only
    the cap at the tail end.
"""

import json
import math
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import arrow_geometry as ag  # the arrow sticks: names, connectors and exact geometry

# ---- Geometry tuning (adjust if parts overlap or leave gaps in Wokwi) ------
PITCH = 16               # pixels per LED; matches pixelSize "3535" (16 px) and 60 LEDs/m (16.7 mm)
THICKNESS = 16           # strip height in pixels
LENGTH_PADDING = 0       # extra pixels the strip part has beyond pixels * PITCH
PIXEL_SIZE = "3535"
GAP = PITCH              # empty space between two separate physical strips
LETTER_SPACING = PITCH   # space between the LED circles of two letters
LED_SPACING = 1.25 * PITCH  # distance between neighbouring LEDs of a letter along the circle
LINE_SPACING = PITCH     # space between the two text lines
LABEL_OFFSET = (-4, -9)  # top-left corner of a letter label relative to the circle centre
MARGIN = 16              # space around the blocks inside the frame
CLOSED_GAP = 12          # space above and below the CLOSED hole text
CLOSED_TEXT_HEIGHT = 24  # height of the CLOSED hole text (about 2.5 cm)
CLOSED_TEXT_WIDTH_LEDS = 6  # width of the CLOSED hole text in LED pitches (about 10 cm)
ARROW_BELOW = 190        # gap between the frame and the arrow drawn below it (room for the mode list)
SIGN_LEFT = 480          # x of the sign's top-left corner
SIGN_TOP = 20            # y of the sign's top-left corner
MEGA_WIDTH = 388         # approximate size of the Arduino Mega part (px)
MEGA_HEIGHT = 192

SKETCH = Path(__file__).resolve().parent.parent / "sketch.ino"
OUTPUT = Path(__file__).resolve().parent.parent / "diagram.json"

def read_config(text):
    """Extract the constants the layout needs from the sketch."""
    config = {}
    for name in ("FRAME_HALF_LEDS", "FRAME_TOP_LEDS", "LEDS_PER_STICK", "STICK_COUNT",
                 "CLOSED_LIGHT_LEDS", "PIN_CASINO_CLOSED_SWITCH",
                 "PIN_FRAME_CW", "PIN_FRAME_CCW", "PIN_LETTERS",
                 "PIN_ARROW_SWITCH", "PIN_MODE_BUTTON", "PIN_STATUS_LED"):
        match = re.search(r"constexpr\s+\w+\s+%s\s*=\s*(\d+)\s*;" % name, text)
        if not match:
            sys.exit("Could not find constant %s in sketch.ino" % name)
        config[name] = int(match.group(1))
    # The CLOSED light is the 11th Grove stick: its pin is a shield connector name such as A3 (Wokwi pin name "A3").
    match = re.search(r"constexpr\s+uint8_t\s+PIN_CLOSED_LIGHT\s*=\s*(A\d+|\d+)\s*;", text)
    if not match:
        sys.exit("Could not find constant PIN_CLOSED_LIGHT in sketch.ino")
    config["PIN_CLOSED_LIGHT"] = match.group(1)
    # Which end of a real stick carries the ball (the black dot of the drawings): true = the DIN end.
    match = re.search(r"constexpr\s+bool\s+STICK_BALL_AT_DIN_END\s*=\s*(true|false)\s*;", text)
    if not match:
        sys.exit("Could not find constant STICK_BALL_AT_DIN_END in sketch.ino")
    config["STICK_BALL_AT_DIN_END"] = match.group(1) == "true"
    # Arrow stick pins: "2, 3, ..., A0, A1, A2" -> Wokwi pin names "2", "3", ..., "A0", "A1", "A2"
    match = re.search(r"constexpr\s+uint8_t\s+STICK_PINS\[[^\]]*\]\s*=\s*\{([^}]*)\}", text)
    if not match:
        sys.exit("Could not find STICK_PINS in sketch.ino")
    config["STICK_PINS"] = [v.strip() for v in match.group(1).split(",") if v.strip()]
    if len(config["STICK_PINS"]) != config["STICK_COUNT"]:
        sys.exit("STICK_PINS has %d entries but STICK_COUNT is %d" % (len(config["STICK_PINS"]), config["STICK_COUNT"]))
    for name in ("LINE_1_TEXT", "LINE_2_TEXT"):
        match = re.search(r'constexpr\s+char\s+%s\[\]\s*=\s*"([^"]*)"\s*;' % name, text)
        if not match:
            sys.exit("Could not find %s in sketch.ino" % name)
        config[name] = match.group(1).upper()
    for name in ("PIN_SPEED_FRAME_ARROW", "PIN_SPEED_LETTERS"):
        match = re.search(r"constexpr\s+uint8_t\s+%s\[\d*\]\s*=\s*\{([^}]*)\}" % name, text)
        if not match:
            sys.exit("Could not find %s in sketch.ino" % name)
        config[name] = [int(v) for v in match.group(1).split(",") if v.strip()]
        if len(config[name]) != 4:
            sys.exit("%s needs exactly 4 pins" % name)
    match = re.search(r"constexpr\s+uint8_t\s+LETTER_LEDS\[\]\s*=\s*\{([^}]*)\}", text)
    if not match:
        sys.exit("Could not find LETTER_LEDS in sketch.ino")
    config["LETTER_LEDS"] = [int(v) for v in match.group(1).split(",") if v.strip()]
    letters = len(config["LINE_1_TEXT"]) + len(config["LINE_2_TEXT"])
    if letters != len(config["LETTER_LEDS"]):
        sys.exit("LETTER_LEDS has %d entries but the text has %d letters"
                 % (len(config["LETTER_LEDS"]), letters))
    return config


class Layout:
    def __init__(self, config):
        self.cfg = config
        self.parts = []        # diagram parts
        self.chains = []       # (strip name, [part ids in chain order], arduino pin)
        self.expected = {}     # strip name -> LED count the sketch expects
        self.warnings = []
        self.build()

    # -- helpers -----------------------------------------------------------
    def add(self, part_id, pixels, center_x, center_y, rotate=0):
        """Add a strip whose centre should be at (center_x, center_y)."""
        length = pixels * PITCH + LENGTH_PADDING
        part = {
            "type": "wokwi-led-strip",
            "id": part_id,
            "top": round(center_y - THICKNESS / 2, 1),
            "left": round(center_x - length / 2, 1),
            "attrs": {"pixels": str(pixels), "pixelShape": "square", "pixelSize": PIXEL_SIZE},
        }
        if rotate:
            part["rotate"] = round(rotate, 2)
        part["_center"] = (center_x, center_y)   # removed before writing
        self.parts.append(part)
        return part_id

    def segment(self, part_id, pixels, a, b):
        """Add a strip along the line from point a to point b (data flows a -> b)."""
        cx, cy = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2
        angle = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
        return self.add(part_id, pixels, cx, cy, rotate=0 if abs(angle) < 1e-6 else angle)

    # -- build -------------------------------------------------------------
    def build(self):
        c = self.cfg
        P = PITCH
        top_leds = c["FRAME_TOP_LEDS"]
        side_leds = c["FRAME_HALF_LEDS"] - top_leds
        if side_leds < 2:
            sys.exit("FRAME_HALF_LEDS must be at least FRAME_TOP_LEDS + 2")
        fx, fy = SIGN_LEFT, SIGN_TOP
        cols, rows = top_leds + 2, side_leds + 2
        self.frame_box = (fx, fy, cols * P, rows * P)

        def cell(col, row):
            return fx + col * P + P / 2, fy + row * P + P / 2

        # Frame ring: corner cells stay empty (the one-LED gap between neighbours).
        x, y = cell(1 + (top_leds - 1) / 2, 0)
        top = self.add("frame_cw_top", top_leds, x, y)
        x, y = cell(cols - 1, 1 + (side_leds - 1) / 2)
        right = self.add("frame_cw_right", side_leds, x, y, rotate=90)
        self.chains.append(("frameCw", [top, right], c["PIN_FRAME_CW"]))
        x, y = cell(0, 1 + (side_leds - 1) / 2)
        left = self.add("frame_ccw_left", side_leds, x, y, rotate=90)
        x, y = cell(1 + (top_leds - 1) / 2, rows - 1)
        bottom = self.add("frame_ccw_bottom", top_leds, x, y)
        self.chains.append(("frameCcw", [left, bottom], c["PIN_FRAME_CCW"]))
        self.expected["frameCw"] = self.expected["frameCcw"] = c["FRAME_HALF_LEDS"]

        centre_x = fx + cols * P / 2
        interior_top = fy + P
        interior_bottom = fy + (rows - 1) * P
        interior_width = top_leds * P

        # Letters: a circle of single LEDs per letter, "CASINO" above "OPEN", centred.
        def group_diameter(n):
            return LED_SPACING if n <= 2 else n * LED_SPACING / math.pi

        letter_leds = c["LETTER_LEDS"]
        row_height = max(group_diameter(n) for n in letter_leds) + P
        line_centre_y = [interior_top + MARGIN + row_height / 2,
                         interior_top + MARGIN + row_height * 1.5 + LINE_SPACING]
        letter_ids, self.letter_labels = [], []
        index = 0
        widest = 0
        for line, text in enumerate((c["LINE_1_TEXT"], c["LINE_2_TEXT"])):
            sizes = [group_diameter(n) + P for n in letter_leds[index:index + len(text)]]
            line_width = sum(sizes) + (len(text) - 1) * LETTER_SPACING
            widest = max(widest, line_width)
            x0 = centre_x - line_width / 2
            for ch, size in zip(text, sizes):
                n = letter_leds[index]
                cx, cy = x0 + size / 2, line_centre_y[line]
                radius = group_diameter(n) / 2
                for k in range(n):
                    if n == 1:
                        px, py = cx, cy
                    elif n == 2:
                        px, py = cx + (k - 0.5) * LED_SPACING, cy
                    else:       # start at the top and go clockwise
                        angle = -math.pi / 2 + 2 * math.pi * k / n
                        px, py = cx + radius * math.cos(angle), cy + radius * math.sin(angle)
                    letter_ids.append(self.add("letter_%d_%s_%d" % (index + 1, ch, k + 1), 1, px, py))
                self.letter_labels.append((ch, cx, cy))
                x0 += size + LETTER_SPACING
                index += 1
        self.chains.append(("letters", letter_ids, c["PIN_LETTERS"]))
        self.expected["letters"] = sum(letter_leds)
        text_bottom = line_centre_y[1] + row_height / 2

        # CLOSED light: one short strip behind the plate under the OPEN row. The plate has the
        # hole text "CLOSED" there (Wokwi cannot show it, so a text label stands in).
        closed_leds = c["CLOSED_LIGHT_LEDS"]
        closed_cy = text_bottom + CLOSED_GAP + CLOSED_TEXT_HEIGHT / 2
        closed_id = self.add("closed_light", closed_leds, centre_x, closed_cy)
        self.chains.append(("closedLight", [closed_id], c["PIN_CLOSED_LIGHT"]))
        self.expected["closedLight"] = closed_leds
        # The hole text is about 10 cm wide (6 LED pitches); the stick behind it has closed_leds LEDs.
        self.closed_box = (centre_x, closed_cy, CLOSED_TEXT_WIDTH_LEDS * P)
        self.closed_strip_width = closed_leds * P
        closed_block = 2 * CLOSED_GAP + CLOSED_TEXT_HEIGHT

        # Arrow: 10 Grove RGB LED sticks in the real arrow shape (geometry from arrow_geometry.py, in mm), drawn
        # below the frame at Wokwi's LED size (see the docstring). Stick n sits on the n-th pin of STICK_PINS.
        geo_sticks, geo_w, geo_h = ag.shifted(ag.build())
        by_geo = {st.name: st for st in geo_sticks}
        if c["LEDS_PER_STICK"] != ag.LEDS or c["STICK_COUNT"] != len(ag.STICK_DEFS):
            sys.exit("The sketch (LEDS_PER_STICK / STICK_COUNT) and tools/arrow_geometry.py disagree")
        k = P / ag.LED_PITCH                          # Wokwi pixels per mm (one LED = one pitch)
        arrow_left = centre_x - geo_w * k / 2
        arrow_top = fy + rows * P + ARROW_BELOW
        self.stick_labels = []
        for idx, definition in enumerate(ag.STICK_DEFS):
            name = definition[0]
            geo = by_geo[name]
            pin = c["STICK_PINS"][idx]
            if pin != geo.connector.lstrip("D"):
                sys.exit("STICK_PINS[%d] is %s but the geometry says %s is on connector %s"
                         % (idx, pin, name, geo.connector))
            sx, sy = arrow_left + geo.cx * k, arrow_top + geo.cy * k
            part_id = "stick_" + name.lower().replace("-", "_")
            # A Wokwi strip part has its first LED (DIN) at the start of the part. With the ball at the DIN end the
            # part runs from the dot (geo.theta); otherwise the real DIN end is the opposite one: turn it by 180
            # degrees (the sketch then counts the LEDs from the other end, so the animation looks the same).
            turn = 0.0 if c["STICK_BALL_AT_DIN_END"] else 180.0
            self.add(part_id, c["LEDS_PER_STICK"], sx, sy, rotate=(geo.theta + turn) % 360.0)
            self.chains.append((name, [part_id], pin))
            self.expected[name] = c["LEDS_PER_STICK"]
            # label: outside the arrow, on the side of the stick that faces away from the arrow's middle
            ux, uy = math.cos(math.radians(geo.theta)), math.sin(math.radians(geo.theta))
            nx, ny = -uy, ux
            mid_x, mid_y = arrow_left + geo_w * k / 2, arrow_top + geo_h * k / 2
            if (sx - mid_x) * nx + (sy - mid_y) * ny < 0:
                nx, ny = -nx, -ny
            text = "%s %s" % (name, geo.connector)
            half_w = 4.0 * len(text)                  # Wokwi text is about 8 px per character
            if name.endswith("CAP"):                  # beside the cap, left or right of it
                lx = sx + nx * (THICKNESS / 2 + 10) + (half_w if nx > 0 else -half_w)
                ly = sy
            elif "WING" in name:                      # beyond the wing, shifted outwards along the text line
                lx = sx + nx * 26 + (half_w if nx > 0 else -half_w)
                ly = sy + ny * 26
            else:                                     # above or below the rail
                lx, ly = sx, sy + ny * 24
            self.stick_labels.append((text, lx, ly, nx))
        self.arrow_extent = (arrow_left, arrow_left + geo_w * k, arrow_top, arrow_top + geo_h * k)
        self.text_extent = (widest, text_bottom)
        self.interior = (interior_top, interior_bottom, interior_width)

        # Consistency check: the chained parts must add up to the sketch's strip lengths.
        sizes = {p["id"]: int(p["attrs"]["pixels"]) for p in self.parts}
        for name, ids, _pin in self.chains:
            total = sum(sizes[i] for i in ids)
            if total != self.expected[name]:
                sys.exit("Internal error: %s has %d LEDs in the diagram but %d in the sketch"
                         % (name, total, self.expected[name]))

    def check_fit(self):
        problems = list(self.warnings)
        interior_top, interior_bottom, interior_width = self.interior
        widest, _ = self.text_extent
        if widest > interior_width - 2 * MARGIN:
            problems.append("The text is wider than the frame interior. Increase FRAME_TOP_LEDS.")
        return problems

    def legend(self):
        """Text labels listing which strip uses which Arduino pin."""
        c = self.cfg
        fx, fy, width, _height = self.frame_box
        lines = [
            "Strips: D<pin> name",
            "D%d frameCw: top edge, then right edge" % c["PIN_FRAME_CW"],
            "D%d frameCcw: left edge, then bottom edge" % c["PIN_FRAME_CCW"],
            "D%d letters: %s, then %s (one LED circle per letter)" % (c["PIN_LETTERS"], c["LINE_1_TEXT"], c["LINE_2_TEXT"]),
            "Arrow sticks in the Grove Base Shield (drawn below the sign):",
            "  D2 L-CAP, D3 L-WING-UP, D4 L-WING-DN, D5 RAIL-UP-L,",
            "  D6 RAIL-UP-R, D7 RAIL-DN-L, D8 RAIL-DN-R,",
            "  A0 R-CAP, A1 R-WING-UP, A2 R-WING-DN",
            "%s closedLight: 11th Grove stick, %d white LEDs behind the CLOSED hole text" % (c["PIN_CLOSED_LIGHT"], c["CLOSED_LIGHT_LEDS"]),
            "D%d: casino closed switch SW5 (handle toward GND = closed)" % c["PIN_CASINO_CLOSED_SWITCH"],
            "D%d-D%d: speed of frame + arrow (hex switch SW3)" % (c["PIN_SPEED_FRAME_ARROW"][0], c["PIN_SPEED_FRAME_ARROW"][3]),
            "D%d-D%d: speed of letters (hex switch SW4)" % (c["PIN_SPEED_LETTERS"][0], c["PIN_SPEED_LETTERS"][3]),
            "D%d (LED L on the board): sign of life; Serial Monitor 115200 baud" % c["PIN_STATUS_LED"],
            "Gaps between strips are drawn only for clarity;",
            "on the real sign the strips touch each other.",
        ]
        return [{"type": "wokwi-text", "id": "legend_%d" % i, "top": fy + i * 22,
                 "left": fx + width + 40, "attrs": {"text": text}} for i, text in enumerate(lines)]

    def diagram(self):
        fx, fy, width, height = self.frame_box
        # The inputs (switches, button, DIP bank) sit in a block ABOVE the Arduino, so the wires
        # from the Arduino to them do not cross the Arduino or the switches themselves.
        # Every caption sits ABOVE its component; the DIP bank is below its caption block.
        label_top = SIGN_TOP                     # first caption line of the switches and button
        inputs_top = label_top + 56              # slide switches and push button
        dip_label_top = inputs_top + 70          # caption block of the DIP bank
        dip_top = dip_label_top + 5 * 20 + 10    # the DIP bank itself
        mega_top = dip_top + 90 + 40
        parts = [{
            "type": "wokwi-arduino-mega",
            "id": "mega",
            "top": mega_top,
            "left": SIGN_LEFT - MEGA_WIDTH - 90,
            "attrs": {},
        }]
        for part in self.parts:
            parts.append({k: v for k, v in part.items() if not k.startswith("_")})
        parts.extend(self.legend())
        # List of the letter modes under the sign (the modes are defined in sketch.ino).
        for i, line in enumerate((
                "Letter modes (button D%d steps through them, after 10 it wraps to 0):" % self.cfg["PIN_MODE_BUTTON"],
                "0 breathe   1 wave   2 typewriter   3 ignite   4 word blink",
                "5 alternate   6 scanner   7 sparkle   8 chase   9 pulse",
                "10 random auto: a random pattern from 0-9, new one every 2 min",
                "The sign starts in mode 0.")):
            parts.append({"type": "wokwi-text", "id": "mode_list_%d" % (i + 1),
                          "top": round(fy + height + 30 + i * 20, 1), "left": round(fx, 1),
                          "attrs": {"text": line}})
        for n, (text, lx, ly, nx) in enumerate(self.stick_labels):
            parts.append({"type": "wokwi-text", "id": "stick_label_%d" % (n + 1),
                          "top": round(ly - 9, 1), "left": round(lx - 4 * len(text), 1),
                          "attrs": {"text": text}})
        ccx, ccy, cwidth = self.closed_box
        parts.append({"type": "wokwi-text", "id": "closed_label", "top": round(ccy - 9, 1),
                      "left": round(ccx + max(cwidth, self.closed_strip_width) / 2 + 10, 1),
                      "attrs": {"text": "<- CLOSED hole text (stick on A3)"}})
        for n, (ch, cx, cy) in enumerate(self.letter_labels):
            parts.append({"type": "wokwi-text", "id": "letter_label_%d" % (n + 1),
                          "top": round(cy + LABEL_OFFSET[1], 1), "left": round(cx + LABEL_OFFSET[0], 1),
                          "attrs": {"text": ch}})

        # Inputs above the Arduino: slide switch (arrow direction) and push button (letter mode).
        # Both use the internal pull-up, so each one only needs a wire to GND.
        # Everything stays left of x = 460 so no label runs into the sign (which starts at SIGN_LEFT).
        parts.append({"type": "wokwi-text", "id": "arrow_switch_label", "top": label_top + 18,
                      "left": 20, "attrs": {"text": "D%d: arrow direction" % self.cfg["PIN_ARROW_SWITCH"]}})
        parts.append({"type": "wokwi-slide-switch", "id": "arrow_switch",
                      "top": inputs_top, "left": 40, "attrs": {}})
        parts.append({"type": "wokwi-text", "id": "mode_button_label", "top": label_top + 18,
                      "left": 170, "attrs": {"text": "D%d: letter mode" % self.cfg["PIN_MODE_BUTTON"]}})
        parts.append({"type": "wokwi-pushbutton", "id": "mode_button",
                      "top": inputs_top, "left": 180, "attrs": {"color": "green"}})
        parts.append({"type": "wokwi-text", "id": "casino_closed_label", "top": label_top,
                      "left": 290, "attrs": {"text": "D%d: casino closed" % self.cfg["PIN_CASINO_CLOSED_SWITCH"]}})
        parts.append({"type": "wokwi-text", "id": "casino_closed_label_2", "top": label_top + 18,
                      "left": 290, "attrs": {"text": "(handle left = closed)"}})
        parts.append({"type": "wokwi-slide-switch", "id": "casino_closed_switch",
                      "top": inputs_top, "left": 330, "attrs": {"value": "1"}})

        # Speed switches. Wokwi has no hex rotary switch, so a DIP switch bank stands in:
        # switches 1-4 are the bits 1, 2, 4, 8 of SW3 (frame + arrow speed), switches 5-8 the
        # bits of SW4 (letters speed). A closed switch pulls its pin to GND = bit set.
        parts.append({"type": "wokwi-dip-switch-8", "id": "speed_switches",
                      "top": dip_top, "left": 80, "attrs": {}})
        for i, line in enumerate((
                "Speed DIP bank (stands in for the hex switches):",
                "1-4 = SW3 bits 1,2,4,8: frame + arrow (D%d-D%d)"
                % (self.cfg["PIN_SPEED_FRAME_ARROW"][0], self.cfg["PIN_SPEED_FRAME_ARROW"][3]),
                "5-8 = SW4 bits 1,2,4,8: letters (D%d-D%d)"
                % (self.cfg["PIN_SPEED_LETTERS"][0], self.cfg["PIN_SPEED_LETTERS"][3]),
                "ON = bit set; click the part, press keys 1-8.",
                "All OFF = position 0 (slowest).")):
            parts.append({"type": "wokwi-text", "id": "speed_label_%d" % (i + 1),
                          "top": dip_label_top + i * 20, "left": 20, "attrs": {"text": line}})

        # I2C LCD 2x16 (PCF8574 backpack, address 0x27) below the Arduino, with its caption above it.
        lcd_top = mega_top + MEGA_HEIGHT + 90
        for i, line in enumerate(("LCD 2x16 (I2C 0x27): SDA = D20, SCL = D21",
                                  "pages: mode, SW3, SW4, casino + arrow")):
            parts.append({"type": "wokwi-text", "id": "lcd_label_%d" % (i + 1),
                          "top": lcd_top - 46 + i * 18, "left": 20, "attrs": {"text": line}})
        parts.append({"type": "wokwi-lcd1602", "id": "lcd", "top": lcd_top, "left": 20,
                      "attrs": {"pins": "i2c"}})

        connections = [
            ["mega:20", "lcd:SDA", "green", []],
            ["mega:21", "lcd:SCL", "green", []],
            ["mega:5V", "lcd:VCC", "red", []],
            ["mega:GND.1", "lcd:GND", "black", []],
            ["mega:%d" % self.cfg["PIN_ARROW_SWITCH"], "arrow_switch:2", "orange", []],
            ["arrow_switch:1", "mega:GND.1", "black", []],
            ["mega:%d" % self.cfg["PIN_CASINO_CLOSED_SWITCH"], "casino_closed_switch:2", "orange", []],
            ["casino_closed_switch:1", "mega:GND.1", "black", []],
            ["mega:%d" % self.cfg["PIN_MODE_BUTTON"], "mode_button:1.r", "orange", []],
            ["mode_button:2.r", "mega:GND.1", "black", []],
        ]
        speed_pins = self.cfg["PIN_SPEED_FRAME_ARROW"] + self.cfg["PIN_SPEED_LETTERS"]
        for k, pin in enumerate(speed_pins, start=1):
            connections.append(["mega:%d" % pin, "speed_switches:%da" % k, "orange", []])
            connections.append(["speed_switches:%db" % k, "mega:GND.1", "black", []])
        for _name, ids, pin in self.chains:   # pin: a number for the strips, "A0".. for the sticks on A0..A2
            first = ids[0]
            connections.append(["mega:%s" % pin, "%s:DIN" % first, "green", []])
            connections.append(["mega:5V", "%s:VDD" % first, "red", []])
            connections.append(["mega:GND.1", "%s:VSS" % first, "black", []])
            for prev, nxt in zip(ids, ids[1:]):
                connections.append(["%s:DOUT" % prev, "%s:DIN" % nxt, "green", []])
                connections.append(["%s:VDD.2" % prev, "%s:VDD" % nxt, "red", []])
                connections.append(["%s:VSS.2" % prev, "%s:VSS" % nxt, "black", []])

        return {
            "version": 1,
            "author": "Casino sign project",
            "editor": "wokwi",
            "parts": parts,
            "connections": connections,
            "dependencies": {},
        }


def main():
    config = read_config(SKETCH.read_text())
    layout = Layout(config)
    for problem in layout.check_fit():
        print("WARNING:", problem)
    OUTPUT.write_text(json.dumps(layout.diagram(), indent=2) + "\n")
    leds = sum(layout.expected.values())
    print("Wrote %s (%d LEDs in %d strips, %d strip parts)"
          % (OUTPUT, leds, len(layout.expected), len(layout.parts)))


if __name__ == "__main__":
    main()
