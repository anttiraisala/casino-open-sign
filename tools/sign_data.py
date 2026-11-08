"""Shared description of the LED strips of the casino sign.

Used by generate_schematic.py and generate_layout.py so that both drawings show
exactly the same pins, LED counts and pieces. All numbers come from the
CONFIGURATION section of ../sketch.ino (read with generate_diagram.read_config).
"""


def strip_info(cfg):
    """Return one dict per strip, sorted by Arduino pin.

    Keys: pin, name, desc, feed, pieces [(label, LEDs)], leds, summary.
    The pieces are listed in LED index order (the order the data flows).
    """
    texts = cfg["LINE_1_TEXT"], cfg["LINE_2_TEXT"]
    letters = list(texts[0] + texts[1])
    counts = cfg["LETTER_LEDS"]
    top = cfg["FRAME_TOP_LEDS"]
    side = cfg["FRAME_HALF_LEDS"] - top
    first = len(texts[0])
    letter_summary = "groups: %s | %s" % (" ".join(str(n) for n in counts[:first]),
                                          " ".join(str(n) for n in counts[first:]))

    rows = [
        dict(pin=cfg["PIN_FRAME_CW"], name="frameCw", desc="frame, clockwise half",
             feed="feed: start and end", pieces=[("top edge", top), ("right edge", side)],
             summary="top edge %d + right edge %d" % (top, side)),
        dict(pin=cfg["PIN_FRAME_CCW"], name="frameCcw", desc="frame, counter-clockwise half",
             feed="feed: start and end", pieces=[("left edge", side), ("bottom edge", top)],
             summary="left edge %d + bottom edge %d" % (side, top)),
        dict(pin=cfg["PIN_LETTERS"], name="letters", desc="%s, then %s" % texts,
             feed="feed: every group", pieces=[(ch, n) for ch, n in zip(letters, counts)],
             summary=letter_summary),
    ]
    for row in rows:
        row["leds"] = sum(n for _, n in row["pieces"])
    return sorted(rows, key=lambda r: r["pin"])


def stick_info(cfg):
    """One dict per arrow stick (Grove RGB LED stick), in the order of STICK_PINS: name, desc, connector, leds.

    The names, descriptions and shield connectors come from tools/arrow_geometry.py (the single source of the
    arrow); the sketch must agree with it (checked here)."""
    import arrow_geometry as ag
    out = []
    for definition, pin in zip(ag.STICK_DEFS, cfg["STICK_PINS"]):
        name, desc, _colours, connector = definition
        if connector.lstrip("D") != pin:
            raise SystemExit("STICK_PINS in sketch.ino disagrees with arrow_geometry.py for %s" % name)
        out.append(dict(name=name, desc=desc, connector=connector, pin=pin, leds=cfg["LEDS_PER_STICK"]))
    return out


def closed_stick_info(cfg):
    """The 11th Grove stick: the white CLOSED light behind the plate (shield connector A3). Not part of the arrow."""
    return dict(name="CLOSED", desc="CLOSED light, behind the plate", connector=cfg["PIN_CLOSED_LIGHT"],
                pin=cfg["PIN_CLOSED_LIGHT"], leds=cfg["CLOSED_LIGHT_LEDS"])


def all_stick_info(cfg):
    """The 10 arrow sticks followed by the CLOSED light stick (the 11 sticks of the Grove Base Shield)."""
    return stick_info(cfg) + [closed_stick_info(cfg)]


def with_ranges(pieces):
    """[(label, LEDs)] -> [(label, LEDs, first index, last index)]."""
    out, start = [], 0
    for label, n in pieces:
        out.append((label, n, start, start + n - 1))
        start += n
    return out


def totals(rows):
    """(strips, pieces, LEDs) of the whole sign."""
    return len(rows), sum(len(r["pieces"]) for r in rows), sum(r["leds"] for r in rows)
