#!/usr/bin/env python3
"""Make the parts list (shopping list) of the casino sign: PDF, CSV and XLSX.

    python3 tools/generate_parts_list.py

Reads the LED counts from ../sketch.ino and the parts from parts_data.py. Output (project folder):
    parts-list.pdf   A4 landscape, printable, with clickable partco.fi links and a page with the full URLs
    parts-list.csv   for any spreadsheet program (one row per part, with the full URL)
    parts-list.xlsx  Excel workbook; line totals and sums are formulas, links are clickable

PDF needs Python Playwright + Chromium, XLSX needs openpyxl. After changing the XLSX run the
LibreOffice recalculation (see AI_CONTEXT.md) so the cached values are filled in.
"""

import csv
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_diagram as gd
import parts_data as pd_
import sign_data
from generate_schematic import Svg

ROOT = Path(__file__).resolve().parent.parent
PDF, CSV, XLSX = ROOT / "parts-list.pdf", ROOT / "parts-list.csv", ROOT / "parts-list.xlsx"
GREY, DARK = "#666666", "#222222"


def load_rows():
    cfg = gd.read_config(gd.SKETCH.read_text())
    strips = sign_data.strip_info(cfg)
    n_strips, _pieces, n_leds = sign_data.totals(strips)
    return pd_.bill_of_materials(n_leds, n_strips), n_leds, n_strips


def in_total(r):
    return "Yes" if r["counted"] else ("Insurance" if r["insurance"] else "No")


def eur(v):
    return "" if v is None else "%.2f" % v


# ------------------------------------------------------------------ CSV
def write_csv(rows):
    total, unpriced, insurance = pd_.totals(rows)
    with open(CSV, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Bought", "Group", "Part", "Used for", "Qty", "Unit", "Unit price EUR (VAT incl.)",
                    "Line total EUR", "In total", "Certainty", "Availability (checked %s)" % pd_.CHECKED_ON, "Product", "SKU",
                    "Link", "Notes"])
        for r in rows:
            w.writerow(["", r["group"], r["part"], r["used_for"], r["qty"], r["unit"], eur(r["unit_price"]),
                        eur(pd_.line_total(r)), in_total(r), r["certainty"], r["availability"], r["product"],
                        r["sku"], r["url"], r["note"]])
        w.writerow([])
        w.writerow(["", "", "Total of the rows marked Yes that have a price", "", "", "", "", "%.2f" % total])
        w.writerow(["", "", "Insurance rows (marked Insurance)", "", "", "", "", "%.2f" % insurance])
        w.writerow(["", "", "Total with insurance", "", "", "", "", "%.2f" % (total + insurance)])
        w.writerow(["", "", "Rows marked Yes without a price (not in the total)", "", "", "", "", unpriced])
        w.writerow(["", "", "%s. %s The XLSX file has the same data with formulas." % (pd_.VAT_NOTE, pd_.SNAPSHOT_NOTE)])


