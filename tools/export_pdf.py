#!/usr/bin/env python3
"""Convert the A4 SVG drawings of the project to A4 PDF files (needs Python Playwright + Chromium).

    python3 tools/export_pdf.py

Reads wiring-diagram.svg, layout-diagram.svg and layout-parts.svg from the project folder and
writes a PDF with the same name next to each of them. Print the PDFs at 100 % (no scaling).
"""

import re
from pathlib import Path

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parent.parent
FILES = ["wiring-diagram", "layout-diagram", "layout-parts", "strip_test/wiring-diagram", "strip_test/layout-diagram", "stick_test/wiring-diagram", "stick_test/layout-diagram", "stick_test_direct/wiring-diagram", "stick_test_direct/layout-diagram", "arrow_sticks/wiring-diagram", "arrow_sticks/layout-diagram", "strip_single_led_test/wiring-diagram", "strip_single_led_test/layout-diagram", "strip_knight_rider_test/wiring-diagram", "strip_knight_rider_test/layout-diagram", "strip_frame_demo_test/wiring-diagram", "strip_frame_demo_test/layout-diagram", "sticks_11_test/wiring-diagram", "sticks_11_test/layout-diagram", "casino_open_sign_v2/wiring-diagram", "casino_open_sign_v2/layout-diagram", "casino_open_sign_v2/layout-parts", "casino_open_sign_v2/parts-list"]


def main():
    with sync_playwright() as pw:
        browser = pw.chromium.launch()
        page = browser.new_page()
        for name in FILES:
            svg = (ROOT / (name + ".svg")).read_text()
            w, h = (float(v) for v in re.search(r"width='([\d.]+)mm' height='([\d.]+)mm'", svg).groups())
            page.set_content("<html><head><style>@page{size:%gmm %gmm;margin:0}"
                             "html,body{margin:0;padding:0}svg{display:block}</style></head><body>%s</body></html>"
                             % (w, h, svg))
            page.pdf(path=str(ROOT / (name + ".pdf")), width="%gmm" % w, height="%gmm" % h,
                     print_background=True, margin={"top": "0", "bottom": "0", "left": "0", "right": "0"})
            print("Wrote %s.pdf (%g x %g mm)" % (name, w, h))
        browser.close()


if __name__ == "__main__":
    main()
