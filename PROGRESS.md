# PROGRESS.md - status (2026-11-07)

## Done
- 2026-11-11: Casino Open Sign V2: program switch SW6 (hex rotary D34-D37, read all the time), 15 letter programs (new: converge, words swap, slot machine, twin chase, marquee; F = random every 2 min) and 15 frame patterns, D33 now only skips the LED test (IDEA-015, IDEA-016, OQ-013; REQ-013 in progress, AP-007). Wiring diagram, Wokwi diagram.json (second DIP bank) and READMEs updated. Run: V2 harness, mutate_v2.py (all detected), arduino_signature_lint. Host only, not run on hardware or Wokwi. `menu_test/` unchanged.
- Sign sketch (`sketch.ino`) for the first design: 10 WS2812B strips (pins D4-D12, D31), inputs D2, D3, D30, D22-D29, I2C LCD. Simulated in Wokwi by the owner earlier.
- Drawings, parts list (snapshot 2026-11-02), host test harnesses (all four pass on a host compiler with mock libraries).
- `strip_test/` (WS2812B strip test), `stick_test/` (sticks on a separate 5 V supply, modified cables), `strip_single_led_test/` (walking pixel on three 12 V WS2811 strips, not yet run on hardware), `sticks_11_test/` (10 arrow sticks + CLOSED light stick on A3, heat test, not yet run on hardware), `strip_knight_rider_test/` (bouncing light on three 50-pixel strips, not yet run on hardware), `strip_frame_demo_test/` (18-pixel frame strip, route from the upper-left corner clockwise, with three bouncing colour lights + single pulsing pixel + 6-pixel colour bar + 10-pixel pixel test (D12), not yet run on hardware), `stick_test_direct/` (11 sticks in the Grove shield, moving patterns, sign of life).

- `arrow_sticks/`: exact 1:1 drill template (A3 PDF, or two A4 pages with half an arrow on each), indicative layout and wiring for the 10-stick arrow with named sticks; dimensions checked by `tests/arrow_geometry_test.py` (gaps, rails on LED 3 and 8, size, symmetry; mutation-tested).

## Tested
- `stick_test_direct`: run on real hardware by the owner. All 11 Grove RGB LED sticks plugged unmodified into the Grove Base Shield (D2-D8, A0-A3), Arduino + shield fed from an external 5 V supply, every stick shows its patterns. The load test (button held, all LEDs white) was not reported separately; heat and voltage drop at full load are not confirmed.
- Host harnesses: logic only (mock libraries), no AVR build, no Wokwi run by the AI.

## Not tested
- `strip_test` and `stick_test` on real hardware; the sign sketch on real hardware.

## Ball/dot package DONE (2026-11-08)
- Dot = ball of the real stick (left half left, right half right, caps up); STICK_BALL_AT_DIN_END bit in sketch.ino (+ Wokwi generator, harness variant with the bit false, 42 mutation checks); LCD shows customer view "<-Customer right"/"->Customer left"; switch D32 mounting rule (back view, GND contact on the right) in README/schematic.

## Brightness curves DONE (2026-11-08)
- Per-LED-type lookup tables (strip, stick), defaults 0/255/1.97/1.0/keepZero, setBrightnessCurve() rebuilds a table; harness + 24 new mutants. Planned next (needs "toteuta"): menu with 4 rotary encoders, 3 buttons and EEPROM storage.

- Curve defaults changed to 0/80/2.67/1.0/true; global BRIGHTNESS removed (curve max is the limit); phone tool in tools/brightness-tool/ with its defaults test.

## Plate Designer DONE (2026-11-09)
- `tools/plate-designer/`: browser tool for the front plate (boxes, letter holes, CLOSED round/slot, dimension arrows, LED strip frame, strip pieces, fonts by name, thickening, thin-part warning, preview, undo, JSON, localStorage); exports drill EPS/DXF, tiled A4/A3 PDF, letter cut EPS/DXF (as placed / nested, mirror, kerf). Core logic has a node test with a mutation test (47 mutants); a Playwright test drives the UI with a local font. Not tested: real Google Fonts loading, real plate/laser.