# ------------------------------------------------------------------ XLSX
def write_xlsx(rows):
    from openpyxl import Workbook
    from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
    from openpyxl.utils import get_column_letter

    wb = Workbook()
    ws = wb.active
    ws.title = "Parts list"
    arial = lambda **kw: Font(name="Arial", size=kw.pop("size", 10), **kw)
    thin = Side(style="thin", color="BBBBBB")
    box = Border(left=thin, right=thin, top=thin, bottom=thin)

    ws["A1"] = "Casino sign - parts list (shopping list)"
    ws["A1"].font = arial(size=14, bold=True)
    ws["A2"] = ("Prices include VAT 25.5 %%, checked on partco.fi on %s. Blue cells (quantity, unit price) are inputs: "
                "change them and the totals recalculate. 'In total' = Yes: needed; Insurance: extra parts for the uncertain "
                "situations; No: alternatives and options. Certainty = how sure we are that the part is the right one." % pd_.CHECKED_ON)
    ws["A2"].font = arial(italic=True, color="555555")
    ws["A3"] = pd_.SNAPSHOT_NOTE
    ws["A3"].font = arial(italic=True, bold=True, color="C62828")
    heads = ["Bought", "Group", "Part", "Used for", "Qty", "Unit", "Unit price EUR", "Line total EUR", "In total",
             "Certainty", "Availability", "Product", "SKU", "Link", "Notes"]
    widths = [8, 24, 32, 42, 6, 7, 12, 13, 10, 11, 26, 46, 24, 20, 60]
    hr = 4
    for c, (h, wd) in enumerate(zip(heads, widths), start=1):
        cell = ws.cell(row=hr, column=c, value=h)
        cell.font = arial(bold=True, color="FFFFFF")
        cell.fill = PatternFill("solid", fgColor="37474F")
        cell.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        cell.border = box
        ws.column_dimensions[get_column_letter(c)].width = wd
    first = hr + 1
    for i, r in enumerate(rows):
        n = first + i
        vals = [None, r["group"], r["part"], r["used_for"], r["qty"], r["unit"], r["unit_price"],
                '=IF(ISNUMBER(G%d),E%d*G%d,"")' % (n, n, n), in_total(r), r["certainty"], r["availability"],
                r["product"], r["sku"], None, r["note"]]
        for c, v in enumerate(vals, start=1):
            cell = ws.cell(row=n, column=c, value=v)
            cell.font = arial()
            cell.border = box
            cell.alignment = Alignment(vertical="top", wrap_text=c in (2, 3, 4, 11, 12, 15))
        ws.cell(row=n, column=5).font = arial(color="0000FF")             # inputs are blue
        ws.cell(row=n, column=7).font = arial(color="0000FF")
        ws.cell(row=n, column=7).number_format = '#,##0.00;(#,##0.00);"-"'
        ws.cell(row=n, column=8).number_format = '#,##0.00;(#,##0.00);"-"'
        ws.cell(row=n, column=1).alignment = Alignment(horizontal="center", vertical="top")
        ws.cell(row=n, column=9).alignment = Alignment(horizontal="center", vertical="top")
        ws.cell(row=n, column=10).alignment = Alignment(horizontal="center", vertical="top")
        link = ws.cell(row=n, column=14, value="open on partco.fi" if r["sku"] != "-" or "partco.fi/" in r["url"] else r["url"])
        link.hyperlink = r["url"]
        link.font = arial(color="0563C1", underline="single")
        link.alignment = Alignment(vertical="top")
        if not r["counted"]:
            fill = "FFF8E1" if r["insurance"] else "F3F3F3"
            for c in range(1, 16):
                if c != 14:
                    ws.cell(row=n, column=c).fill = PatternFill("solid", fgColor=fill)
        if r["certainty"] == "uncertain":
            ws.cell(row=n, column=10).font = arial(bold=True, color="C62828")
    last = first + len(rows) - 1

    s = last + 2
    ws.cell(row=s, column=3, value="Total (rows with In total = Yes and a price)").font = arial(bold=True)
    t = ws.cell(row=s, column=8, value='=SUMIFS(H%d:H%d,I%d:I%d,"Yes")' % (first, last, first, last))
    t.font = arial(bold=True)
    t.number_format = '#,##0.00'
    ws.cell(row=s + 1, column=3, value="Rows with In total = Yes but no price (not in the total)").font = arial()
    ws.cell(row=s + 1, column=8, value='=COUNTIFS(I%d:I%d,"Yes",G%d:G%d,"")' % (first, last, first, last)).font = arial()
    ws.cell(row=s + 2, column=3, value="Insurance rows (extra parts for the uncertain situations)").font = arial()
    ins = ws.cell(row=s + 2, column=8, value='=SUMIFS(H%d:H%d,I%d:I%d,"Insurance")' % (first, last, first, last))
    ins.font = arial()
    ins.number_format = '#,##0.00'
    ws.cell(row=s + 3, column=3, value="Total with insurance").font = arial(bold=True)
    gt = ws.cell(row=s + 3, column=8, value='=H%d+H%d' % (s, s + 2))
    gt.font = arial(bold=True)
    gt.number_format = '#,##0.00'
    ws.cell(row=s + 4, column=3, value="Rows marked Bought (checked off)").font = arial()
    ws.cell(row=s + 4, column=8, value='=COUNTA(A%d:A%d)' % (first, last)).font = arial()

    g = s + 6
    ws.cell(row=g, column=3, value="Subtotals by group (In total = Yes or Insurance)").font = arial(bold=True)
    groups = []
    for r in rows:
        if r["group"] not in groups and (r["counted"] or r["insurance"]):
            groups.append(r["group"])
    for k, name in enumerate(groups, start=1):
        ws.cell(row=g + k, column=3, value=name).font = arial()
        c = ws.cell(row=g + k, column=8, value='=SUMIFS(H%d:H%d,B%d:B%d,C%d,I%d:I%d,"Yes")+SUMIFS(H%d:H%d,B%d:B%d,C%d,I%d:I%d,"Insurance")' % (
            first, last, first, last, g + k, first, last, first, last, first, last, g + k, first, last))
        c.font = arial()
        c.number_format = '#,##0.00'
    ws.cell(row=g + len(groups) + 2, column=3,
            value="Source: partco.fi product pages and category listings, checked %s. Quantities are what is needed; "
                  "the insurance rows are extra. Wire lengths are estimates." % pd_.CHECKED_ON).font = arial(italic=True, color="555555")
    ws.freeze_panes = ws.cell(row=first, column=4)
    ws.sheet_view.zoomScale = 90
    ws.page_setup.orientation = "landscape"
    ws.page_setup.paperSize = ws.PAPERSIZE_A4
    ws.page_setup.fitToWidth = 1
    ws.page_setup.fitToHeight = 0
    ws.sheet_properties.pageSetUpPr.fitToPage = True
    wb.save(XLSX)


