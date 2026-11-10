#!/usr/bin/env python3
"""Draw the layout drawings of Casino Open Sign V2 (casino_open_sign_v2/layout-diagram.svg and layout-parts.svg).

layout-diagram.svg: the plate 60 x 30 cm (front view), the 18 frame pixels, the 10 letter pixels (OPEN reversed), the
CLOSED hole text with its stick, and the arrow of 10 sticks at real size (indicative positions, to scale in mm).
layout-parts.svg: the strips, pixel numbers, letter table, stick table and the cutting list.

    python3 tools/generate_v2_layout.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import arrow_geometry as ag
import v2_data as v2
from generate_schematic import Svg, GREY as GREY_T

OUT = Path(__file__).resolve().parent.parent / "casino_open_sign_v2"
DARK, GREY = "#222222", "#777777"
FRAME_C, LETTER_C, CLOSED_C, ARROW_C, ORANGE = "#1e88e5", "#e53935", "#546e7a", "#2e7d32", "#ef6c00"
PLATE_W, PLATE_H = 600.0, 300.0     # mm
K = 1.6                             # SVG units per mm
STRIP_W = 10.0                      # mm, width of the strip
FONT = "font-family='DejaVu Sans, Arial, sans-serif'"


def frame_pixel_boxes(cfg):
    """[(number, x, y, w, h)] in mm: pixel 1 lower-left, counter-clockwise (indicative: sides are drawn between
    the top and the bottom strip)."""
    long_n, short_n = cfg["FRAME_EDGE_LONG"], cfg["FRAME_EDGE_SHORT"]
    pw = PLATE_W / long_n
    side_h = (PLATE_H - 2 * STRIP_W) / short_n
    boxes = []
    n = 1
    for i in range(long_n):
        boxes.append((n, i * pw, PLATE_H - STRIP_W, pw, STRIP_W)); n += 1            # bottom, left to right
    for i in range(short_n):
        boxes.append((n, PLATE_W - STRIP_W, PLATE_H - STRIP_W - (i + 1) * side_h, STRIP_W, side_h)); n += 1   # right, up
    for i in range(long_n):
        boxes.append((n, PLATE_W - (i + 1) * pw, 0, pw, STRIP_W)); n += 1            # top, right to left
    for i in range(short_n):
        boxes.append((n, 0, STRIP_W + i * side_h, STRIP_W, side_h)); n += 1          # left, going down
    return boxes


def layout_diagram(cfg):
    boxes = frame_pixel_boxes(cfg)
    cor = v2.frame_corners(cfg)
    W, H = 1100, 790
    ox, oy = 70, 118
    svg = Svg(W, H)

    def X(mm):
        return ox + mm * K

    def Y(mm):
        return oy + mm * K

    svg.text(W / 2, 30, "Casino Open Sign V2 - layout (front view, 60 x 30 cm plate, to scale)", size=18, anchor="middle", weight="bold")
    svg.text(W / 2, 50, "2 strips (12 V WS2811, 1 pixel = 1 chip = 3 LEDs = 10 cm): frame 18 pixels on D%d, letters 10 pixels on D%d; + 11 Grove sticks (10 arrow + CLOSED)."
             % (cfg["PIN_FRAME"], cfg["PIN_LETTERS"]), size=12, anchor="middle", color=GREY)
    svg.text(W / 2, 68, "Positions are indicative (letter and arrow positions are your choice); the pixel numbers and the wiring order are exact.",
             size=12, anchor="middle", color=GREY)
    svg.rect(ox, oy, PLATE_W * K, PLATE_H * K, fill="#f6f6f6", stroke="#444444", width=2.5)
    # dimensions
    svg.line(ox, oy - 22, ox + PLATE_W * K, oy - 22, DARK, 1.5)
    svg.line(ox, oy - 28, ox, oy - 16, DARK, 1.5)
    svg.line(ox + PLATE_W * K, oy - 28, ox + PLATE_W * K, oy - 16, DARK, 1.5)
    svg.text(ox + PLATE_W * K / 2, oy - 28, "60 cm", size=14, anchor="middle", weight="bold")
    svg.line(ox - 24, oy, ox - 24, oy + PLATE_H * K, DARK, 1.5)
    svg.line(ox - 30, oy, ox - 18, oy, DARK, 1.5)
    svg.line(ox - 30, oy + PLATE_H * K, ox - 18, oy + PLATE_H * K, DARK, 1.5)
    svg.add("<text x='%g' y='%g' font-size='14' font-weight='bold' text-anchor='middle' fill='%s' transform='rotate(-90 %g %g)' %s>30 cm</text>"
            % (ox - 32, oy + PLATE_H * K / 2, DARK, ox - 32, oy + PLATE_H * K / 2, FONT))

    # frame pixels
    centres = {}
    for n, x, y, w, h in boxes:
        svg.rect(X(x) + 1, Y(y) + 1, w * K - 2, h * K - 2, fill="#bbdefb", stroke=FRAME_C, width=1.6, rx=2)
        centres[n] = (X(x + w / 2), Y(y + h / 2))
    pts = [centres[n] for n in sorted(centres)]
    edges = [(1, cfg["FRAME_EDGE_LONG"]), (cfg["FRAME_EDGE_LONG"] + 1, cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"]),
             (cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"] + 1, 2 * cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"]),
             (2 * cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"] + 1, 2 * (cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"]))]
    for a, b in edges:       # data direction inside every edge (the strip is one piece; it bends round the corners)
        svg.path("M " + " L ".join("%g %g" % centres[n] for n in range(a, b + 1)), "#0d47a1", 1.2)
    svg.dot(pts[0][0], pts[0][1], DARK, 4)
    for n, (cx, cy) in centres.items():
        # number just inside the frame
        bx, by = cx, cy
        if n <= cfg["FRAME_EDGE_LONG"]:
            by = cy - 16
        elif n <= cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"]:
            bx = cx - 16
        elif n <= 2 * cfg["FRAME_EDGE_LONG"] + cfg["FRAME_EDGE_SHORT"]:
            by = cy + 16
        else:
            bx = cx + 16
        svg.rect(bx - 10, by - 8, 20, 16, fill="#ffffff", stroke=FRAME_C, width=1.2, rx=3)
        svg.text(bx, by + 4, str(n), size=11, anchor="middle", weight="bold", color=FRAME_C)

    # letters: one pixel behind each letter, OPEN reversed on the strip
    t1, t2 = cfg["LINE_1_TEXT"], cfg["LINE_2_TEXT"]
    pix = v2.letter_pixels(cfg)
    step = 60.0
    row_y = (42.0, 97.0)
    letter_centres = {}
    for line, text in enumerate((t1, t2)):
        x0 = PLATE_W / 2 - (len(text) - 1) * step / 2
        for k, ch in enumerate(text):
            idx = (0 if line == 0 else len(t1)) + k
            number = pix[idx][1]
            cx, cy = x0 + k * step, row_y[line]
            svg.rect(X(cx) - 26, Y(cy) - 28, 52, 56, fill="#ffffff", stroke=LETTER_C, width=1.6, rx=4, dash="4 3")
            svg.text(X(cx), Y(cy) + 12, ch, size=34, anchor="middle", weight="bold", color="#ef9a9a")
            svg.rect(X(cx) - 19, Y(cy) - 25, 38, 14, fill="#ffcdd2", stroke=LETTER_C, width=1.2, rx=3)
            svg.text(X(cx), Y(cy) - 14, "px %d" % number, size=10, anchor="middle", weight="bold", color=LETTER_C)
            letter_centres[number] = (X(cx), Y(cy) - 18)
    ordered = [letter_centres[n] for n in sorted(letter_centres)]
    for a, b in zip(ordered, ordered[1:]):
        svg.line(a[0], a[1] + 4, b[0], b[1] + 4, GREY, 1.2, dash="4 3")
    first_open = letter_centres[len(t1) + len(t2)]
    svg.text(X(PLATE_W / 2), Y(row_y[1]) + 52, "strip pixels 7-10 run right to left: OPEN is wired REVERSED (the program maps the letters)",
             size=11, anchor="middle", color=LETTER_C)

    # CLOSED hole text and its stick behind the plate
    ccx, ccy = PLATE_W / 2, 142.0
    svg.add("<text x='%g' y='%g' font-size='36' font-weight='bold' text-anchor='middle' fill='none' stroke='#444444' stroke-width='1.6' "
            "stroke-dasharray='5 3' textLength='%g' lengthAdjust='spacingAndGlyphs' %s>CLOSED</text>"
            % (X(ccx), Y(ccy) + 13, 100 * K, FONT))
    half_l, half_w = ag.BODY_L / 2 * K, ag.BODY_W / 2 * K
    svg.add("<rect x='%g' y='%g' width='%g' height='%g' fill='%s' fill-opacity='0.25' stroke='%s' stroke-width='1.5' stroke-dasharray='5 3'/>"
            % (X(ccx) - half_l, Y(ccy) - half_w, 2 * half_l, 2 * half_w, CLOSED_C, CLOSED_C))
    wl, ww = ag.LED_L / 2 * K, ag.LED_W / 2 * K
    svg.add("<rect x='%g' y='%g' width='%g' height='%g' fill='#ffffff' stroke='%s' stroke-width='0.8'/>"
            % (X(ccx) - wl, Y(ccy) - ww, 2 * wl, 2 * ww, CLOSED_C))
    for k in range(1, ag.LEDS + 1):
        svg.dot(X(ccx) - wl + (k - 0.5) * ag.LED_PITCH * K, Y(ccy), CLOSED_C, 1.6)
    svg.rect(X(ccx) - 8, Y(ccy) - 7, 16, 14, fill="#ffffff", stroke=CLOSED_C, width=1.2, rx=3)
    svg.text(X(ccx), Y(ccy) + 4, "11", size=10, anchor="middle", weight="bold", color=DARK)

    # arrow sticks at real size
    geo, gw, gh = ag.shifted(ag.build())
    left, top = (PLATE_W - gw) / 2, 168.0

    def svg_pt(mx, my):
        return X(left + mx), Y(top + my)

    for number, st in enumerate(geo, start=1):
        body = [svg_pt(*p) for p in st.body()]
        svg.path("M " + " L ".join("%g %g" % p for p in body) + " Z", st.stroke, 1.5, fill=st.fill)
        win = [svg_pt(*p) for p in st.window()]
        svg.path("M " + " L ".join("%g %g" % p for p in win) + " Z", st.stroke, 0.8, fill="#ffffff")
        for k in range(1, ag.LEDS + 1):
            lx, ly = svg_pt(*st.led_centre(k))
            svg.dot(lx, ly, st.stroke, 1.6)
        bx, by = svg_pt(st.cx + st.din_dir[0] * (ag.BODY_L / 2 - 2.5), st.cy + st.din_dir[1] * (ag.BODY_L / 2 - 2.5))
        svg.dot(bx, by, DARK, 3)
        mx, my = svg_pt(st.cx, st.cy)
        svg.rect(mx - 8, my - 7, 16, 14, fill="#ffffff", stroke=st.stroke, width=1.2, rx=3)
        svg.text(mx, my + 4, str(number), size=10, anchor="middle", weight="bold", color=DARK)
    svg.text(X(left + gw / 2), Y(top + gh) + 14, "arrow %.1f x %.1f cm (sticks at real size, numbers 1-10 = arrow sticks)" % (gw / 10, gh / 10),
             size=11, anchor="middle", weight="bold")

    # corner call-outs for the frame animation
    ly = oy + PLATE_H * K + 34
    svg.text(ox, ly, "Frame animation (a bar of 2 lit pixels, 2 dark, repeating, runs both ways round the frame, 9 steps, meets at the end corner):",
             size=12, weight="bold")
    svg.text(ox, ly + 20, "arrow points LEFT (customer's left): starts at the UPPER-RIGHT corner (pixel %d), runs clockwise (%d, 9, 8, 7, %d ...) and counter-clockwise (%d, %d ... %d, 16-18)"
             % (cor["upper_right"], cor["upper_right"], cor["lower_right"], cor["upper_right"], cor["upper_right"] + 1, cor["upper_left"]),
             size=11)
    svg.text(ox, ly + 36, "to the LOWER-LEFT corner (pixel %d)." % cor["lower_left"], size=11)
    svg.text(ox, ly + 56, "arrow points RIGHT (customer's right): starts at the UPPER-LEFT corner (pixel %d), runs both ways to the LOWER-RIGHT corner (pixel %d)."
             % (cor["upper_left"], cor["lower_right"]), size=11)
    svg.text(ox, ly + 80, "Strip: pixel 1 in the lower-left corner, counter-clockwise (dot = DIN end). Dashed grey line in the letters = data jumper in strip order. Front view.",
             size=11, color=GREY)
    svg.text(ox, ly + 96, "Letters: CASINO = pixels 1-6, OPEN = pixels 10, 9, 8, 7 (O P E N). Dashed boxes: the plexiglass letters; the pixel sits behind each letter.",
             size=11, color=GREY)
    (OUT / "layout-diagram.svg").write_text(svg.render_a4(landscape=True))


def layout_parts(cfg):
    strips = v2.strip_info(cfg)
    sticks = v2.all_stick_info(cfg)
    cor = v2.frame_corners(cfg)
    W = 760
    svg = Svg(W, 1090)
    svg.text(W / 2, 34, "Casino Open Sign V2 - parts: strips, pixel numbers and cutting list", size=18, anchor="middle", weight="bold")
    total = sum(s["leds"] for s in strips)
    svg.text(W / 2, 56, "2 strips, %d pixels = %.1f m of 12 V WS2811 strip (30 LEDs per metre, 1 pixel = 10 cm); 11 Grove sticks of %d LEDs."
             % (total, total / 10.0, cfg["LEDS_PER_STICK"]), size=12, anchor="middle", color=GREY)
    svg.text(W / 2, 76, "Cut the strip only at the cut marks (between two pixels). Both strips are single pieces: no joints.", size=12, anchor="middle", color=GREY)
    y = 120
    svg.text(40, y - 12, "Strips", size=15, weight="bold")
    cols = [40, 150, 205, 260, 330]
    svg.rect(30, y, W - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    for x, h in zip(cols, ("Strip", "Pin", "Pixels", "LEDs", "Length / contents")):
        svg.text(x, y + 17, h, size=12, weight="bold")
    y += 24
    for i, s in enumerate(strips):
        svg.rect(30, y, W - 60, 40, fill="#ffffff" if i % 2 == 0 else "#fafafa", stroke="#dddddd", width=1)
        svg.text(cols[0], y + 16, s["name"], size=12, weight="bold", color=FRAME_C if s["name"] == "frame" else LETTER_C)
        svg.text(cols[1], y + 16, "D%d" % s["pin"], size=12)
        svg.text(cols[2], y + 16, "%d" % s["leds"], size=12, weight="bold")
        svg.text(cols[3], y + 16, "%d" % (3 * s["leds"]), size=12)
        svg.text(cols[4], y + 16, "%d cm, one piece" % (10 * s["leds"]), size=12)
        svg.text(cols[4], y + 32, s["summary"], size=11, color=GREY)
        y += 40
    svg.rect(30, y, W - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    svg.text(40, y + 17, "Total", size=12, weight="bold")
    svg.text(cols[2], y + 17, "%d" % total, size=12, weight="bold")
    svg.text(cols[3], y + 17, "%d" % (3 * total), size=12)
    svg.text(cols[4], y + 17, "%.1f m of one 5 m reel (50 pixels): %.1f m left over" % (total / 10.0, 5 - total / 10.0), size=12)
    y += 70
    svg.text(40, y - 12, "Frame strip (D%d): pixel numbers (front view, counter-clockwise)" % cfg["PIN_FRAME"], size=15, weight="bold")
    l, s = cfg["FRAME_EDGE_LONG"], cfg["FRAME_EDGE_SHORT"]
    rows = [("1 - %d" % l, "bottom edge, left to right", "pixel 1 = lower-left corner, pixel %d = lower-right corner" % cor["lower_right"]),
            ("%d - %d" % (l + 1, l + s), "right edge, going up", ""),
            ("%d - %d" % (l + s + 1, 2 * l + s), "top edge, right to left", "pixel %d = upper-right corner, pixel %d = upper-left corner" % (cor["upper_right"], cor["upper_left"])),
            ("%d - %d" % (2 * l + s + 1, 2 * (l + s)), "left edge, going down", "the strip ends here (DOUT end)")]
    svg.rect(30, y, W - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    for x, h in zip((40, 130, 360), ("Pixels", "Position", "Corners")):
        svg.text(x, y + 17, h, size=12, weight="bold")
    for i, (a, b, c) in enumerate(rows):
        yy = y + 24 + i * 22
        svg.rect(30, yy, W - 60, 22, fill="#ffffff" if i % 2 == 0 else "#fafafa", stroke="#dddddd", width=1)
        svg.text(40, yy + 15, a, size=12, weight="bold", color=FRAME_C)
        svg.text(130, yy + 15, b, size=12)
        svg.text(360, yy + 15, c, size=11, color=GREY)
    y += 24 + len(rows) * 22 + 20
    svg.text(40, y, "Frame light, arrow right: start pixel %d (upper left), end pixel %d (lower right)." % (cor["upper_left"], cor["lower_right"]), size=11, color=GREY)
    svg.text(40, y + 16, "Frame light, arrow left: start pixel %d (upper right), end pixel %d (lower left)." % (cor["upper_right"], cor["lower_left"]), size=11, color=GREY)
    y += 56
    svg.text(40, y - 12, "Letters strip (D%d): one pixel per letter" % cfg["PIN_LETTERS"], size=15, weight="bold")
    pix = v2.letter_pixels(cfg)
    svg.rect(30, y, W - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    svg.rect(30, y + 24, W - 60, 24, fill="#ffffff", stroke="#dddddd", width=1)
    cell = (W - 60) / len(pix)
    for i, (ch, n) in enumerate(pix):
        x = 30 + cell * i + cell / 2
        svg.text(x, y + 17, "%s (%s)" % (ch, "line 1" if i < len(cfg["LINE_1_TEXT"]) else "line 2"), size=11, anchor="middle", weight="bold", color=LETTER_C)
        svg.text(x, y + 41, "pixel %d" % n, size=12, anchor="middle")
        if i:
            svg.line(30 + cell * i, y, 30 + cell * i, y + 48, "#dddddd", 1)
    svg.text(40, y + 70, "The strip order is C A S I N O N E P O: the OPEN letters are wired in REVERSED order (O is the last pixel).", size=11, color=GREY)
    svg.text(40, y + 86, "The program maps every letter to its pixel, so all patterns run in reading order for the viewer.", size=11, color=GREY)
    y += 136
    svg.text(40, y - 12, "Grove sticks: 10 arrow sticks + the CLOSED light (in the shield, not cut)", size=15, weight="bold")
    scols = [40, 190, 280, 340]
    svg.rect(30, y, W - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    for x, h in zip(scols, ("Stick", "Connector", "LEDs", "Position")):
        svg.text(x, y + 17, h, size=12, weight="bold")
    for i, st in enumerate(sticks):
        yy = y + 24 + i * 20
        svg.rect(30, yy, W - 60, 20, fill="#ffffff" if i % 2 == 0 else "#fafafa", stroke="#dddddd", width=1)
        svg.text(scols[0], yy + 14, "%d  %s" % (i + 1, st["name"]), size=12, weight="bold", color=ARROW_C)
        svg.text(scols[1], yy + 14, st["connector"], size=12)
        svg.text(scols[2], yy + 14, "%d" % st["leds"], size=12)
        svg.text(scols[3], yy + 14, st["desc"], size=12)
    svg.text(40, y + 24 + len(sticks) * 20 + 18, "Stick body 83 x 10 mm, LED window 74 x 4 mm; arrow geometry and drill template: ../arrow_sticks/.", size=11, color=GREY)
    (OUT / "layout-parts.svg").write_text(svg.render_a4(landscape=False))


def main():
    cfg = v2.config()
    layout_diagram(cfg)
    layout_parts(cfg)
    print("Wrote %s/layout-diagram.svg and layout-parts.svg" % OUT)


if __name__ == "__main__":
    main()
