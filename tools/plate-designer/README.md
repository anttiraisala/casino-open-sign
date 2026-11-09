# Plate Designer

A single-page browser tool for the front plate of the CASINO OPEN sign: where the cable holes go, how the letters
are cut and how much LED strip fits around the edge. Open `index.html` in a browser (Chrome recommended); there is
nothing to install and nothing is sent anywhere, except that fonts are fetched from Google Fonts by name.

## What it does
- mm-accurate canvas, vinyl plate 1-1000 mm wide and high, default 960 x 300 mm (sliders + numeric fields).
- Boxes you drag: ARROW (fixed 349.76 x 102.32 mm, 10 cable holes of 8 mm from `tools/arrow_geometry.py`),
  CASINO, OPEN and CLOSED (default height half of CASINO). The plate centre (crossing marker and dash-dot centre lines, teal) is always drawn; box centre lines snap to the other boxes and the plate centre lines (3 mm), and dragged holes and LED strip pieces snap to the plate centre lines. Hold Ctrl while dragging to move without snapping.
- Every CASINO/OPEN letter cell has a round hole (one shared diameter, 5-30 mm, default 10 mm = 6 wires). Drag holes
  individually, or use "Centre holes in cells" / "Fit holes into thick parts". Hole outline is green when the whole
  circle is behind the letter, red when not.
- CLOSED has no per-letter holes: one round hole (shared diameter) and one slot (74 x 4 mm, the LED stick window);
  the export picks round / slot / none.
- Text per box (editable), font per box ("same font as CASINO" is on by default for OPEN and CLOSED). In the font box the
  up/down arrows switch the font live, Enter accepts, Esc puts the old font back. "Other Google font" takes any Google Fonts name.
- Spacing: uniform cells or natural widths. Letter thickening (grows or shrinks the outline), thin-part warning (minimum stroke, default 3 mm).
- Dimension arrows (mm) from the centre and corners of the selected object (and its hole centres) to the final plate edge; option "all" or "none".
- LED strip frame outside the plate (default) or, with the "LED strips inside the plate" checkbox in the side panel under Plate, on the plate against its edges (the strips then cover the outer 10 mm; counts are recalculated): 10 mm wide, 100 mm units with 3 LEDs each (coloured clusters), whole units per side, leftover mm and totals; layout "top and bottom over the corners" (default) or "sides over the corners".
- Optional 100 x 10 mm strip pieces you can drag behind the letters (R rotates, Delete removes, shows how much is behind a letter).
- Preview (black plate, red glowing letters, glowing LEDs), zoom/pan (wheel, drag background), grid and rulers.
- Every field has a tooltip that explains exactly what it does. All dropdowns work the same way: click opens the list, the arrow keys preview each choice live, Enter accepts, Esc goes back to the old value (typing a letter jumps to a matching entry).
- Number fields: drag the number sideways to change it (1 step per 2 px); dragging up/down is 4 times faster (up = more). A plain click lets you type.
- Handles on the selected text box (CASINO, OPEN, CLOSED): drag a corner or a side midpoint to scale the whole text uniformly (aspect ratio locked) around the centre of the box, which stays in place; the centre dot moves the box. The dragged handle snaps (6 px) to the edges and centre lines of the other boxes and the plate (pink guide line; "snap" option). The Height and Width fields scale the same way (cells, gaps and hole positions scale with the text); the cell and gap fields change the layout and also keep the centre.
- Keyboard: arrows nudge 1 mm, Shift 10 mm, Alt 0.1 mm. Ctrl+Z / Ctrl+Y undo/redo.

## Exports
- Drill pattern as EPS (1 unit = 1 mm, 5 mm margin; plate outline optional) and as DXF (R12 ASCII, mm, y up, layers PLATE / HOLES / SLOT: circles and closed polylines).
- Drill pattern as 1:1 tiled PDF, A4 or A3 landscape (print at 100 %, tape the tiles together; crosshair at every hole). The PDF also shows the letters and the arrow sticks in light grey with labels (letter numbers C1..O6, stick names with connectors such as L-CAP D2, CLOSED), so it doubles as a layout drawing: print it twice, drill through one copy and read the other. Option "letters, arrow and labels in the drill PDF" turns this off. Around every hole the PDF also writes the distance from the hole centre to all four plate edges (mm, rounded to 0.1, with thin dotted green lines to the edges; for the CLOSED slot the distances to its edges); option "distances to all plate edges around every hole (PDF)". The EPS and DXF files have no text or dimensions.
- Several holes in one letter (O, A, ...): under each CASINO/OPEN text, "Add a hole to" has a button per letter (adds a hole at the roomiest spot, away from the others); or double-click inside a letter to add one exactly there. Extra holes are drawn dashed and blue-tinted, can be dragged, removed with Delete, the x in the list, or by double-clicking them. They are counted in the "covered by the letters" check, scale with the text, and go into the drill EPS/DXF/PDF (PDF labels C2b, C2c, ...). Saved in the JSON as `texts.*.extraHoles` [{i: letter index, dx, dy from the cell centre}].
- Letter cut as EPS: as placed on the plate or nested on a raw sheet, optional mirror (cut from the back), laser kerf, dashed bounding boxes, optional plate edge as a cut line (EPS, and DXF layer PLATE_CUT; nested as its own piece when nesting). No overlap warnings are shown.
  Letter cut also as DXF (layers LETTERS, BOXES; one closed polyline per outline). DXF is untested in a real cutter program. Letters are written as outlines at 0.1 mm resolution; inner contours (holes of A, O, D, P) are included.