# ------------------------------------------------------------------ PDF
W, H = 1100, 778                      # drawing units of one A4 landscape page
LEFT, RIGHT = 24, 1076
COLS = dict(box=24, part=44, used=214, qty=452, unit=516, line=590, avail=606, prod=752)


def wrap(text, n):
    return textwrap.wrap(text, n) or [""]


def header(svg, y):
    svg.rect(LEFT - 4, y - 14, RIGHT - LEFT + 8, 22, fill="#37474F", stroke="#37474F", width=1)
    for x, label, anchor in ((COLS["box"], "", "start"), (COLS["part"], "Part", "start"), (COLS["used"], "Used for", "start"),
                             (COLS["qty"], "Qty", "end"), (COLS["unit"], "Unit EUR", "end"), (COLS["line"], "Line EUR", "end"),
                             (COLS["avail"], "Availability", "start"), (COLS["prod"], "Product / SKU / notes", "start"),
                             (RIGHT, "Link", "end")):
        svg.text(x, y + 1, label, size=11, anchor=anchor, color="#ffffff", weight="bold")


def build_pages(rows, n_leds, n_strips):
    total, unpriced, insurance = pd_.totals(rows)
    pages, links = [], []
    svg = None
    y = 0

    def new_page(first):
        nonlocal svg, y
        svg = Svg(W, H)
        pages.append(svg)
        svg.text(W / 2, 30, "Casino sign - parts list (shopping list)", size=19, anchor="middle", weight="bold")
        svg.text(W / 2, 46, "Prices incl. VAT 25.5 %%, checked on partco.fi %s. Plan: 12 V colour strip. (probable) / (uncertain) = how sure "
                 "we are of the part. [n] = link number." % pd_.CHECKED_ON, size=10.5, anchor="middle", color=GREY)
        svg.text(W / 2, 61, pd_.SNAPSHOT_NOTE, size=10.5, anchor="middle", color="#c62828", weight="bold")
        y = 88
        header(svg, y)
        y += 24

    new_page(True)
    group = None
    for r in rows:
        part_l = wrap(r["part"], 22) + (["(" + r["certainty"] + ")"] if r["certainty"] != "sure" else [])
        used_l = wrap(r["used_for"], 36)
        avail_l = wrap(r["availability"], 24)
        prod_l = wrap(r["product"] + ("  (" + r["sku"] + ")" if r["sku"] != "-" else ""), 50) + \
            [("> " + ln) for ln in wrap(r["note"], 52)] if r["note"] else wrap(r["product"] + ("  (" + r["sku"] + ")" if r["sku"] != "-" else ""), 50)
        n_lines = max(len(part_l), len(used_l), len(avail_l), len(prod_l))
        h = 14.5 * n_lines + 8
        need = h + (20 if r["group"] != group else 0)
        if y + need > H - 40:
            new_page(False)
            group = None
        if r["group"] != group:
            group = r["group"]
            svg.rect(LEFT - 4, y - 12, RIGHT - LEFT + 8, 18, fill="#eceff1", stroke="#eceff1", width=1)
            svg.text(LEFT, y + 1, group, size=11.5, weight="bold")
            y += 20
        links.append(r["url"])
        k = len(links)
        grey = DARK if r["counted"] else GREY
        svg.rect(COLS["box"], y - 9, 11, 11, fill="#ffffff", stroke=DARK, width=1.2)
        for i, ln in enumerate(part_l):
            tag = ln.startswith("(") and ln.endswith(")")
            tcolor = "#c62828" if ln == "(uncertain)" else ("#ef6c00" if ln == "(probable)" else grey)
            svg.text(COLS["part"], y + i * 14.5, ln, size=9.5 if tag else 10.5,
                     weight="bold" if (i == 0 or ln == "(uncertain)") else "normal", color=tcolor if tag else grey)
        for i, ln in enumerate(used_l):
            svg.text(COLS["used"], y + i * 14.5, ln, size=10, color=grey)
        svg.text(COLS["qty"], y, "%g %s" % (r["qty"], r["unit"]), size=10.5, anchor="end", color=grey)
        svg.text(COLS["unit"], y, eur(r["unit_price"]) or "-", size=10.5, anchor="end", color=grey)
        lt = pd_.line_total(r)
        svg.text(COLS["line"], y, eur(lt) or "-", size=10.5, anchor="end", weight="bold" if r["counted"] else "normal", color=grey)
        for i, ln in enumerate(avail_l):
            bad = "SOLD OUT" in r["availability"] or "out of stock" in r["availability"]
            svg.text(COLS["avail"], y + i * 14.5, ln, size=10, color="#c62828" if bad else grey, weight="bold" if bad else "normal")
        for i, ln in enumerate(prod_l):
            svg.text(COLS["prod"], y + i * 14.5, ln, size=9.5 if ln.startswith(">") else 10, color=GREY if ln.startswith(">") else grey)
        label = "[%d]" % k
        svg.text(RIGHT, y, label, size=10.5, anchor="end", color="#0563C1")
        svg.add("<a href='%s' target='_blank'><rect x='%g' y='%g' width='34' height='14' fill='#ffffff' fill-opacity='0'/></a>"
                % (r["url"], RIGHT - 32, y - 11))
        y += h
        svg.line(LEFT - 4, y - 11, RIGHT + 4, y - 11, "#dddddd", 0.8)
    # totals
    if y + 90 > H - 20:
        new_page(False)
    y += 14
    svg.rect(LEFT - 4, y - 14, RIGHT - LEFT + 8, 80, fill="#fffde7", stroke="#bbbbbb", width=1)
    svg.text(LEFT + 6, y + 4, "Total of the rows with a price (not counting insurance, alternatives and options): %.2f EUR (VAT incl.)" % total,
             size=13, weight="bold")
    svg.text(LEFT + 6, y + 56, "Insurance rows (extra parts for the uncertain situations): %.2f EUR. Total with insurance: %.2f EUR."
             % (insurance, total + insurance), size=11.5, weight="bold")
    svg.text(LEFT + 6, y + 22, "%d counted rows have no price (wire, solder, plate, letters, frosted plastic, mounting): buy elsewhere or "
             "check the price yourself." % unpriced, size=10.5)
    svg.text(LEFT + 6, y + 38, "The LED strip is the most uncertain part (only 1 reel in stock; pixel size, colour order and speed unknown): "
             "test a short piece first with strip_test/.", size=10.5, color="#c62828")
    # link page
    svg = Svg(W, H)
    pages.append(svg)
    svg.text(W / 2, 30, "Links to partco.fi (numbers match the [n] in the table)", size=17, anchor="middle", weight="bold")
    yy = 66
    for k, (r, url) in enumerate(zip(rows, links), start=1):
        svg.text(LEFT, yy, "[%d]" % k, size=10.5, weight="bold")
        svg.text(LEFT + 36, yy, r["part"], size=10.5, weight="bold")
        svg.text(LEFT + 300, yy, url, size=9.5, color="#0563C1")
        svg.add("<a href='%s' target='_blank'><rect x='%g' y='%g' width='700' height='13' fill='#ffffff' fill-opacity='0'/></a>"
                % (url, LEFT + 300, yy - 10))
        yy += 21.5
    return pages


