# casino-open-sign

Arduino Mega LED sign that says CASINO OPEN for a home poker room, with animated light effects.

## For use with AI

- Clone: `git clone https://github.com/anttiraisala/casino-open-sign.git` (default branch: `master`).
- Rules and decisions for AI assistants: [`CLAUDE.md`](CLAUDE.md). Current status, test notes and open questions: [`PROGRESS.md`](PROGRESS.md).
- Technical description for continuing the project: [`AI_CONTEXT.md`](AI_CONTEXT.md). What the project is for: [`DESCRIPTION.md`](DESCRIPTION.md).

## What is here

- `sketch.ino`, `diagram.json`: the sign program for an Arduino Mega 2560 and its Wokwi simulation (WS2812B LED strips).
- `wiring-diagram`, `layout-diagram`, `layout-parts` (`.svg` / `.pdf`), `parts-list` (`.csv` / `.pdf` / `.xlsx`): electrical schematic, layout of the strips on the plate, parts list from partco.fi (a snapshot of 2026-11-02).
- `strip_test/`, `stick_test/`, `stick_test_direct/`: separate hardware tests, each with its own sketch, Wokwi diagram, schematic, layout and README. `stick_test_direct` has been run on real hardware: 11 Grove RGB LED sticks in the Grove Base Shield work.
- `tests/`: host-side test harnesses with mock Arduino libraries (no AVR toolchain needed). `tools/`: Python generators for the drawings, `diagram.json` files and PDFs.
- `README.txt` / `README-fi.txt`: the full hardware README (English / Finnish) with exact wiring and the parts-to-buy list.

## Run the host tests

```
g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH='"../sketch.ino"' -o /tmp/harness tests/harness.cpp && /tmp/harness
for t in strip_test stick_test stick_test_direct; do g++ -std=c++17 -O1 -w -Itests -o /tmp/$t tests/${t}_harness.cpp && /tmp/$t; done
```

## License

MIT, see [`LICENSE`](LICENSE).