## Plate Designer online (prepared 2026-11-09)
- Root `index.html` redirect + `.nojekyll` + README links for GitHub Pages (https://anttiraisala.github.io/casino-open-sign/). Not live until the owner enables Pages (steps in `tools/plate-designer/README.md`); not checked whether the repository is public.

## Planned / open
- Phase 1 DONE (2026-11-07; refined the same evening: tail cap middle 6 LEDs, head cap 3 + 3 end LEDs, upper/lower routes of 36 LEDs, alternating pulse; owner saw the first version run in Wokwi and said it works): sketch.ino + diagram.json use the 10 sticks (D2-D8, A0-A2) with a simple flowing arrow animation; frame D9/D10, letters D11, switch D32, button D33; CLOSED light changed from a 6-LED strip on D31 to the 11th Grove stick (10 LEDs, A3); sign of life in sketch.ino; old strip arrow in backup/old_arrow_strips/. Not yet run in Wokwi or built for the real Arduino by the AI.
- Phase 2 (after the owner asks): sign of life in strip_test and stick_test, old stick_test* layouts to 83x10 / 74x4 mm, richer arrow animations, 12 V WS2811 redesign, versioned releases, update the account skill ar-git-repo to v4.
- Check the drill template on real sticks before drilling: print at 100 %, lay a stick on it; confirm the LED pitch, Grove connector position, DIN end and the cable hole diameter (8 mm is a guess).
- Add the sign-of-life rule to `strip_test/`, `stick_test/` (done in `sketch.ino` and `stick_test_direct/`).
- Write the full arrow program (more patterns); phase 1 has only the flow animation.
- Open: second Grove connector on the sticks?, shield 5 V switch position (user says: 5 V), final stick count (11 work), whether the partco parts list needs a refresh (only on request).
- Plate Designer: options "LED strips inside the plate" (top bar) and "plate edge as a cut line in the letter cut" added, with tests (115 mutants detected).
- Plate Designer: strips inside the plate are drawn as translucent orange dashed ghosts over the plate; 100 mm LED groups alternate in tint with clear dividers (also outside).
- Plate Designer: several holes per letter (extraHoles): add buttons per letter, double-click add/remove, drag, Delete, drill exports, PDF labels, tests + mutants (132).
- Plate Designer: plate centre lines/marker drawn; holes and strips snap to them; Ctrl while dragging disables snapping.
- Plate Designer: plate width/height changes around the plate centre (resizePlate), 143 mutants.
- Plate Designer: plate size limits 1-1000 mm (width and height), default 960 x 300; default OPEN x 580, CLOSED x 540; 149 mutants.
- Plate Designer: gold selection flash + panel scroll into view for the text elements; 156 mutants.
- Plate Designer: right/middle button pans, rubber-band and Shift/Ctrl multi-selection of texts, group move/scale, "same height" option (sameHeight); 173 mutants.
- Plate Designer: frame material calculator (frameWidth 0-100, default 30; mitred bars, perimeter and material needed, frame page in the drill PDFs + "Frame PDF (A4)"); default plate 600 x 300 with new default text/arrow positions; 188 mutants.
- Plate Designer: EPS exports rewritten for Roland CutStudio (m/l/c only, pt, y up, extra 0 0 m); the plate is centred in the Export sheet w x h; 208 mutants.
- Plate Designer: strip pieces turn freely (rotation field, drag handle with Shift 15 degree snap, R = 90 degree step); strips JSON v2 (older rot 0/90 converted); 227 mutants.
- Plate Designer: arrow drill export = outlines of the 10 LED sticks (no round holes; DXF layer ARROW); new option mirror image of the drill pattern (EPS/DXF/PDF, default off); 240 mutants.
- Plate Designer: the arrow drill export is the 74 x 4 mm LED window of each stick (size from the LED window fields shared with the CLOSED slot, written whatever the CLOSED mode); 247 mutants.
- Casino Open Sign V2 (`casino_open_sign_v2/`, 2026-11-10): sketch (frame 18 px on D11, letters 10 px on D12 with OPEN reversed, two-way frame light from the upper corners), Wokwi diagram, schematic with two supplies (5 V + 12 V), layout, layout-parts, parts list (CSV/PDF), README EN/FI, host harness + ~85 mutants all detected. Stick shield connectors re-wired for V2 only (2026-11-10): L-WING-UP D6, L-WING-DN D5, RAIL-UP-L D7, RAIL-UP-R D8, RAIL-DN-L D3, RAIL-DN-R D4, R-CAP A3, CLOSED stick A0 (L-CAP D2, R-WING-UP A1, R-WING-DN A2 as before). V2 boot I2C scan prints the found devices to Serial (2026-11-10). Not run on hardware or Wokwi; open: owner to confirm the 5 V + 12 V power plan, frame bar width/speed, colour order of the 12 V strips.
- 2026-11-10: RDD taken into use (RDD-REQUIREMENTS.md, RDD-IDEAS.md, RDD-PROGRESS.md, rules in CLAUDE.md); the planned settings menu is recorded as REQ-001..011, ideas IDEA-001..010 and open questions OQ-001..012. Documents only; nothing implemented, no code run.
- 2026-11-10: `hex_rotary_test/` added (hex rotary switch test: legs 1, 2, 4, 8 on D34-D37, C to GND; number A 2..51 on the I2C LCD; direction from the position change; LCD retry every second and re-initialisation every 5 s because the owner saw "light but no text" on the real LCDs with the Arduino on an external supply). Host test + 108 mutants pass; drawings SVG+PDF, README.txt/README-fi.txt, Wokwi diagram (DIP bank stands in for the switch). Not run on hardware or Wokwi.
- 2026-11-11: RDD-IDEAS.md: quick-pick rows that set several values (IDEA-004 extended: max 16 values, hidden properties allowed), no active-value marker (IDEA-006 narrowed), new IDEA-012 (`default` value), IDEA-013 (generator validation), IDEA-014 (one EEPROM batch). Ideas only; no requirement changed, nothing implemented.
- 2026-11-11: RDD: V2 controls and settings-menu decisions recorded as REQ-012..017; REQ-003, REQ-005, REQ-009 revised (status changed); action points AP-001..005. Documents only; nothing run.
- 2026-11-11: Menu Editor added (`tools/menu-editor/`: `menu-core.js` validation + engine, browser editor with 2 x 16 preview and JSON export, `menu-v2.json`, `tools/build_menu_editor_single.py`); published as an artifact (slug 840aef6c-fb14-4ea5-9587-4e52d8c076dd). `tests/menu_core_test.js`, `tests/mutate_menu_core.py` (127 mutants), `tests/menu_editor_ui_test.py` all pass. REQ-017 done (AP-001, AP-002). No Arduino sketch changed.
- 2026-11-11: Menu Editor: brightness menu now Strip (frame strip), Letters, Stick, each Min, Max, Exponent, Mix (mix float linear 0..1, default 1.0); REQ-015 revised (changed). Tests and 127 mutants pass; editor artifact republished. No sketch changed.
- 2026-11-11: Menu engine and hardware test: `tools/menu_engine/` (C++ engine + hardware shell + table generator), `tools/generate_menu_test.js`, `menu_test/` (sketch, Wokwi diagram, wiring and layout drawings SVG+PDF, README.txt/README-fi.txt), `tools/generate_menu_test_drawings.py`; menu editor loaded with the owner's JSON and republished; AP-003 done, AP-006 added; REQ-015 evidence (host only). Tests run: menu_core_test, mutate_menu_core 128/128, menu_editor_ui_test, menu_diff_test 18006 events 0 mismatches, menu_engine_harness, mutate_menu_engine 99/99, all older harnesses. The V2 sketch was NOT changed; not run on hardware.
- 2026-11-11: `menu_test/` compile fix: the Arduino IDE reported "'HexSwitch' does not name a type" (function signatures with user-defined structs break the automatic prototypes); `readHexRaw`, `pollHex`, `pollButton` now take an index; new `tests/arduino_signature_lint.py` catches this (it flags the old sketch); engine checks, JS/C++ difference test (12006 events, 0 mismatches) and 99 engine mutants pass again. Not compiled with the real Arduino toolchain here; the owner's compiler output was the evidence.
- 2026-11-11: Wiring diagrams of `menu_test/` and `hex_rotary_test/`: rotary switch legs 1, 2, 4, 8 and C now labelled large and bold on the switch symbol; rule added to CLAUDE.md. Owner ran `menu_test/` on the hardware: all features worked (menu navigation, entering submenus, changing values, leaving), no errors found (owner's report; REQ-015 stays `changed` until the maintainer verifies it in RDD-PROGRESS.md).
