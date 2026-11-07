#!/usr/bin/env python3
"""Draw the layout diagram of the casino sign as an SVG file.

The drawing shows where every LED strip goes on the plate, to scale, in its real
shape, with its name, Arduino pin, LED count and the LED index ranges, plus the
main dimensions in centimetres. It is drawn from the same geometry as the Wokwi
diagram (tools/generate_diagram.py) and from the settings in ../sketch.ino:

    python3 tools/generate_layout.py [--leds-per-m 60]

Output: layout-diagram.svg in the project folder.

On the real sign the strips touch each other; the drawing keeps the one-LED gaps of
the Wokwi layout so that every strip piece can be seen.
"""

import argparse
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_diagram as gd
import sign_data
from generate_schematic import Svg

OUTPUT = Path(__file__).resolve().parent.parent / "layout-diagram.svg"
OUTPUT_PARTS = Path(__file__).resolve().parent.parent / "layout-parts.svg"

S = 1.5            # SVG units per diagram pixel
LED = 17           # side of one LED square in SVG units
GREY, DARK = "#777777", "#222222"
COLORS = {
    "frameCw": "#1e88e5", "frameCcw": "#8e24aa", "letters": "#e53935",
    "arrowBodyTop": "#43a047", "arrowBodyBottom": "#00897b",
    "arrowCapLeft": "#fb8c00", "arrowCapRight": "#d84315",
    "arrowHeadLeft": "#3949ab", "arrowHeadRight": "#6d4c41",
    "closedLight": "#546e7a",
}
PANEL_W = 285      # width of the callout panels left and right of the plate


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--leds-per-m", type=float, default=60, help="LED density of the strip (default 60)")
    args = parser.parse_args()
    cm_per_px = (100.0 / args.leds_per_m) / gd.PITCH     # one pitch = 100 / LEDs-per-metre cm

    cfg = gd.read_config(gd.SKETCH.read_text())
    lay = gd.Layout(cfg)
    info = sign_data.strip_info(cfg)
    by_name = {row["name"]: row for row in info}
    total_strips, total_pieces, total_leds = sign_data.totals(info)

    fx, fy, fw, fh = lay.frame_box
    ox, oy = PANEL_W + 60, 95                    # top-left corner of the plate in SVG units
    plate_w, plate_h = fw * S, fh * S
    width = int(PANEL_W * 2 + 120 + plate_w)

    def X(px):
        return ox + (px - fx) * S

    def Y(py):
        return oy + (py - fy) * S

    parts = {p["id"]: p for p in lay.parts}

    def led_centres(part):
        k = int(part["attrs"]["pixels"])
        cx, cy = part["_center"]
        a = math.radians(part.get("rotate", 0))
        dx, dy = math.cos(a), math.sin(a)
        return [(X(cx + (i - (k - 1) / 2) * gd.PITCH * dx), Y(cy + (i - (k - 1) / 2) * gd.PITCH * dy))
                for i in range(k)], (dx, dy)

    # Plate content height (the tables go below)
    n_rows = len(info)
    height = int(oy + plate_h + 38 + 52 + 40)       # plate, dimensions and the notes under it
    svg = Svg(width, height)

    svg.text(width / 2, 30, "Casino sign - layout of the LED strips (to scale, dimensions in cm)",
             size=18, anchor="middle", weight="bold")
    svg.text(width / 2, 50, "%d strips, %d pieces, %d LEDs in total. LED pitch %.1f mm (%g LEDs per metre)."
             % (total_strips, total_pieces, total_leds, cm_per_px * gd.PITCH * 10, args.leds_per_m),
             size=12, anchor="middle", color=GREY)

    # ---- plate and dimensions -----------------------------------------------------------
    svg.rect(ox, oy, plate_w, plate_h, fill="#f6f6f6", stroke="#444444", width=2.5)
    plate_cm = (fw * cm_per_px, fh * cm_per_px)
    # width dimension above the plate
    dy_ = oy - 26
    svg.line(ox, dy_, ox + plate_w, dy_, DARK, 1.5)
    svg.line(ox, dy_ - 6, ox, dy_ + 6, DARK, 1.5)
    svg.line(ox + plate_w, dy_ - 6, ox + plate_w, dy_ + 6, DARK, 1.5)
    svg.text(ox + plate_w / 2, dy_ - 7, "%.0f cm" % plate_cm[0], size=14, anchor="middle", weight="bold")
    # height dimension left of the plate
    dx_ = ox - 28
    svg.line(dx_, oy, dx_, oy + plate_h, DARK, 1.5)
    svg.line(dx_ - 6, oy, dx_ + 6, oy, DARK, 1.5)
    svg.line(dx_ - 6, oy + plate_h, dx_ + 6, oy + plate_h, DARK, 1.5)
    svg.add("<text x='%g' y='%g' font-size='14' font-weight='bold' text-anchor='middle' fill='%s' "
            "transform='rotate(-90 %g %g)' %s>%.0f cm</text>"
            % (dx_ - 8, oy + plate_h / 2, DARK, dx_ - 8, oy + plate_h / 2, gd_font(), plate_cm[1]))

    # ---- LED strips ---------------------------------------------------------------------------
    plate_centre = (X(fx + fw / 2), Y(fy + fh / 2))
    left, right, top_, bottom = lay.arrow_extent
    arrow_centre = (X((left + right) / 2), Y((top_ + bottom) / 2))

    centres = {}                                  # part id -> list of LED centres
    directions = {}
    for part in lay.parts:
        centres[part["id"]], directions[part["id"]] = led_centres(part)

    def square(c, color, angle, dashed=False):
        dash = " stroke-dasharray='3 2'" if dashed else ""
        svg.add("<rect x='%g' y='%g' width='%d' height='%d' fill='%s' fill-opacity='0.35' stroke='%s' "
                "stroke-width='1.6'%s transform='rotate(%g %g %g)'/>"
                % (c[0] - LED / 2, c[1] - LED / 2, LED, LED, color, color, dash, angle, c[0], c[1]))

    piece_labels = []                             # (centre, outward normal, text)
    callout_targets = {}
    for name, ids, _pin in lay.chains:
        color = COLORS[name]
        row = by_name[name]
        if name == "letters":
            continue
        for pid in ids:
            part = parts[pid]
            angle = part.get("rotate", 0)
            for c in centres[pid]:
                square(c, color, angle, dashed=(name == "closedLight"))
        # data direction arrow and jumper wires
        for index, pid in enumerate(ids):
            cs = centres[pid]
            (dx, dy) = directions[pid]
            if len(cs) >= 2:
                svg.line(cs[0][0], cs[0][1], cs[-1][0], cs[-1][1], DARK, 1.4)
                tip = (cs[-1][0] + dx * (LED / 2 + 5), cs[-1][1] + dy * (LED / 2 + 5))
                px, py = -dy, dx
                svg.path("M %g %g L %g %g L %g %g Z" % (tip[0], tip[1],
                                                         cs[-1][0] + dx * 1 + px * 4, cs[-1][1] + dy * 1 + py * 4,
                                                         cs[-1][0] + dx * 1 - px * 4, cs[-1][1] + dy * 1 - py * 4),
                         DARK, 1, fill=DARK)
            if index + 1 < len(ids):
                nxt = centres[ids[index + 1]]
                svg.line(cs[-1][0], cs[-1][1], nxt[0][0], nxt[0][1], GREY, 1.4, dash="4 3")
        first = centres[ids[0]][0]
        svg.dot(first[0], first[1], DARK, 4)
        # piece labels (LED count of each piece)
        ref = plate_centre if name.startswith("frame") else arrow_centre
        for (label, n), pid in zip(row["pieces"], ids):
            cs = centres[pid]
            mid = ((cs[0][0] + cs[-1][0]) / 2, (cs[0][1] + cs[-1][1]) / 2)
            dx, dy = directions[pid]
            nx, ny = -dy, dx
            if (mid[0] - ref[0]) * nx + (mid[1] - ref[1]) * ny < 0:
                nx, ny = -nx, -ny
            piece_labels.append((mid, (nx, ny), "%d" % n, color))

    # Letter groups: circles of single LEDs, the letter in the middle
    color = COLORS["letters"]
    letters_row = by_name["letters"]
    group_start = 0
    letter_ids = lay.chains[[c[0] for c in lay.chains].index("letters")][1]
    group_centres = []
    pos = 0
    for gi, (ch, n) in enumerate(letters_row["pieces"]):
        ids = letter_ids[pos:pos + n]
        pos += n
        pts = [centres[i][0] for i in ids]
        cx = sum(p[0] for p in pts) / n
        cy = sum(p[1] for p in pts) / n
        group_centres.append((cx, cy, pts))
        for p in pts:
            square(p, color, 0)
        svg.dot(pts[0][0], pts[0][1], DARK, 3.5)
        radius = max(math.hypot(p[0] - cx, p[1] - cy) for p in pts)
        inner = radius - LED / 2
        if n >= 3 and inner >= 18:
            svg.text(cx, cy + 1, ch, size=17, anchor="middle", weight="bold", color=DARK)
            svg.text(cx, cy + 14, "%d" % n, size=10, anchor="middle", color=GREY)
        else:
            svg.text(cx, cy + 5, ch, size=15, anchor="middle", weight="bold", color=DARK)
            svg.text(cx, cy + radius + LED / 2 + 11, "%d" % n, size=10, anchor="middle", color=GREY)
    # jumper wires between the letter groups (reading order)
    for (ax, ay, apts), (bx, by, bpts) in zip(group_centres, group_centres[1:]):
        svg.line(apts[-1][0], apts[-1][1], bpts[0][0], bpts[0][1], GREY, 1.2, dash="4 3")

    # Hole text "CLOSED" cut into the plate (outline only, drawn over the LED piece that sits behind the plate)
    ccx, ccy, cwidth = lay.closed_box
    svg.add("<text x='%g' y='%g' font-size='46' font-weight='bold' text-anchor='middle' fill='none' "
            "stroke='#444444' stroke-width='2' stroke-dasharray='5 3' textLength='%g' "
            "lengthAdjust='spacingAndGlyphs' %s>CLOSED</text>"
            % (X(ccx), Y(ccy) + 16, cwidth * S * 1.12, gd_font()))

    # piece labels drawn last so that they stay on top
    for (mid, (nx, ny), text, color) in piece_labels:
        lx, ly = mid[0] + nx * 22, mid[1] + ny * 22
        svg.rect(lx - 11, ly - 8, 22, 16, fill="#ffffff", stroke=color, width=1.2, rx=3)
        svg.text(lx, ly + 4, text, size=11, anchor="middle", weight="bold", color=color)

    # ---- arrow dimensions -----------------------------------------------------------------------
    arrow_w_cm = (right - left) * cm_per_px
    arrow_h_cm = (bottom - top_) * cm_per_px
    ay = Y(bottom) + 26
    svg.line(X(left), ay, X(right), ay, DARK, 1.2)
    svg.line(X(left), ay - 5, X(left), ay + 5, DARK, 1.2)
    svg.line(X(right), ay - 5, X(right), ay + 5, DARK, 1.2)
    svg.text((X(left) + X(right)) / 2, ay + 16, "arrow %.1f cm" % arrow_w_cm, size=12, anchor="middle", weight="bold")
    vx = X(right) + 26
    svg.line(vx, Y(top_), vx, Y(bottom), DARK, 1.2)
    svg.line(vx - 5, Y(top_), vx + 5, Y(top_), DARK, 1.2)
    svg.line(vx - 5, Y(bottom), vx + 5, Y(bottom), DARK, 1.2)
    svg.add("<text x='%g' y='%g' font-size='12' font-weight='bold' text-anchor='middle' fill='%s' "
            "transform='rotate(-90 %g %g)' %s>%.1f cm</text>"
            % (vx - 7, (Y(top_) + Y(bottom)) / 2, DARK, vx - 7, (Y(top_) + Y(bottom)) / 2, gd_font(), arrow_h_cm))

    # ---- callouts ---------------------------------------------------------------------------------
    def target(name):
        if name == "frameCw":
            return centres["frame_cw_top"][-1], "R"
        if name == "frameCcw":
            return centres["frame_ccw_left"][0], "L"
        if name == "letters":
            cx, cy, pts = group_centres[0]
            return (min(p[0] for p in pts) - LED / 2, cy), "L"
        if name == "closedLight":
            return centres["closed_light"][-1], "R"
        if name == "arrowBodyTop":
            return centres["arrow_body_top"][-1], "R"
        if name == "arrowBodyBottom":
            return centres["arrow_body_bottom"][0], "L"
        if name == "arrowCapLeft":
            return centres["arrow_cap_left"][0], "L"
        if name == "arrowCapRight":
            return centres["arrow_cap_right"][-1], "R"
        if name == "arrowHeadLeft":
            return centres["arrow_head_left_upper_wing"][-1], "L"
        return centres["arrow_head_right_upper_wing"][-1], "R"

    boxes = {"L": [], "R": []}
    for row in info:
        pt, side = target(row["name"])
        lines = ["%s" % row["name"], "D%d - %d LEDs - %s" % (row["pin"], row["leds"],
                                                              "1 piece" if len(row["pieces"]) == 1
                                                              else "%d pieces" % len(row["pieces"]))]
        if row["name"] == "letters":
            lines.append("one strip, one piece per letter,")
            lines.append("joined in reading order (see table)")
        elif row["name"] == "closedLight":
            lines.append("white LEDs BEHIND the plate: they")
            lines.append("light the CLOSED hole text (about")
            lines.append("10 x 2.5 cm) while the casino is closed")
            lines.append("piece: %d LEDs (#0-%d)" % (row["leds"], row["leds"] - 1))
        else:
            for label, n, a, b in sign_data.with_ranges(row["pieces"]):
                lines.append("%s: %d LEDs (#%d-%d)" % (label, n, a, b) if n > 1 else "%s: 1 LED (#%d)" % (label, a))
        boxes[side].append([row, pt, lines])
    for side in ("L", "R"):
        group = sorted(boxes[side], key=lambda b: b[1][1])
        y_next = oy - 10
        for item in group:
            row, pt, lines = item
            h = 15 * len(lines) + 14
            y0 = max(pt[1] - h / 2, y_next)
            item.append(y0)
            y_next = y0 + h + 12
        for row, pt, lines, y0 in group:
            h = 15 * len(lines) + 14
            color = COLORS[row["name"]]
            if side == "L":
                bx0 = 14
            else:
                bx0 = ox + plate_w + 60 + 14
            bx1 = bx0 + PANEL_W - 28
            svg.rect(bx0, y0, bx1 - bx0, h, fill="#ffffff", stroke=color, width=2.5, rx=4)
            svg.text(bx0 + 10, y0 + 18, lines[0], size=13, weight="bold", color=color)
            for k, line in enumerate(lines[1:]):
                svg.text(bx0 + 10, y0 + 33 + k * 15, line, size=10.5, color=DARK)
            ex = bx1 if side == "L" else bx0
            svg.line(ex, y0 + h / 2, pt[0], pt[1], color, 1.4)
            svg.dot(pt[0], pt[1], color, 3.5)

    # ---- legend ---------------------------------------------------------------------------------------
    ly = oy + plate_h + 38
    svg.dot(ox + 8, ly - 4, DARK, 4)
    svg.text(ox + 20, ly, "= first LED of the strip (DIN, index #0)", size=11)
    svg.line(ox + 270, ly - 4, ox + 300, ly - 4, DARK, 1.4)
    svg.text(ox + 308, ly, "= data direction inside a piece", size=11)
    svg.line(ox + 505, ly - 4, ox + 535, ly - 4, GREY, 1.4, dash="4 3")
    svg.text(ox + 543, ly, "= wire jumper between pieces", size=11)
    svg.text(ox, ly + 20, "Numbers next to the pieces and inside the letter circles are LED counts. #a-b are LED indexes in the sketch.",
             size=11, color=GREY)
    svg.text(ox, ly + 36, "The gaps between strips are drawn only for clarity: on the real sign the strips touch each other.",
             size=11, color=GREY)
    svg.text(ox, ly + 52, "A letter group may be laid out behind its letter in any shape; the circle is only an illustration.",
             size=11, color=GREY)

    # ---- tables ------------------------------------------------------------------------------------------
    pw_ = 760
    table_y = 120
    height2 = int(table_y + 24 + n_rows * 22 + 3 * 16 + 24 + 60 + 72 + 90 + 150)
    ps = Svg(pw_, height2)
    ps.text(pw_ / 2, 34, "Casino sign - parts list: strips, LED counts and cutting list", size=18,
            anchor="middle", weight="bold")
    ps.text(pw_ / 2, 56, "%d strips, %d pieces, %d LEDs = %.2f m of strip (%g LEDs per metre, pitch %.1f mm)."
            % (total_strips, total_pieces, total_leds, total_leds / args.leds_per_m, args.leds_per_m,
               cm_per_px * gd.PITCH * 10), size=12, anchor="middle", color=GREY)
    ps.text(pw_ / 2, 76, "Cut the strip only between two LEDs (at the cut marks). See layout-diagram.svg for the positions.",
            size=12, anchor="middle", color=GREY)
    ps.text(40, table_y - 12, "LED counts and cutting list", size=15, weight="bold")
    cols = [40, 190, 235, 285, 345]
    heads = ["Strip", "Pin", "LEDs", "Pieces", "LEDs per piece (index range)"]
    ps.rect(30, table_y, pw_ - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    for x, h in zip(cols, heads):
        ps.text(x, table_y + 17, h, size=12, weight="bold")
    y = table_y + 24
    for i, row in enumerate(info):
        if row["name"] == "letters":
            lines = ["see the letter table below"]
        else:
            rngs = sign_data.with_ranges(row["pieces"])
            items = ["%s %d (#%d-%d)" % (l, n, a, b) if n > 1 else "%s 1 (#%d)" % (l, a) for l, n, a, b in rngs]
            lines, cur = [], ""
            for item in items:               # wrap the piece list so it stays inside the page
                if cur and len(cur) + len(item) > 46:
                    lines.append(cur)
                    cur = item
                else:
                    cur = cur + ",  " + item if cur else item
            lines.append(cur)
        h = 22 + 16 * (len(lines) - 1)
        ps.rect(30, y, pw_ - 60, h, fill="#ffffff" if i % 2 == 0 else "#fafafa", stroke="#dddddd", width=1)
        color = COLORS[row["name"]]
        ps.rect(34, y + 5, 10, 12, fill=color, stroke=color, width=1)
        ps.text(52, y + 16, row["name"], size=12, weight="bold", color=color)
        ps.text(cols[1], y + 16, "D%d" % row["pin"], size=12)
        ps.text(cols[2], y + 16, "%d" % row["leds"], size=12, weight="bold")
        ps.text(cols[3], y + 16, "%d" % len(row["pieces"]), size=12)
        for k, line in enumerate(lines):
            ps.text(cols[4], y + 16 + 16 * k, line, size=12)
        y += h
    ty = y
    ps.rect(30, ty, pw_ - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    ps.text(40, ty + 17, "Total", size=12, weight="bold")
    ps.text(cols[2], ty + 17, "%d" % total_leds, size=12, weight="bold")
    ps.text(cols[3], ty + 17, "%d" % total_pieces, size=12, weight="bold")
    ps.text(cols[4], ty + 17, "%d strips, %d LEDs, %d pieces" % (total_strips, total_leds, total_pieces), size=12)

    # letter table
    lt = ty + 60
    ps.text(40, lt - 12, "Letters (strip D%d): LEDs per letter group and LED index range" % by_name["letters"]["pin"],
             size=15, weight="bold")
    rng = sign_data.with_ranges(letters_row["pieces"])
    cell = (pw_ - 60) / max(len(rng), 1)
    ps.rect(30, lt, pw_ - 60, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    ps.rect(30, lt + 24, pw_ - 60, 24, fill="#ffffff", stroke="#dddddd", width=1)
    ps.rect(30, lt + 48, pw_ - 60, 24, fill="#fafafa", stroke="#dddddd", width=1)
    first = len(cfg["LINE_1_TEXT"])
    for i, (ch, n, a, b) in enumerate(rng):
        x = 30 + cell * i + cell / 2
        ps.text(x, lt + 17, "%s  (%s)" % (ch, "line 1" if i < first else "line 2"), size=11, anchor="middle", weight="bold",
                 color=COLORS["letters"])
        ps.text(x, lt + 41, "%d LEDs" % n, size=12, anchor="middle")
        ps.text(x, lt + 65, "#%d-%d" % (a, b) if n > 1 else "#%d" % a, size=12, anchor="middle")
        if i:
            ps.line(30 + cell * i, lt, 30 + cell * i, lt + 72, "#dddddd", 1)
    ps.text(40, lt + 94, "The letter pieces are joined in reading order (CASINO, then OPEN) with wire jumpers;",
            size=11, color=GREY)
    ps.text(40, lt + 110, "together they are the one strip 'letters' on pin D%d." % by_name["letters"]["pin"],
            size=11, color=GREY)

    OUTPUT_PARTS.write_text(ps.render_a4(landscape=False))
    print("Wrote %s (A4 portrait, 12-unit text = %.2f mm)" % (OUTPUT_PARTS, 12 * ps.mm_per_unit))

    OUTPUT.write_text(svg.render_a4(landscape=True))
    print("Wrote %s (A4 landscape, 12-unit text = %.2f mm)" % (OUTPUT, 12 * svg.mm_per_unit))


def gd_font():
    return "font-family='DejaVu Sans, Arial, sans-serif'"


if __name__ == "__main__":
    main()