## Saving
- Autosave in the browser (localStorage), named layouts, undo/redo.
- JSON textarea shows the layout live. "Copy JSON from app" puts it on the clipboard; "Paste clipboard into app" loads a layout from the clipboard (if the browser blocks reading the clipboard, press Ctrl+V anywhere on the page outside the text fields). "Apply edited JSON" loads hand-edited text. Fonts are stored by name only, no font files.

## Use it online (GitHub Pages)
Address: https://anttiraisala.github.io/casino-open-sign/tools/plate-designer/ (the root address https://anttiraisala.github.io/casino-open-sign/ redirects here). It is the same `index.html` + `app.js` + `plate-core.js` + `arrow-data.js` as in this folder, served as a static page; nothing to build. Unlike the published Claude artifact, file downloads (EPS, DXF, PDF) work there, and Google Fonts load normally.

One-time setup (the repository owner, in a browser; the AI cannot do this):
1. The repository must be public (GitHub Pages on a private repository needs a paid GitHub plan). Settings > General > Danger Zone > Change visibility, only if you want it public: the whole repository, including the sign files, becomes readable by everyone.
2. Open https://github.com/anttiraisala/casino-open-sign/settings/pages
3. Under "Build and deployment" choose Source: "Deploy from a branch", Branch: `master`, Folder: `/ (root)`, press Save.
4. Wait about a minute; the page shows "Your site is live at https://anttiraisala.github.io/casino-open-sign/". Open it. Later pushes to `master` update the site automatically (about a minute).
Notes: layouts saved in the browser (localStorage) belong to that address; copy the JSON out if you want to move a layout to another browser. If the page shows 404 right after saving, wait and reload. A custom domain is possible later (Settings > Pages) but is not needed.

## Published copy
`python3 tools/build_plate_designer_single.py` writes `plate-designer-single.html` (everything inlined); it is published as the artifact "Plate Designer - CASINO OPEN front plate" (slug f2576151-ab61-4c1b-b072-788b0609f3cd). Downloads are blocked inside the artifact, so every export is also shown as text in the Export section (Copy button). Republish it to the same slug after changing the tool.

## Files
- `index.html`, `app.js` - the user interface.
- `plate-core.js` - pure logic (letter geometry, hole checks, snapping, frame counts, EPS/PDF writers, JSON); tested in `tests/plate_designer_core_test.js` (+ mutation test `tests/mutate_plate_designer.py`).
- `arrow-data.js` - generated by `python3 tools/generate_plate_designer_arrow.py` from `tools/arrow_geometry.py` (single geometry source). Regenerate after changing the arrow.
- Browser test: `python3 tests/plate_designer_ui_test.py` (Playwright; uses a local font because Google Fonts is not reachable from the build sandbox).

## How the letters are measured
Each letter is drawn on a canvas at 4 px/mm (10 px/mm for the exports), turned into a distance field and traced into outlines.
Thickening is an exact shift of that field. The size of a box is the cap height (height of "H"), so descenders and swashes can stick out of the box.
"Covered by the letter" and "thin part" are checks on that raster, accurate to about 0.25 mm at screen resolution.

## Not verified
- Fonts from Google Fonts could not be tested from the sandbox (no network); only local fonts were used in the tests. Wrong names show a warning and the browser default font.
- The thin-part and hole checks are approximations (see above). Check the printed drill template on the real plate and sticks before drilling or cutting.
- The 74 x 4 mm CLOSED slot is an assumption (the LED window of the stick); the 8 mm arrow cable hole is a guess.
- Changing the plate width or height resizes it around its centre: both opposite edges move and everything on the plate (texts, arrow, strip pieces) shifts by half the change, so the layout keeps its place relative to the plate centre.
- Selecting CASINO, OPEN or CLOSED from the drawing (click or start of a drag) flashes a gold frame for half a second around the text box and its panel section; the panel scrolls only if the section is not fully visible, then so that it is fully visible with at least 1/5 of the panel height above and below (when it fits). Already selected text and the arrow: no flash.
