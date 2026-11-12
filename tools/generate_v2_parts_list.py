#!/usr/bin/env python3
"""Make the parts list of Casino Open Sign V2: casino_open_sign_v2/parts-list.csv and parts-list.svg (+ .pdf by export_pdf.py).

Built from tools/parts_data.py (partco.fi snapshot of 2026-11-02; prices, stock and links are NOT refreshed here) with the V2
changes: two supplies (5 V for the Mega, shield and sticks; 12 V for the two WS2811 strips), 11 Grove sticks + the Grove Base
Shield, 2 data resistors.

    python3 tools/generate_v2_parts_list.py
"""

import csv
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import parts_data as pd_
import v2_data as v2
from generate_schematic import Svg

OUT = Path(__file__).resolve().parent.parent / "casino_open_sign_v2"


def v2_rows():
    cfg = v2.config()
    strips = v2.strip_info(cfg)
    pixels = sum(s["leds"] for s in strips)
    rows = [dict(r) for r in pd_.bill_of_materials(pixels, len(strips))]
    rows = [r for r in rows if r["group"] != pd_.G_TEST]
    by_sku = {r["sku"]: r for r in rows}
    drop = {"LED-N-RGB-W2812X300-5M", "SUA-10A"}      # old-design alternatives, replaced by real rows below
    rows = [r for r in rows if not (r["group"] == pd_.G_INS and r["sku"] in ("LRS-50-5", "SUA-10A"))]
    for r in rows:
        if r["sku"] == "LED-N-RGB-W2812X150-5M" and r["group"] == pd_.G_LED:
            r["used_for"] = "frame (18 pixels) + letters (10 pixels): %d pixels = %.1f m, one reel is enough" % (pixels, pixels / 10.0)
            r["note"] = ("one control chip per 3 LEDs = 50 pixels of 10 cm per reel; colour order, bit speed and current were tested only "
                         "with the strip tests (strip_frame_demo_test/); only one reel in stock at the snapshot")
        elif r["sku"] == "LRS-50-12" and r["group"] == pd_.G_POWER:
            r["part"], r["used_for"] = "12 V power supply 4.2 A (PSU2)", "powers ONLY the two 12 V strips (28 pixels peak about 1-2 A at the brightness limit)"
        elif r["sku"] == "SULPES-APUR":
            r["qty"], r["used_for"] = 2, "fuse holders for F1 (5 V line) and F2 (+12 V line)"
        elif r["sku"] == "SUA-5A" and r["group"] == pd_.G_POWER:
            r["used_for"] = "F2 in the +12 V line (the two strips need about 2 A at most)"
        elif r["sku"] == "ELKO105-1000U-25V":
            r["qty"], r["part"] = 2, "Capacitor 1000 uF 25 V"
            r["used_for"] = "C1 across the 5 V supply (16 V or more) and C2 across +12 V at the strips (25 V)"
        elif r["sku"] == "LAITEJOHTO-2V-KULMA":
            r["qty"], r["used_for"] = 2, "230 V into PSU1 and PSU2"
        elif r["sku"] == "KOTEV-171":
            r["used_for"] = "encloses both power supplies and the mains wiring (check the size: two supplies)"
        elif r["sku"] == "VASTUS-2W-330R" and r["group"] == pd_.G_WIRING:
            r["qty"], r["used_for"] = 2, "R1, R2: data lines of the frame (D12) and the letters (D10) strips; the sticks have none"
        elif r["sku"] == "KYTKENTÄLEVY" or r["sku"] == "KL-PL200":
            r["used_for"] = "resistors, capacitors, fuse holders, hex switches, connections"
        elif r["sku"] == "-" and r["part"] == "Red plexiglass letters":
            r["product"] = "10 letters (C A S I N O O P E N), size to fit the plate"
        elif r["sku"] == "-" and r["part"] == "Black plate 60 x 30 cm":
            r["used_for"] = "the sign plate 60 x 30 cm (with the CLOSED hole text and the arrow stick holes)"
    power = [i for i, r in enumerate(rows) if r["group"] == pd_.G_POWER]
    at = power[0]
    rows.insert(at, pd_.part(pd_.G_POWER, "5 V power supply 10 A (PSU1)", "feeds the Mega 5V pins, the Grove shield, the 11 sticks and the LCD", 1, "pcs", 23.79,
                             "in stock (10 pcs)", "HAKKURI MODULI 50W 5VDC 10A (Mean Well)", "LRS-50-5",
                             "https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777", "as used in stick_test_direct and sticks_11_test"))
    rows.insert(at + 1, pd_.part(pd_.G_POWER, "Blade fuse 10 A", "F1 right after PSU1 (the Mega 5V pins carry the stick current)", 1, "pcs", 0.31,
                                 "in stock (76 pcs)", "AUTOSULAKE 32V 10A punainen", "SUA-10A",
                                 "https://partco.fi/tuote/autosulake-32v-10a-punainen-5320"))
    led = [i for i, r in enumerate(rows) if r["group"] == pd_.G_LED][-1]
    rows.insert(led + 1, pd_.part(pd_.G_LED, "Grove RGB LED stick (10x WS2813 mini)", "10 arrow sticks + the CLOSED light stick", 11, "pcs", None,
                                  "not looked up", "Grove - RGB LED Stick (10 pcs WS2813 Mini)", "-", pd_.SITE,
                                  "the owner's sticks are already tested (stick_test_direct, sticks_11_test); search \"Grove RGB LED Stick\" at partco.fi; link is the front page",
                                  certainty="probable"))
    rows.insert(led + 2, pd_.part(pd_.G_LED, "Grove Base Shield for Arduino Mega", "the 11 sticks plug into its connectors D2-D8 and A0-A3", 1, "pcs", None,
                                  "not looked up", "Grove Base Shield V2.0 (switch set to 5 V)", "-", pd_.SITE,
                                  "price not looked up; link is the front page; the 3V3/5V switch must be on 5 V", certainty="probable"))
    return rows


