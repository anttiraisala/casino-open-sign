"""Shared description of the two strips of Casino Open Sign V2 (read from casino_open_sign_v2/sketch.ino).

Used by generate_v2_schematic.py, generate_v2_layout.py and generate_v2_parts_list.py so that all drawings show the
same pins and pixel numbers. One pixel = one WS2811 chip = 3 LEDs of the 12 V strip (10 cm).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_v2_diagram as gd  # read_config() and the path of the V2 sketch


def config():
    return gd.read_config(gd.SKETCH.read_text())


def strip_info(cfg):
    """The two strips (frame, letters) sorted by pin. Keys: pin, name, desc, feed, leds (pixels), summary."""
    long_n, short_n = cfg["FRAME_EDGE_LONG"], cfg["FRAME_EDGE_SHORT"]
    frame_n = 2 * (long_n + short_n)
    a, b, c = long_n, long_n + short_n, 2 * long_n + short_n
    frame = dict(pin=cfg["PIN_FRAME"], name="frame", desc="frame, one strip around the plate", feed="12 V: start + end",
                 leds=frame_n,
                 summary="1-%d bottom %d-%d right %d-%d top %d-%d left" % (a, a + 1, b, b + 1, c, c + 1, frame_n))
    text1, text2 = cfg["LINE_1_TEXT"], cfg["LINE_2_TEXT"]
    order = list(text1) + list(reversed(text2))
    letters = dict(pin=cfg["PIN_LETTERS"], name="letters", desc="one pixel per letter", feed="12 V: start",
                   leds=len(order), summary="1-%d %s, %d-%d %s (reversed)" % (
                       len(text1), " ".join(text1), len(text1) + 1, len(order), " ".join(order[len(text1):])))
    return sorted([frame, letters], key=lambda r: r["pin"])


def connector_name(pin):
    """Shield connector name of a sketch pin value: 3 -> D3, A3 -> A3."""
    pin = str(pin)
    return "D" + pin if pin.isdigit() else pin


def connector_sort_key(connector):
    """D2..D8 first, then A0..A3."""
    return (connector[0] == "A", int(connector[1:]))


def stick_info(cfg):
    import arrow_geometry as ag
    out = []
    for definition, pin in zip(ag.STICK_DEFS, cfg["STICK_PINS"]):
        name, desc = definition[0], definition[1]
        # V2 wiring: the shield connector comes from the V2 sketch, NOT from arrow_geometry.py (that keeps the V1 mapping)
        connector = connector_name(pin)
        out.append(dict(name=name, desc=desc, connector=connector, pin=pin, leds=cfg["LEDS_PER_STICK"]))
    return out


def closed_stick_info(cfg):
    return dict(name="CLOSED", desc="CLOSED light, behind the plate", connector=connector_name(cfg["PIN_CLOSED_LIGHT"]),
                pin=cfg["PIN_CLOSED_LIGHT"], leds=cfg["CLOSED_LIGHT_LEDS"])


def all_stick_info(cfg):
    return stick_info(cfg) + [closed_stick_info(cfg)]


def letter_pixels(cfg):
    """[(letter, strip pixel number 1-based)] in reading order: C A S I N O O P E N -> 1..6, 10, 9, 8, 7."""
    t1, t2 = cfg["LINE_1_TEXT"], cfg["LINE_2_TEXT"]
    out = [(ch, i + 1) for i, ch in enumerate(t1)]
    out += [(ch, len(t1) + len(t2) - j) for j, ch in enumerate(t2)]
    return out


def frame_corners(cfg):
    """1-based pixel numbers of the corners: lower-left, lower-right, upper-right, upper-left."""
    l, s = cfg["FRAME_EDGE_LONG"], cfg["FRAME_EDGE_SHORT"]
    return dict(lower_left=1, lower_right=l, upper_right=l + s + 1, upper_left=2 * l + s)
