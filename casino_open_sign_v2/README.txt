CASINO OPEN SIGN V2 - Arduino Mega 2560, two 12 V WS2811 strips + 11 Grove RGB LED sticks
=========================================================================================

Version 2 of the sign program. Version 1 (the sketch.ino in the folder above) stays untouched.
V2 has exactly TWO LED strips and ELEVEN sticks:

  Strip 1  D11  frame     12 V WS2811, 18 pixels (one pixel = one chip = 3 LEDs = 10 cm), around the 60 x 30 cm plate
  Strip 2  D12  letters   12 V WS2811, 10 pixels, one pixel behind each letter
  Sticks   Grove Base Shield D2-D8, A1-A3: the 10 arrow sticks (V2 wiring, see below), A0: the CLOSED light stick
  D13 is the status LED (sign of life), as before.

Not run on real hardware or in Wokwi yet. The host tests pass (see Tests).

Files
-----
sketch.ino                 The program (settings at the top). Serial Monitor: 115200 baud.
diagram.json               Wokwi circuit (paste into the diagram.json tab; the 12 V strips are simulated from the Mega 5V).
libraries.txt              Library: Adafruit NeoPixel (Wire is built in).
wiring-diagram.svg/.pdf    Electrical schematic, A4 portrait (two supplies, fuses, capacitors, all pins).
layout-diagram.svg/.pdf    Layout of the plate, A4 landscape: frame pixels 1-18, letter pixels, CLOSED, arrow (to scale).
layout-parts.svg/.pdf      Strips, pixel numbers, letter table, stick table, cutting list (A4 portrait).
parts-list.csv/.pdf        Parts to buy (partco.fi snapshot of 2026-11-02; prices/stock/links are NOT refreshed).
README.txt / README-fi.txt This file (English / Finnish).
The arrow (drill template, stick drawings, stick wiring) is unchanged: see ../arrow_sticks/.

Frame strip (D11): 18 pixels, front view
----------------------------------------
Pixel 1 is in the LOWER-LEFT corner; the strip runs counter-clockwise:
    pixels  1-6    bottom edge, left to right (6 = lower-right corner)
    pixels  7-9    right edge, going up
    pixels 10-15   top edge, right to left (10 = upper-right corner, 15 = upper-left corner)
    pixels 16-18   left edge, going down