def write_pdf(pages):
    from playwright.sync_api import sync_playwright
    svgs = [p.render_a4(landscape=True, margin_mm=8.0) for p in pages]
    html = ("<html><head><style>@page{size:297mm 210mm;margin:0}html,body{margin:0;padding:0}"
            ".pg{page-break-after:always;width:297mm;height:210mm;overflow:hidden}svg{display:block}</style></head><body>%s</body></html>"
            % "".join("<div class='pg'>%s</div>" % s for s in svgs))
    with sync_playwright() as pw:
        browser = pw.chromium.launch()
        page = browser.new_page()
        page.set_content(html)
        page.pdf(path=str(PDF), width="297mm", height="210mm", print_background=True,
                 margin={"top": "0", "bottom": "0", "left": "0", "right": "0"})
        browser.close()
    print("Wrote %s (%d A4 landscape pages, 12-unit text = %.2f mm)" % (PDF, len(svgs), 12 * pages[0].mm_per_unit))


def main():
    rows, n_leds, n_strips = load_rows()
    write_csv(rows)
    print("Wrote", CSV)
    write_xlsx(rows)
    print("Wrote", XLSX)
    write_pdf(build_pages(rows, n_leds, n_strips))
    total, unpriced, insurance = pd_.totals(rows)
    print("Total %.2f EUR (+ insurance %.2f EUR), %d counted rows without a price" % (total, insurance, unpriced))


if __name__ == "__main__":
    main()
