# RDD-PROGRESS
Current status of every requirement with action points and evidence (RDD = Requirement Driven Development).
Statuses: planned, in progress, done, verified, changed, dropped.

| ID | Status | Evidence |
|----|--------|----------|
| REQ-001 | planned | - |
| REQ-002 | planned | - |
| REQ-003 | changed | - |
| REQ-004 | planned | - |
| REQ-005 | done | 2026-11-11, host only (V2 sketch with the menu): `tests/casino_open_sign_v2_harness.cpp` drives the menu switch D42-D45 (relative steps 1..7 / 9..15, 8 ignored, SHIFT bigger step) and OK D48, BACK D49, SHIFT D50 through the sketch; `tests/menu_diff_test.py` 36006 events identical in JS and C++; `tests/mutate_v2.py` 171 mutants all detected (each also run in the boot modes with a saved, a wrong-tag and an out-of-range EEPROM image), `tests/mutate_menu_engine.py` 99/99. The hex switch itself works on the owner's hardware in `menu_test/` (owner's report). The V2 sketch is not run on hardware or Wokwi; not verified. |
| REQ-006 | planned | - |
| REQ-007 | done | 2026-11-11, host only (V2 sketch with the menu): row texts, quick-pick rows and scrolling long text checked by `tests/menu_engine_harness.cpp` and `tests/menu_diff_test.py`; V2 menu rows through `menuRows` in the V2 harness. Not run on hardware; not verified. |
| REQ-008 | planned | - |
| REQ-009 | changed | - |
| REQ-010 | planned | - |
| REQ-011 | planned | - |
| REQ-012 | done | 2026-11-11, host only (V2 sketch with the menu): V2 harness: Arrow reverse and Casino reverse invert the switches (also after a boot with the saved flag), D22-D29 and the mode button no longer read (D33 only skips the LED test), speeds and curves change only through properties. Not run on hardware or Wokwi; not verified. |
| REQ-013 | done | Host only (2026-11-11): V2 sketch reads the program switch D34-D37 (positions 0..E, F random), 15 letter programs and 15 frame patterns; `tests/casino_open_sign_v2_harness.cpp` with hard-coded tables from the independent `tests/frame_pattern_reference.py` passes, `tests/mutate_v2.py` all mutants detected.  Random delay property (Change delay, default 2 min, 0 = random 1..30) and the F countdown are checked in the V2 harness. Not run on hardware or Wokwi; not verified. |
| REQ-014 | done | 2026-11-11, host only (V2 sketch with the menu): V2 harness checks the status rows for the program, casino and arrow combinations, the ROM arrow characters 0x7F/0x7E and the F countdown `Change in M:SS`. The arrow characters on the real display are still unchecked (AP-005). Not verified. |
| REQ-015 | changed | 2026-11-11: menu JSON updated, editor tests pass. Host evidence: `tests/menu_diff_test.py` 36006 events, `tests/menu_engine_harness.cpp`, `tests/mutate_menu_engine.py` 99/99; the V2 sketch now contains the generated menu (`tools/generate_v2_menu.js --check` up to date) and the V2 harness checks it (speeds, curves, random delay, 60 s timer, EEPROM save and boot load). `menu_test/` ran on the owner's hardware (owner's report: all features worked). The V2 sketch is not run on hardware; not verified. |
| REQ-016 | done | 2026-11-11, host only (V2 sketch with the menu): `tests/menu_core_test.js`: a preset on a read-only property gives a warning and the target is dropped; `tests/mutate_menu_core.py` 129/129 mutants; V2 harness: Restore defaults sets every stored value, one callback batch, one EEPROM write. Not run on hardware; not verified. |
| REQ-017 | done | 2026-11-11: `tests/menu_core_test.js` passes (V2 menu valid, loop and read-only quick-pick rejected, 16-character rows, EEPROM image), `tests/mutate_menu_core.py` 128/128 mutants detected, `tests/menu_editor_ui_test.py` passes (also for the single-file page; JSON round trip). Published as an artifact. Not yet verified by the maintainer. |

Action points are added per requirement when the maintainer asks for them.

## REQ-017 action points
- [x] AP-001 Menu JSON format and core logic (`tools/menu-editor/menu-core.js`) with host test and mutation test
- [x] AP-002 Browser editor with display preview and JSON export, UI test, published link
- [x] AP-003 Generator JSON -> PROGMEM tables and C++ menu engine (`tools/menu_engine/`, `tools/generate_menu_test.js`) with the hardware test `menu_test/` (evidence under REQ-017)

## REQ-012..016 action points
- [x] AP-007 (2026-11-11) V2 sketch: program switch D34-D37, 15 letter programs (5 new) and 15 frame patterns, mode button D33 only skips the LED test; host tests and mutation tests (evidence under REQ-013). (The SW3/SW4 removal, status page and menu followed under AP-004.)
- [x] AP-004 (2026-11-11) V2 sketch: menu switch D42-D45, OK D48, BACK D49, SHIFT D50, generated menu tables (`tools/generate_v2_menu.js`), status page, EEPROM, speeds and brightness curves as menu properties, SW3/SW4 removed (host tests and mutation tests; evidence under REQ-005..016). Not run on hardware.
- [ ] AP-005 Test the LCD ROM arrow characters on the real display (part of running `menu_test/` on the hardware)
- [ ] AP-006 Run `menu_test/` on the hardware and write down the results (owner)
