# Menu Editor

Browser tool for the settings menu of Casino Open Sign V2 (requirements REQ-001..017 in `RDD-REQUIREMENTS.md`).
Open `index.html` (or the published single-file version): edit the menu tree, properties, types and presets, see the
2 x 16 character display preview, and export the JSON. Nothing is sent anywhere. The Arduino generator that turns the JSON into
PROGMEM tables is a later step (AP-003); the sketch does not use the menu yet.

## Files
- `menu-core.js` - validation of the JSON (format 1) and the menu engine (keys, editing, EEPROM image, rows). Pure logic, used by the page and by the tests.
- `app.js`, `index.html` - the editor page. `menu-v2-data.js` is generated from `menu-v2.json` by `tools/build_menu_editor_single.py`, which also builds `menu-editor-single.html` (everything inline, for the published artifact).
- `menu-v2.json` - the V2 menu (Controls, Speed with Frame/Letters/Arrow speed, a group each for Frame, Letters and Arrow with the speed and Min, Max, Exponent, Mix, Brightness with the same values grouped, Random program and Restore defaults; read-only values open a view page with a padlock, character code 1).
- Tests: `node tests/menu_core_test.js`, `python3 tests/mutate_menu_core.py` (127 mutants), `python3 tests/menu_editor_ui_test.py`.

## JSON format 1 (short)
`common.types` (templates), `common.properties` (id -> definition), `common.menus` (id -> `{name, items}`), `root` (`{name, items}`), `status` (property ids of program, casino and arrow, `randomValue` 15, `arrowAscii`).
Items: `{menu, items}`, `{menuRef}`, `{property: id, ...fields}` (inline), `{propertyRef}`, `{preset, set | setAll: "default", confirm, confirmText}`; any item may have `text` (row text).
Property fields: `kind` (int, float, choice, bool), `type` (template), `name`, `min`, `max`, `default`, `step` (int), `steps` and `scale` (float, linear or log), `wrap`, `shift` (x10 for numbers),
`readOnly`, `persist`, `bind` (note only), `unit`, `decimals`, `format` (hex), `valueTexts`, `choices`, `trueText`, `falseText`, `callbacks`.
Order of precedence: built-in default < type template (chain) < property.

## Behaviour (also in the preview)
Status page -> OK opens the menu at the root. Hex switch moves the selection (stops at both ends); OK enters a submenu or starts editing; while editing the value changes live (callbacks run), OK saves to EEPROM, BACK restores the value;
SHIFT multiplies the step. Presets apply all values first, then callbacks once per changed property, one EEPROM batch; a preset with `confirm` asks OK / BACK.
Turning the arrow, casino or program switch closes the menu and shows the status page. At the root level the menu closes after 60 s without input (the seconds are shown at the right edge of row 0).
EEPROM image: one slot per stored property plus a layout tag; a different layout or invalid data means all defaults.

## Notes
- In the published page the download button is not allowed by the viewer: use Copy JSON or the JSON tab and save the text yourself.
- Arrow characters on the real LCD (HD44780 ROM codes 0x7E and 0x7F) are still to be tested; `status.arrowAscii` selects `<-` / `->`.
