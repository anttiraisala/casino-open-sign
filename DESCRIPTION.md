# Casino sign "CASINO OPEN" - project description

## What this is

An illuminated sign for a home poker casino, used when friends gather for poker nights. The
sign tells guests whether the casino is open and, with an arrow, which way to go.

The sign is controlled by an Arduino Mega 2560 and lit by WS2812B addressable LED strips. Before
anything is built, the whole sign is simulated in the Wokwi online simulator, so the program,
the strip lengths and the wiring can be tested on a screen first.

## What the sign looks like

- A black rectangular plate, 50 x 45 cm.
- An LED frame around the edge of the plate. A light pattern (5 LEDs lit, 5 dark) runs along
  the frame from one corner to the opposite corner. The direction follows the arrow.
- Red plexiglass letters, "CASINO" above "OPEN", floating a little in front of the plate.
  Behind each letter is a small group of LEDs (4 to 8 per letter).
- A hole text "CLOSED" cut into the plate, under the OPEN row. Six white LEDs behind the plate,
  with frosted plastic in between, light it up.
- A left/right arrow under that, about 40 x 15 cm: a rectangular shaft with a triangular head
  at each end. Only the head the arrow points at is lit. The light flows from the tail towards
  the tip in blue, cyan, violet and ice-white waves.

## What the sign does

- **Letters.** Eleven lighting modes, changed with a push button (after the last one it wraps
  to the first; the sign starts in mode 0):
  0 breathe, 1 wave, 2 typewriter, 3 ignite (like cold neon tubes), 4 word blink (CASINO steady,
  OPEN blinking), 5 alternate (odd and even letters breathe in opposite phase), 6 scanner (a bright
  spot with a tail sweeps back and forth), 7 sparkle (random twinkling), 8 chase (one dim letter
  runs through each word), 9 pulse (heartbeat) and 10 random auto (picks a random pattern from
  0..9 and changes to another one every 2 minutes).
  In every mode one random letter occasionally flickers like a faulty neon tube.
- **Arrow direction.** A slide switch selects left or right. The arrow, the head that is lit
  and the direction of the frame animation all follow it.
- **Casino closed.** A second slide switch tells the sign that the casino is closed. The OPEN
  letters then go dark and the CLOSED light turns on. It behaves like a faulty fluorescent tube:
  it flashes a few times, burns steadily for 5-14 seconds, flickers for about a second, and so on.
  The CASINO letters, the frame and the arrow keep working as normal.
- **Power-on LED test.** For 10 seconds after power-up every LED is checked: all strips at the
  same time, a short moving window of lit LEDs in each, first in white and then in red, green and
  blue (two passes). The display shows a progress bar and the seconds. The push button skips it.
- **Display.** A small 2x16 character LCD (I2C) shows what the sign is doing, in a debug style:
  the current letter mode, the positions and speeds of both speed switches (with the frame and
  arrow step times), whether the casino is open or closed and the arrow direction. The four pages
  rotate every 3 seconds; when something is changed, its page appears at once and stays for 6 seconds.
- **Speed.** Two 16-position rotary switches (hex coded, positions 0 to F) set the speed from
  4 times slower to 4 times faster: one for the frame and the arrow, one for the letter patterns.

## How it is built

- One Arduino Mega 2560, an I2C LCD (SDA D20, SCL D21), 10 separate LED strips (each on its own data pin), 238 LEDs in total,
  about 3.97 m of 60 LEDs per metre strip.
- A 5 V / 10 A power supply, a fuse, a 1000 uF capacitor and a 330 ohm resistor in each data line.
- `parts-list.pdf`, `.csv` and `.xlsx` are the complete shopping list (every component, quantity, price,
  stock status and a link to partco.fi); the Excel file calculates the sums with formulas. The
  LED strip was sold out at partco.fi when the list was made. The prices, stock and links are a
  snapshot from 2026-11-02 and are not updated unless the user explicitly asks for it.
- The ready-made drawings in this folder are meant to be printed on A4 paper:
  `wiring-diagram` (electrical schematic), `layout-diagram` (where each strip goes on the plate)
  and `layout-parts` (LED counts and cutting list).
- `README.txt` (and `README-fi.txt` in Finnish) contain the exact pin-by-pin wiring, the list of
  parts to buy from partco.fi and the instructions for opening the project in Wokwi.
- Everything is generated from the settings at the top of `sketch.ino`, so a different sign size
  only needs those numbers changed and the generator scripts rerun.

## Status

- The sketch, the Wokwi diagram, the drawings and the documents are ready and tested with a
  host-side simulation (not yet on real hardware).
- Wokwi cannot show the CLOSED hole text or the frosted plastic; a text label stands in for them.
- So far the project is the simulation, the documents and the parts list. The real sign is not built yet.

## LED strip under study

The 60 LEDs/m 5 V strip the sign was designed for is sold out at partco.fi, so a 12 V WS2811 strip
(30 LEDs/m, probably one control chip per 3 LEDs = 10 cm pixels) is being considered. The folder
`stick_test/` holds a separate test for 10 Grove RGB LED sticks (planned arrow, design B) powered from a separate 5 V supply; see stick_test/README.txt.

`strip_test/` holds a small program for testing a short piece of the strip before the sign is changed.
The sign's own files still describe the 5 V strip design.

## Planned and possible features

Nothing has been decided yet. Add ideas here when they come up in the conversation, for example:

- (empty - to be filled in as features are planned)

When a feature is implemented, move it from this list to "What the sign does" above.

Status 2026-11-06: stick_test_direct was run on real hardware and works with all 11 Grove sticks plugged unmodified into the Grove Base Shield and fed from an external 5 V supply. See README.txt (PROJECT STATUS).
