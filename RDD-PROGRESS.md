# RDD-PROGRESS
Current status of every requirement with action points and evidence (RDD = Requirement Driven Development).
Statuses: planned, in progress, done, verified, changed, dropped.

| ID | Status | Evidence |
|----|--------|----------|
| REQ-001 | planned | - |
| REQ-002 | planned | - |
| REQ-003 | changed | - |
| REQ-004 | planned | - |
| REQ-005 | changed | - (the hex rotary switch is being tried in `hex_rotary_test/`; prepared 2026-11-10, host test only, not yet run on hardware, so nothing is verified) |
| REQ-006 | planned | - |
| REQ-007 | planned | - |
| REQ-008 | planned | - |
| REQ-009 | changed | - |
| REQ-010 | planned | - |
| REQ-011 | planned | - |
| REQ-012 | planned | - |
| REQ-013 | in progress | Host only (2026-11-11): V2 sketch reads the program switch D34-D37 (positions 0..E, F random), 15 letter programs and 15 frame patterns; `tests/casino_open_sign_v2_harness.cpp` with hard-coded tables from the independent `tests/frame_pattern_reference.py` passes, `tests/mutate_v2.py` all mutants detected. Not run on hardware or Wokwi; not verified. |
| REQ-014 | planned | - |
| REQ-015 | changed | - (menu JSON updated 2026-11-11, editor tests pass. 2026-11-11 host evidence for the C++ menu engine in `menu_test/`: `tests/menu_diff_test.py` 18006 random events, JS engine and C++ engine rows/callbacks/EEPROM batches identical; `tests/menu_engine_harness.cpp` checks pass (EEPROM image, texts, pins, debounce, sign of life); `tests/mutate_menu_engine.py` 99/99 mutants detected (equivalent mutants listed in the script). Not run on hardware; the V2 sketch does not contain the menu yet) |
| REQ-016 | planned | - |
| REQ-017 | done | 2026-11-11: `tests/menu_core_test.js` passes (V2 menu valid, loop and read-only quick-pick rejected, 16-character rows, EEPROM image), `tests/mutate_menu_core.py` 128/128 mutants detected, `tests/menu_editor_ui_test.py` passes (also for the single-file page; JSON round trip). Published as an artifact. Not yet verified by the maintainer. |

Action points are added per requirement when the maintainer asks for them.

## REQ-017 action points
- [x] AP-001 Menu JSON format and core logic (`tools/menu-editor/menu-core.js`) with host test and mutation test
- [x] AP-002 Browser editor with display preview and JSON export, UI test, published link
- [x] AP-003 Generator JSON -> PROGMEM tables and C++ menu engine (`tools/menu_engine/`, `tools/generate_menu_test.js`) with the hardware test `menu_test/` (evidence under REQ-017)

## REQ-012..016 action points
- [x] AP-007 (2026-11-11) V2 sketch: program switch D34-D37, 15 letter programs (5 new) and 15 frame patterns, mode button D33 only skips the LED test; host tests and mutation tests (evidence under REQ-013). SW3/SW4 speed switches, the LCD pages, brightness constants and the menu are still the old ones.
- [ ] AP-004 V2 sketch (rest): new controls, 15 letter programs and frame patterns, status page, menu, EEPROM (later, not started; the 5 new letter programs and 15 frame patterns are still to be proposed)
- [ ] AP-005 Test the LCD ROM arrow characters on the real display (part of running `menu_test/` on the hardware)
- [ ] AP-006 Run `menu_test/` on the hardware and write down the results (owner)