Frame light (a bar of 2 lit pixels, 2 dark, repeating; one step every 200 ms at the default speed x1; menu: Speed > Frame speed):
    arrow points LEFT (the customer's left):  starts at the UPPER-RIGHT corner (pixel 10) and runs both ways round the frame -
                                              clockwise (10, 9, 8, 7, 6, 5 ... 1) and counter-clockwise (10, 11 ... 15, 16, 17, 18, 1) -
                                              to the LOWER-LEFT corner (pixel 1).
    arrow points RIGHT (the customer's right): starts at the UPPER-LEFT corner (pixel 15) and runs both ways to the
                                              LOWER-RIGHT corner (pixel 6).
Both paths are 9 steps long and move exactly together (no time offset); they meet at the end corner.

Letters strip (D12): 10 pixels, one per letter
----------------------------------------------
    pixels 1-6    C A S I N O    (CASINO, left to right)
    pixels 7-10   N E P O        (OPEN is wired REVERSED: pixel 7 = N ... pixel 10 = O)
The program maps the letters, so every letter effect runs in natural reading order for the viewer:
the letters of OPEN, read left to right O P E N, are pixels 10, 9, 8, 7. A pixel is 3 LEDs in one colour (deep red).

What stays as in version 1
--------------------------
Arrow animation, CLOSED light (faulty fluorescent tube), the switches (D32 arrow direction,
D30 casino closed), the LCD (I2C, SDA D20, SCL D21), the
brightness curves (per LED type, max 80, exponent 2.67; now menu properties), the power-on LED test (10 s; window now 2 pixels), the sign of life
(LED on pin 13 blinks; one Serial line per second at 115200 baud).

Wiring (exactly)
----------------
    Arduino D11   -> 330 ohm resistor R1 -> DIN of the frame strip (pixel 1, the lower-left corner end)
    Arduino D12   -> 330 ohm resistor R2 -> DIN of the letters strip (pixel 1, behind the C)
    PSU2 12 V +   -> fuse F2 (5 A) -> +12 V of BOTH strips (feed the frame at the start and the end; the letters at the start)
    PSU2 12 V -   -> GND of both strips AND Arduino GND (common ground)
    Capacitor C2 1000 uF 25 V across +12 V and GND at the strips (plus to +12 V)
    PSU1 5 V +    -> fuse F1 (10 A) -> BOTH 5V pins of the Mega long header (the shield, the 11 sticks and the LCD run from them)
    PSU1 5 V -    -> Mega GND pins; capacitor C1 1000 uF across the 5 V rail (plus to +5 V)
    Grove Base Shield on the Mega (3V3/5V switch on 5 V): sticks D2 L-CAP, D6 L-WING-UP, D5 L-WING-DN, D7 RAIL-UP-L, D8 RAIL-UP-R,
                  D3 RAIL-DN-L, D4 RAIL-DN-R, A3 R-CAP, A1 R-WING-UP, A2 R-WING-DN, A0 CLOSED light stick (no wire from a supply to a stick)
                  (V2 only: the connectors differ from version 1 and ../arrow_sticks/; the stick positions on the plate are unchanged)
    D32 -> slide switch SW1 common (contact 1 -> GND)      D30 -> slide switch SW5 common (contact 1 -> GND)
    D42-D45 -> SW3 menu switch pins 1, 2, 4, 8 (common C -> GND)
    D48 -> push button SW7 OK -> GND     D49 -> push button SW8 CANCEL -> GND     D50 -> push button SW9 SHIFT -> GND
    (every button: contact 1 to the Arduino pin, contact 2 to GND; all inputs use INPUT_PULLUP, no resistors)
    The old speed switches SW3/SW4 (D22-D29) are gone: the speeds are menu properties. The menu pins are the same as in ../menu_test/.
    D20 SDA, D21 SCL, 5V, GND -> LCD I2C backpack
                  (at boot the sketch scans the I2C bus and prints every device found to the Serial Monitor, 115200 baud;
                  the sketch talks to the display at 0x27: if the scan shows e.g. 0x3F, set LCD_I2C_ADDRESS in sketch.ino)
    D13 -> nothing (the LED on the Mega board)
  NEVER connect +12 V to an Arduino pin or to the 5 V parts. USB only while PSU1 AND PSU2 are switched off.
  The 5 V data signal into a 12 V WS2811 strip normally works (as in the strip tests) but is not verified on the final sign.
  Mains side of both supplies: inside an enclosure, wired by a qualified person.
  See wiring-diagram.svg (schematic) and layout-diagram.svg (positions).

Parts to buy
------------
See parts-list.csv / parts-list.pdf (partco.fi, prices and stock are a snapshot of 2026-11-02 and are not refreshed: check them
before buying). Main items: Arduino Mega 2560, Grove Base Shield + 11 Grove RGB LED sticks, 5 V PSU Mean Well LRS-50-5 + 10 A fuse,
12 V PSU Mean Well LRS-50-12 + 5 A fuse, one reel of 12 V WS2811 strip (28 of its 50 pixels are used), 2 x 330 ohm, 2 x 1000 uF 25 V.

Running it
----------
Real hardware: open sketch.ino in the Arduino IDE (library: Adafruit NeoPixel), board Arduino Mega 2560, upload with PSU1 and PSU2 off,
then power up; Serial Monitor 115200 baud. Wokwi: new Arduino Mega project, paste sketch.ino and diagram.json, add the library
"Adafruit NeoPixel", press Play (Wokwi has no 12 V part: the strips are powered from the Mega 5V there).
If the colours are wrong, the strip's colour order may differ: change NEO_GRB + NEO_KHZ800 in the two strip lines of the sketch.
The power-on LED test (10 s) lights a 2-pixel window running along every strip (pass 1 white, pass 2 red-green-blue); the CANCEL button (D49) skips it (the LCD scrolls "press CANCEL to skip" and then says "Test skipped" for a moment; a CANCEL held down at power-up does not skip it). D33 is free.

Settings menu (menu switch SW3 D42-D45, buttons OK D48, CANCEL D49, SHIFT D50)
--------------------------------------------------------------------------
Status page (the display after power-up): row 0 shows the two effects and the casino state, e.g. "F12 L03 Open" (F = frame effect, L = letters
effect, two digits; a star before the number marks a random group and the number is the effect really showing, e.g. "F*07"; "--" = unknown);
row 1 shows the arrow symbol and the customer side ("<- Customer right" = the arrow points to the customer's right) and, while the frame
or the letters are random, alternates with "Change in M:SS" (time to the next random change). OK opens the menu; the display shows the LCD button name OK.
Menu: Controls (arrow, casino: read-only switch positions; Arrow reverse / Casino reverse flags), Effects (Frame effect, Letters effect, Random delay), Speed (Frame speed, Letters speed,
Arrow speed: x0.125..x4, logarithmic, 30 steps, shown with 3 decimals, default x1), then one group per LED type: Frame (the frame LED strip),
Letters (the letter LED strip) and Arrow (the arrow sticks), each with its speed (Frame and Letters also start with their effect: one value, two places) and the brightness Min, Max, Exponent and Mix (the curve max is the
limit, there is no global brightness); Brightness (the same brightness values again, grouped Frame / Letters / Arrow: one property, two places),
Restore defaults (asks for confirmation).
A property row shows "Name  value" (value at the right edge); if the two do not fit in 16 characters the row scrolls as one text "Name: value"
(pause at the start, one character per step, pause with the last character at the right edge). OK on a read-only value opens the same page as
an edit (name, then a padlock and the value) but nothing can be changed; CANCEL or OK returns.
The menu switch SW3 is free-running and RELATIVE: the change of its code is a step (1..7 forward, 9..15 back, exactly 8 ignored).
The menu definition used for this sketch is stored here as menu-v2.json (an exact copy of tools/menu-editor/menu-v2.json, written by
tools/generate_v2_menu.js together with the menu block of sketch.ino; open it in the Menu Editor to see or change the menu).
A move of the menu switch at the first or last item does nothing (the scroll of a long row goes on); a move away and straight back
within 300 ms (contact bounce) also keeps the scroll of that row.
Holding SHIFT while turning makes the step bigger. OK enters / starts an edit / accepts, CANCEL goes one level up / cancels an edit.
Long row text scrolls. At the menu root, with no property open and no edit, 60 s without any input returns the display to the status page.
There is no program switch: the effects are chosen in the menu (Effects).
Values are saved in EEPROM (layout tag checked at start-up; a layout that does not match gives the defaults).
Tested on the host only (tests/casino_open_sign_v2_harness.cpp, the menu engine tests), not yet on hardware or in Wokwi.
Known limitation: the LCD is retried after a failed transmit, but not fully re-initialised periodically as in hex_rotary_test.

Effects (menu: Effects > Frame effect, Letters effect, Random delay)
----------------------------------------------------------------------
The frame and the letters each have their own effect, 0..15: 0 = Random, 1..15 = a fixed effect (default 1; no wrap-around; the value row
shows the number first, e.g. "12 Slot machine"). While you turn the value the effect is shown live; OK accepts and saves it, CANCEL restores the
old value and the old effect.
  1 breathe  2 wave  3 typewriter  4 ignite  5 word blink  6 alternate  7 scanner  8 sparkle  9 chase  10 pulse
  11 converge  12 words swap  13 slot machine  14 twin chase  15 marquee
Random: one shared timer, its length is the Random delay (minutes; 0 = a random 1..30 minutes; default 2). When it runs out, only the groups set
to 0 pick a new effect (each its own, never the same twice in a row); a fixed effect never changes by itself. Setting a group to 0 picks an
effect at once and restarts the shared timer, so random groups always change at the same moment.
The frame patterns are white and follow the arrow's start corner in effects 1, 2, 3, 7, 9, 14 and 15. Both are first versions to be
judged live on the sign (RDD-IDEAS.md IDEA-015, IDEA-016). Tested on the host only, not yet on hardware.

Changing the settings
---------------------
At the top of sketch.ino: FRAME_EDGE_LONG (6) and FRAME_EDGE_SHORT (3) (frame pixels per edge), PIN_FRAME (11), PIN_LETTERS (12),
FRAME_SPEED_MS (200), FRAME_BAR_LEDS (2), STARTUP_TEST_WINDOW_LEDS (2), the brightness curve defaults, and the letter/arrow timings.
After changing pins or the frame size regenerate the drawings and Wokwi file:
    python3 tools/generate_v2_diagram.py && python3 tools/generate_v2_schematic.py && python3 tools/generate_v2_layout.py
    python3 tools/generate_v2_parts_list.py && python3 tools/export_pdf.py

Tests
-----
tests/casino_open_sign_v2_harness.cpp checks the program on the computer (expected values are hard-coded, not read from the sketch),
tests/mutate_v2.py checks the tests with about 196 mutants of the sketch:
    g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH='"'$PWD'/casino_open_sign_v2/sketch.ino"' -o /tmp/v2 tests/casino_open_sign_v2_harness.cpp && /tmp/v2
    python3 tests/mutate_v2.py
(Not run on real hardware or in Wokwi.)
