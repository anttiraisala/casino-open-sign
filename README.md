# casino-open-sign

Arduino Mega LED sign that says CASINO OPEN for a home poker room, with animated light effects.

## Plate Designer online

**Use the front plate editor directly in the browser: https://anttiraisala.github.io/casino-open-sign/** (opens `tools/plate-designer/`; direct link: https://anttiraisala.github.io/casino-open-sign/tools/plate-designer/).
It is a static page served by GitHub Pages straight from this repository (branch `master`, folder `/`), so every push updates it. The page works once GitHub Pages is switched on for the repository (Settings > Pages, see `tools/plate-designer/README.md`). The same tool is also published as a Claude artifact; the GitHub Pages copy is the one where file downloads (EPS, DXF, PDF) work.

## For use with AI

- Clone: `git clone https://github.com/anttiraisala/casino-open-sign.git` (default branch: `master`).
- Rules and decisions for AI assistants: [`CLAUDE.md`](CLAUDE.md). Current status, test notes and open questions: [`PROGRESS.md`](PROGRESS.md).
- Technical description for continuing the project: [`AI_CONTEXT.md`](AI_CONTEXT.md). What the project is for: [`DESCRIPTION.md`](DESCRIPTION.md).

## What is here

- `sketch.ino`, `diagram.json`: the sign program for an Arduino Mega 2560 and its Wokwi simulation (WS2812B LED strips).
- `wiring-diagram`, `layout-diagram`, `layout-parts` (`.svg` / `.pdf`), `parts-list` (`.csv` / `.pdf` / `.xlsx`): electrical schematic, layout of the strips on the plate, parts list from partco.fi (a snapshot of 2026-11-02).
- `strip_test/`, `stick_test/`, `stick_test_direct/`: separate hardware tests, each with its own sketch, Wokwi diagram, schematic, layout and README. `stick_test_direct` has been run on real hardware: 11 Grove RGB LED sticks in the Grove Base Shield work.
- `strip_frame_demo_test/`: 18-pixel frame strip on D9 with three differently fast bouncing colour lights (colours add), a pulsing single pixel on D10 and a moving 6-pixel colour bar on D11 (not yet run on hardware).
- `strip_single_led_test/`, `strip_knight_rider_test/`, `sticks_11_test/`: newer hardware tests, not yet run on real hardware: one pixel walking along three 12 V WS2811 strips (counts the real pixels, checks the colour order); a knight-rider light bouncing on three 50-pixel strips; and the 10 arrow sticks plus the CLOSED light stick (A3) kept lit to see whether anything gets hot.
- `arrow_sticks/`: the planned arrow made of 10 Grove RGB LED sticks (design B): exact 1:1 drill template (A3 PDF, or two A4 pages with half an arrow on each), indicative layout and wiring, all from one geometry file (`tools/arrow_geometry.py`). The sign sketch now drives it (phase 1: a simple flowing arrow animation; see `AI_CONTEXT.md`).
- `backup/old_arrow_strips/`: the previous sign with the arrow made of six LED strips, kept as a backup list; not wired any more.
- `tools/plate-designer/`: Plate Designer, a browser tool for the front plate (drill pattern, letter cut, LED strip frame); open `index.html`, or use it online at https://anttiraisala.github.io/casino-open-sign/ (see its README.md). `index.html` in the repository root only redirects there (needed for GitHub Pages); `.nojekyll` tells GitHub Pages to serve the files as they are.
- `tests/`: host-side test harnesses with mock Arduino libraries (no AVR toolchain needed). `tools/`: Python generators for the drawings, `diagram.json` files and PDFs.
- `README.txt` / `README-fi.txt`: the full hardware README (English / Finnish) with exact wiring and the parts-to-buy list.

## Run the host tests

```
g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH='"../sketch.ino"' -o /tmp/harness tests/harness.cpp && /tmp/harness
for t in strip_test stick_test stick_test_direct strip_single_led_test strip_knight_rider_test sticks_11_test strip_frame_demo_test; do g++ -std=c++17 -O1 -w -Itests -o /tmp/$t tests/${t}_harness.cpp && /tmp/$t; done
python3 tests/arrow_geometry_test.py
```

## License

MIT, see [`LICENSE`](LICENSE).