def eur(v):
    return "" if v is None else "%.2f" % v


def in_total(r):
    return "Yes" if r["counted"] else ("Insurance" if r["insurance"] else "No")


def write_csv(rows):
    total, unpriced, insurance = pd_.totals(rows)
    with open(OUT / "parts-list.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Bought", "Group", "Part", "Used for", "Qty", "Unit", "Unit price EUR (VAT incl.)", "Line total EUR", "In total", "Certainty",
                    "Availability (checked %s)" % pd_.CHECKED_ON, "Product", "SKU", "Link", "Notes"])
        for r in rows:
            w.writerow(["", r["group"], r["part"], r["used_for"], r["qty"], r["unit"], eur(r["unit_price"]), eur(pd_.line_total(r)), in_total(r),
                        r["certainty"], r["availability"], r["product"], r["sku"], r["url"], r["note"]])
        w.writerow([])
        w.writerow(["", "", "Total of the rows marked Yes that have a price", "", "", "", "", "%.2f" % total])
        w.writerow(["", "", "Insurance rows (marked Insurance)", "", "", "", "", "%.2f" % insurance])
        w.writerow(["", "", pd_.SNAPSHOT_NOTE])
    return total, unpriced, insurance


def write_svg(rows, total, unpriced, insurance):
    W = 1400
    y = 120
    line_h = 15
    blocks = []
    for r in rows:
        used = textwrap.wrap(r["used_for"], 58) or [""]
        part = textwrap.wrap(r["part"], 40) or [""]
        h = max(len(used), len(part)) * line_h + 8
        blocks.append((r, part, used, h))
    H = y + 60 + sum(b[3] for b in blocks) + 150 + 60
    svg = Svg(W, H)
    svg.text(W / 2, 34, "Casino Open Sign V2 - parts list (partco.fi)", size=20, anchor="middle", weight="bold")
    svg.text(W / 2, 58, pd_.SNAPSHOT_NOTE, size=12, anchor="middle", color="#c62828")
    svg.text(W / 2, 78, "%s. Rows without a price: not sold at partco.fi or not looked up. Links: see parts-list.csv." % pd_.VAT_NOTE, size=12, anchor="middle", color="#666666")
    cols = [30, 90, 400, 880, 940, 1010, 1100, 1190, 1290]
    heads = ["Qty", "Part", "Used for", "Unit", "EUR each", "EUR line", "In total", "Certainty", "SKU"]
    cols = [30, 80, 380, 870, 920, 990, 1070, 1150, 1240]
    svg.rect(20, y - 4, W - 40, 24, fill="#eeeeee", stroke="#bbbbbb", width=1)
    for x, h in zip(cols, heads):
        svg.text(x, y + 13, h, size=12, weight="bold")
    yy = y + 20
    group = None
    for r, part, used, h in blocks:
        if r["group"] != group:
            group = r["group"]
            svg.rect(20, yy, W - 40, 20, fill="#e3f2fd", stroke="#bbdefb", width=1)
            svg.text(30, yy + 14, group, size=12, weight="bold", color="#0d47a1")
            yy += 20
        svg.rect(20, yy, W - 40, h, fill="#ffffff", stroke="#e0e0e0", width=1)
        svg.text(cols[0], yy + 13, str(r["qty"]), size=12)
        for k, line in enumerate(part):
            svg.text(cols[1], yy + 13 + k * line_h, line, size=12, weight="bold")
        for k, line in enumerate(used):
            svg.text(cols[2], yy + 13 + k * line_h, line, size=11.5)
        svg.text(cols[3], yy + 13, r["unit"], size=12)
        svg.text(cols[4], yy + 13, eur(r["unit_price"]) or "-", size=12)
        svg.text(cols[5], yy + 13, eur(pd_.line_total(r)) or "-", size=12)
        svg.text(cols[6], yy + 13, in_total(r), size=12)
        svg.text(cols[7], yy + 13, r["certainty"], size=12)
        svg.text(cols[8], yy + 13, r["sku"][:18], size=10)
        yy += h
    yy += 24
    svg.text(30, yy, "Total of the rows marked Yes that have a price: %.2f EUR (%d counted rows have no price). Insurance rows: %.2f EUR." % (total, unpriced, insurance), size=14, weight="bold")
    svg.text(30, yy + 22, "Not in the list: your own Grove sticks if you already have them, the plate material, the letters, the frosted plastic, tools.", size=11, color="#666666")
    svg.text(30, yy + 38, "Strip pixels: frame 18 + letters 10 = 28 pixels of one 5 m reel (50 pixels); the 11 sticks and the Grove shield are listed without price.", size=11, color="#666666")
    (OUT / "parts-list.svg").write_text(svg.render_a4(landscape=False))


def main():
    rows = v2_rows()
    total, unpriced, insurance = write_csv(rows)
    write_svg(rows, total, unpriced, insurance)
    print("Wrote parts-list.csv and parts-list.svg in %s (%d rows, total %.2f EUR, %d unpriced)" % (OUT, len(rows), total, unpriced))


if __name__ == "__main__":
    main()
