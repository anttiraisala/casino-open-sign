# RDD-IDEAS
Raw ideas and open questions (RDD = Requirement Driven Development). An idea becomes a requirement only when the maintainer accepts it.

| ID | Idea | Status | Notes |
|----|------|--------|-------|
| IDEA-001 | Text (string) property type: a charset rolled forward with the rotary switch, SHIFT jumps to the next character group (upper case, lower case, digits, symbols); OK moves to the next character, BACK to the previous. | new | Maintainer: not needed yet. Check against the LCD character ROM when it is built. |
| IDEA-002 | Logarithmic scale for ranges that cross zero (a linear zone around zero). | new | AI proposal; a negative-only range can be handled by mirroring. |
| IDEA-003 | A separate `onCommit` callback that runs only after OK (heavier work), next to the per-step callback. | new | AI proposal. |
| IDEA-004 | A quick-pick row sets several properties at once (e.g. "Night mode" sets max 20 and min 0); callbacks shared by those properties run once. | new | AI proposal. |
| IDEA-005 | The generator prints a table of every menu row: shown text and what the row really does. | new | AI proposal, to catch misleading friendly texts. |
| IDEA-006 | After a quick-pick: short confirmation on the LCD, and a marker on the row whose value is currently active. | new | AI proposal. |
| IDEA-007 | A bound control that changes a value shows a short toast page on the LCD (name and new value). | new | AI proposal. |
| IDEA-008 | EEPROM write protection: a value changed quickly by a dial is saved only after it has stayed unchanged for about two seconds. | new | AI proposal; EEPROM lasts about 100 000 writes per cell. |
| IDEA-009 | The existing inputs of the sign (arrow switch, casino closed switch, mode button, speed switches) become bindings of the same mechanism. | new | AI proposal; changes working code, only on request. |
| IDEA-010 | An I2C I/O expander on D20/D21 if the Mega runs out of pins for controls. | new | AI proposal. |
| IDEA-011 | Read the hex rotary switch with only two legs (1 and 2) to save pins: gives the direction (mod 4) but not the absolute position, and a fast turn is ambiguous. The Mega has many free pins, so `hex_rotary_test/` uses all four legs. | new | AI proposal after the owner's question 2026-11-10; the hardware test decides. |

## Open questions
- OQ-001: Does a value change live while editing (callbacks on every step, BACK restores the old value)? Assumed yes.
- OQ-002: Wrap period: min and max count as two separate values (255 + 1 = 0, Z + 1 = A). Assumed yes.
- OQ-003: Default shift step: 16 times the normal step, at most one fifth of the number of steps, overridable per property (`shiftStep`). OK?
- OQ-004: Logarithmic scale: min > 0 required, negative-only ranges by mirroring, ranges crossing zero not supported (see IDEA-002). OK?
- OQ-005: Menu data: the JSON file is turned into tables by a generator at build time (recommended: checks at build time, no JSON parser on the Mega), or the JSON text is embedded in the sketch and parsed at start-up?
- OQ-006: Does the display return to the status pages after some inactivity (how long), and is an unfinished edit then cancelled?
- OQ-007: A rotary switch bound to a property: absolute or relative as the default, and at boot does the physical position win over the saved value?
- OQ-008: While a property is edited in the menu, controls bound to it are ignored (the menu wins). OK?
- OQ-009: Texts on the LCD are English under the repository rule; Finnish texts need an explicit exception.
- OQ-010: A row text longer than 16 characters: build error, or scrolling text?
- OQ-011: Which program gets the menu (Casino Open Sign V2 assumed), and which hex switches, buttons (OK, BACK, SHIFT) and pins are used?
- OQ-012: A quick-pick row that targets a read-only property: warning or error?
