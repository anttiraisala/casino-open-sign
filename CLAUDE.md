# CLAUDE.md - rules and decisions for AI assistants

Chat with the owner in the language he writes (Finnish). Everything in this repository is English
(code, comments, docs, config, tests, file and branch names, commit messages). The only exceptions: `README-fi.txt`
files (Finnish translation of the README.txt next to them) and product names that contain Finnish characters.

## Working rules (from the owner)

- Do not implement or change anything until the owner asks ("toteuta", "toimi"). Questions, ideas and
  "explain in your own words" are discussion only; remind him at the end of the turn that nothing is implemented.
- Flag contradictions in a request before building.
- Hardware/Arduino work always comes with: `README.txt` and `README-fi.txt` containing exact wiring (component to pin)
  and a parts-to-buy list from partco.fi; an SVG electrical schematic in circuit-diagram style (symbols).
- Deliver project files together in ONE zip (the whole folder), sent as an attachment.
- Any test the AI asks the owner to run lives in its own sub-folder, self-contained (own layouts, wirings,
  `sketch.ino`, `diagram.json`, README and README-fi).
- Parts-list prices, stock and links are a snapshot (2026-11-02). Do not refresh them unless asked; remind the owner now and then.
- Sign of life rule for every Arduino sketch: the LED on pin 13 blinks while the program runs (shortly inverted on
  state changes, faster when many changes happen) and the Serial Monitor gets one line per second with changing values;
  the baud rate (115200) is stated at the top of the sketch, printed at start-up and written in the README. Skip the
  LED or serial output only if the project uses that pin or port for something else. Reference: `stick_test_direct/sketch.ino`.
  The sign sketch, `strip_test/` and `stick_test/` do not have it yet: add it when they are next updated.
- Arduino pitfalls: function signatures only with built-in types (automatic prototypes); NeoPixel objects must not be
  copied (named objects + a pointer array); non-blocking `millis()` timing.
- Host tests hard-code the expected values (not taken from the sketch constants) and are checked with mutation tests.

## Decisions

- Arrow design B (barbed head) with 10 Grove RGB LED sticks plus a spare 11th; the arrow is about 31 x 9 cm. Not applied to the sign sketch yet.
- Planned but not applied: 12 V WS2811 strips, 60 x 30 cm plate, versioned releases (current state = v1).
- Details: `AI_CONTEXT.md` (technical description, section 8 tests) and `DESCRIPTION.md`.
