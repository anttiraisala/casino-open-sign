#!/usr/bin/env python3
"""Generate the Wokwi diagram.json for Casino Open Sign V2 (casino_open_sign_v2/diagram.json).

The pins and the sign text are read from the CONFIGURATION section of ../casino_open_sign_v2/sketch.ino. Run it
after changing a pin or the frame size:

    python3 tools/generate_v2_diagram.py

V2 has two 12 V WS2811 strips (Wokwi has no 12 V part: the strips are simulated as LED strips powered from the Mega
5V pin; one Wokwi LED stands for one pixel = one chip = 3 real LEDs) and 11 Grove sticks:

  * Frame (D11): 18 pixels in a ring. Pixel 1 is the lower-left corner, then counter-clockwise: bottom edge 6 pixels
    (left to right), right edge 3 (going up), top edge 6 (right to left), left edge 3 (going down).
  * Letters (D12): 10 pixels, one per letter. Strip pixels 1-6 are C A S I N O, pixels 7-10 are N E P O: the OPEN
    letters are wired in REVERSED order (O is the last pixel). The drawing shows each pixel behind its letter.
  * CLOSED light: the 11th Grove stick (shield connector A0).
  * Arrow: 10 Grove RGB LED sticks in the real arrow shape (tools/arrow_geometry.py), drawn below the frame at
    Wokwi's LED size.
"""

import json
import math
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import v2_data as v2  # V2 shield connectors come from the sketch
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

SKETCH = Path(__file__).resolve().parent.parent / "casino_open_sign_v2" / "sketch.ino"
OUTPUT = Path(__file__).resolve().parent.parent / "casino_open_sign_v2" / "diagram.json"

