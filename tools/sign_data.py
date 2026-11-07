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
    cap = cfg["ARROW_CAP_LEDS"]
    body = cfg["ARROW_BODY_LEDS"]
    base, wing = cfg["ARROW_HEAD_BASE_LEDS"], cfg["ARROW_HEAD_WING_LEDS"]

    first = len(texts[0])
    letter_summary = "groups: %s | %s" % (" ".join(str(n) for n in counts[:first]),
                                          " ".join(str(n) for n in counts[first:]))

    head_pieces = [("upper base piece", base), ("upper wing", wing), ("lower wing", wing), ("lower base piece", base)]
    head_summary = "pieces " + " + ".join(str(n) for _, n in head_pieces)

    rows = [
        dict(pin=cfg["PIN_CLOSED_LIGHT"], name="closedLight", desc="white light behind the CLOSED hole text",
             feed="feed: start", pieces=[("white LED piece", cfg["CLOSED_LIGHT_LEDS"])],
             summary="behind the plate, always white"),
        dict(pin=cfg["PIN_ARROW_CAP_LEFT"], name="arrowCapLeft", desc="arrow, left end cap",
             feed="feed: start", pieces=[("end cap", cap)], summary="arrow, left end cap"),
        dict(pin=cfg["PIN_ARROW_CAP_RIGHT"], name="arrowCapRight", desc="arrow, right end cap",
             feed="feed: start", pieces=[("end cap", cap)], summary="arrow, right end cap"),
        dict(pin=cfg["PIN_FRAME_CW"], name="frameCw", desc="frame, clockwise half",
             feed="feed: start and end", pieces=[("top edge", top), ("right edge", side)],
             summary="top edge %d + right edge %d" % (top, side)),
        dict(pin=cfg["PIN_FRAME_CCW"], name="frameCcw", desc="frame, counter-clockwise half",
             feed="feed: start and end", pieces=[("left edge", side), ("bottom edge", top)],
             summary="left edge %d + bottom edge %d" % (side, top)),
        dict(pin=cfg["PIN_LETTERS"], name="letters", desc="%s, then %s" % texts,
             feed="feed: every group", pieces=[(ch, n) for ch, n in zip(letters, counts)],
             summary=letter_summary),
        dict(pin=cfg["PIN_ARROW_BODY_TOP"], name="arrowBodyTop", desc="arrow, shaft top edge",
             feed="feed: start", pieces=[("shaft top edge", body)], summary="arrow, shaft top edge"),
        dict(pin=cfg["PIN_ARROW_BODY_BOTTOM"], name="arrowBodyBottom", desc="arrow, shaft bottom edge",
             feed="feed: start", pieces=[("shaft bottom edge", body)], summary="arrow, shaft bottom edge"),
        dict(pin=cfg["PIN_ARROW_HEAD_LEFT"], name="arrowHeadLeft", desc="arrow, left arrowhead",
             feed="feed: start and end", pieces=list(head_pieces), summary=head_summary),
        dict(pin=cfg["PIN_ARROW_HEAD_RIGHT"], name="arrowHeadRight", desc="arrow, right arrowhead",
             feed="feed: start and end", pieces=list(head_pieces), summary=head_summary),
    ]
    for row in rows:
        row["leds"] = sum(n for _, n in row["pieces"])
    return sorted(rows, key=lambda r: r["pin"])


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