def read_config(text):
    """Extract the constants the layout needs from the sketch."""
    config = {}
    for name in ("FRAME_EDGE_LONG", "FRAME_EDGE_SHORT", "LEDS_PER_STICK", "STICK_COUNT",
                 "CLOSED_LIGHT_LEDS", "PIN_CASINO_CLOSED_SWITCH",
                 "PIN_FRAME", "PIN_LETTERS",
                 "PIN_ARROW_SWITCH", "PIN_SKIP_BUTTON", "PIN_STATUS_LED"):
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
    for name in ("PIN_SPEED_FRAME_ARROW", "PIN_SPEED_LETTERS", "PIN_PROGRAM"):
        match = re.search(r"constexpr\s+uint8_t\s+%s\[\d*\]\s*=\s*\{([^}]*)\}" % name, text)
        if not match:
            sys.exit("Could not find %s in sketch.ino" % name)
        config[name] = [int(v) for v in match.group(1).split(",") if v.strip()]
        if len(config[name]) != 4:
            sys.exit("%s needs exactly 4 pins" % name)
    letters = len(config["LINE_1_TEXT"]) + len(config["LINE_2_TEXT"])
    if letters != 10:
        sys.exit("V2 has one pixel per letter on a 10-pixel strip: the text must have 10 letters")
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
        long_n, short_n = c["FRAME_EDGE_LONG"], c["FRAME_EDGE_SHORT"]
        fx, fy = SIGN_LEFT, SIGN_TOP
        FPX, FPY = 80, 64                       # distance between neighbouring frame pixels (x, y) in Wokwi pixels
        # ring cell (col, row): columns 0..long-1, rows 0..short+1 (row 0 = top edge, last row = bottom edge)
        ring_rows = short_n + 2
        frame_w, frame_h = (long_n - 1) * FPX + P, (ring_rows - 1) * FPY + P
        self.frame_box = (fx, fy, frame_w, frame_h)

        def ring(col, row):
            return fx + P / 2 + col * FPX, fy + P / 2 + row * FPY

        # Frame pixels in strip order: pixel 1 lower-left, bottom edge to the right, right edge up, top edge to
        # the left, left edge down.
        frame_ids = []
        cells = []
        for col in range(long_n):
            cells.append((col, ring_rows - 1))                  # bottom edge, left to right
        for row in range(ring_rows - 2, 1 - 1, -1):
            if row >= 1:
                cells.append((long_n - 1, row))                 # right edge, going up
        for col in range(long_n - 1, -1, -1):
            cells.append((col, 0))                              # top edge, right to left
        for row in range(1, ring_rows - 1):
            cells.append((0, row))                              # left edge, going down
        self.frame_pixel_cells = cells
        for n, (col, row) in enumerate(cells, start=1):
            x, y = ring(col, row)
            frame_ids.append(self.add("frame_%02d" % n, 1, x, y))
        self.chains.append(("frame", frame_ids, c["PIN_FRAME"]))
        self.expected["frame"] = len(cells)
        self.frame_labels = [(n, ring(col, row)) for n, (col, row) in enumerate(cells, start=1)]

        centre_x = fx + frame_w / 2
        interior_top = fy + FPY * 0.45
        interior_bottom = fy + frame_h - FPY * 0.3
        interior_width = frame_w - 2 * FPX

        # Letters: ONE pixel per letter. Line 1 (CASINO) is strip pixels 1-6, line 2 (OPEN) is drawn left to right as
        # O P E N but it is wired in REVERSED order: pixel 10 is the O, pixel 7 is the N.
        line1, line2 = c["LINE_1_TEXT"], c["LINE_2_TEXT"]
        letter_step = 38
        self.letter_labels = []
        pix_of = {}
        for i in range(len(line1)):
            pix_of[i] = i + 1
        for j in range(len(line2)):
            pix_of[len(line1) + j] = len(line1) + (len(line2) - j)
        row_y = [fy + FPY * 1.0 + 4, fy + FPY * 1.0 + 4 + 44]
        letter_ids = [None] * (len(line1) + len(line2))
        widest = 0
        index = 0
        for line, text in enumerate((line1, line2)):
            width_line = (len(text) - 1) * letter_step
            widest = max(widest, width_line + P)
            x0 = centre_x - width_line / 2
            for k, ch in enumerate(text):
                cx, cy = x0 + k * letter_step, row_y[line]
                letter_ids[pix_of[index] - 1] = self.add("letter_pixel_%02d_%s" % (pix_of[index], ch), 1, cx, cy)
                self.letter_labels.append((ch, pix_of[index], cx, cy))
                index += 1
        self.chains.append(("letters", letter_ids, c["PIN_LETTERS"]))
        self.expected["letters"] = len(letter_ids)
        text_bottom = row_y[1] + 14

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
        arrow_top = fy + frame_h + ARROW_BELOW
        self.stick_labels = []
        for idx, definition in enumerate(ag.STICK_DEFS):
            name = definition[0]
            geo = by_geo[name]
            pin = c["STICK_PINS"][idx]
            connector = v2.connector_name(pin)    # V2 wiring: taken from the sketch (arrow_geometry keeps the V1 mapping)
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
            text = "%s %s" % (name, connector)
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
        stick_sorted = sorted(v2.stick_info(c), key=lambda s: v2.connector_sort_key(s["connector"]))
        lines = [
            "Casino Open Sign V2: 2 strips + 11 sticks",
            "D%d frame: 18 pixels, pixel 1 lower-left, counter-clockwise (12 V WS2811)" % c["PIN_FRAME"],
            "D%d letters: 10 pixels C A S I N O N E P O (OPEN wired reversed)" % c["PIN_LETTERS"],
            "Simulation only: the strips run from the Mega 5V here; in real life they get 12 V.",
            "Arrow sticks in the Grove Base Shield (drawn below the sign):",
            "  " + ", ".join("%s %s" % (s["connector"], s["name"]) for s in stick_sorted[:4]) + ",",
            "  " + ", ".join("%s %s" % (s["connector"], s["name"]) for s in stick_sorted[4:7]) + ",",
            "  " + ", ".join("%s %s" % (s["connector"], s["name"]) for s in stick_sorted[7:]),
            "%s closedLight: 11th Grove stick, %d white LEDs behind the CLOSED hole text" % (c["PIN_CLOSED_LIGHT"], c["CLOSED_LIGHT_LEDS"]),
            "D%d: casino closed switch SW5 (handle toward GND = closed)" % c["PIN_CASINO_CLOSED_SWITCH"],
            "D%d-D%d: speed of frame + arrow (hex switch SW3)" % (c["PIN_SPEED_FRAME_ARROW"][0], c["PIN_SPEED_FRAME_ARROW"][3]),
            "D%d-D%d: speed of letters (hex switch SW4)" % (c["PIN_SPEED_LETTERS"][0], c["PIN_SPEED_LETTERS"][3]),
            "D%d-D%d: program switch SW6 (hex rotary 0-F): letter program + frame pattern" % (c["PIN_PROGRAM"][0], c["PIN_PROGRAM"][3]),
            "D%d (LED L on the board): sign of life; Serial Monitor 115200 baud" % c["PIN_STATUS_LED"],
            "One Wokwi LED = one real pixel (one chip = 3 LEDs, 10 cm).",
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
        dip_top = dip_label_top + 10 * 20 + 10    # the DIP bank itself
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
                "Program switch SW6 (D%d-D%d) picks the letter program AND the frame pattern:" % (self.cfg["PIN_PROGRAM"][0], self.cfg["PIN_PROGRAM"][3]),
                "0 breathe   1 wave   2 typewriter   3 ignite   4 word blink",
                "5 alternate   6 scanner   7 sparkle   8 chase   9 pulse",
                "A converge   B words swap   C slot machine   D twin chase   E marquee",
                "F random auto: a random program from 0-E, new one every 2 min",
                "Frame patterns 0-E follow the same number. The sign starts in the switch position.")):
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
                      "attrs": {"text": "<- CLOSED hole text (stick on %s)" % v2.connector_name(self.cfg["PIN_CLOSED_LIGHT"])}})
        for n, (ch, pix, cx, cy) in enumerate(self.letter_labels):
            parts.append({"type": "wokwi-text", "id": "letter_label_%d" % (n + 1),
                          "top": round(cy + LABEL_OFFSET[1] - 16, 1), "left": round(cx + LABEL_OFFSET[0], 1),
                          "attrs": {"text": "%s%d" % (ch, pix)}})
        for n, (pix, (px, py)) in enumerate(self.frame_labels):
            parts.append({"type": "wokwi-text", "id": "frame_label_%d" % (n + 1),
                          "top": round(py + 12, 1), "left": round(px - 8, 1), "attrs": {"text": str(pix)}})

        # Inputs above the Arduino: slide switch (arrow direction) and push button (letter mode).
        # Both use the internal pull-up, so each one only needs a wire to GND.
        # Everything stays left of x = 460 so no label runs into the sign (which starts at SIGN_LEFT).
        parts.append({"type": "wokwi-text", "id": "arrow_switch_label", "top": label_top + 18,
                      "left": 20, "attrs": {"text": "D%d: arrow direction" % self.cfg["PIN_ARROW_SWITCH"]}})
        parts.append({"type": "wokwi-slide-switch", "id": "arrow_switch",
                      "top": inputs_top, "left": 40, "attrs": {}})
        parts.append({"type": "wokwi-text", "id": "mode_button_label", "top": label_top + 18,
                      "left": 170, "attrs": {"text": "D%d: skip LED test" % self.cfg["PIN_SKIP_BUTTON"]}})
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
        parts.append({"type": "wokwi-dip-switch-8", "id": "program_switches",
                      "top": dip_top, "left": 290, "attrs": {}})
        for i, line in enumerate((
                "Speed DIP bank (stands in for the hex switches):",
                "1-4 = SW3 bits 1,2,4,8: frame + arrow (D%d-D%d)"
                % (self.cfg["PIN_SPEED_FRAME_ARROW"][0], self.cfg["PIN_SPEED_FRAME_ARROW"][3]),
                "5-8 = SW4 bits 1,2,4,8: letters (D%d-D%d)"
                % (self.cfg["PIN_SPEED_LETTERS"][0], self.cfg["PIN_SPEED_LETTERS"][3]),
                "ON = bit set; click the part, press keys 1-8.",
                "All OFF = position 0 (slowest).",
                "",
                "Second DIP bank (right) = program switch SW6 (D%d-D%d):" % (self.cfg["PIN_PROGRAM"][0], self.cfg["PIN_PROGRAM"][3]),
                "1-4 = bits 1,2,4,8; 5-8 unused. Binary 0..15 = program 0..F.",
                "Click the part, press keys 1-4.")):
            parts.append({"type": "wokwi-text", "id": "speed_label_%d" % (i + 1),
                          "top": dip_label_top + i * 20, "left": 20, "attrs": {"text": line}})

        # I2C LCD 2x16 (PCF8574 backpack, address 0x27) below the Arduino, with its caption above it.
        lcd_top = mega_top + MEGA_HEIGHT + 90
        for i, line in enumerate(("LCD 2x16 (I2C 0x27): SDA = D20, SCL = D21",
                                  "pages: program, SW3, SW4, casino + arrow")):
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
            ["mega:%d" % self.cfg["PIN_SKIP_BUTTON"], "mode_button:1.r", "orange", []],
            ["mode_button:2.r", "mega:GND.1", "black", []],
        ]
        speed_pins = self.cfg["PIN_SPEED_FRAME_ARROW"] + self.cfg["PIN_SPEED_LETTERS"]
        for k, pin in enumerate(speed_pins, start=1):
            connections.append(["mega:%d" % pin, "speed_switches:%da" % k, "orange", []])
            connections.append(["speed_switches:%db" % k, "mega:GND.1", "black", []])
        for k, pin in enumerate(self.cfg["PIN_PROGRAM"], start=1):
            connections.append(["mega:%d" % pin, "program_switches:%da" % k, "orange", []])
            connections.append(["program_switches:%db" % k, "mega:GND.1", "black", []])
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
    print("Wrote %s (%d pixels/LEDs in %d chains, %d strip parts)"
          % (OUTPUT, leds, len(layout.expected), len(layout.parts)))


if __name__ == "__main__":
    main()
