LED STRIP TEST - find out what kind of strip you have
======================================================

Purpose
-------
The casino sign is planned for the 12 V addressable strip "RGB-LEDNAUHA 30kpl/m WS2811 5m"
(partco.fi, SKU LED-N-RGB-W2812X150-5M):
https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
The product page does not tell everything we need to know. Cut a short piece of the strip, run this
test and tell what you see. Then the main sketch can be made for exactly that strip.

What we want to find out
  1. The colour order (RGB, GRB, BRG, ...) and the bit speed (800 kHz or 400 kHz) the strip needs.
  2. Does one control chip drive 1 LED or 3 LEDs? (Probably 3: then a "pixel" is 10 cm long.)
  3. Does the strip work with the 5 V data signal of the Arduino without a level shifter?
  4. Anything else odd: flicker, dead pixels, the real width of the strip, the text printed on it.

Files
-----
sketch.ino         The test program. Settings are at the top.
diagram.json       Wokwi circuit for trying the sketch in the simulator (paste into the diagram.json tab).
libraries.txt      Library: Adafruit NeoPixel.
layout-diagram.pdf (piece and what to look for), wiring-diagram.svg/.pdf   The wiring as an electrical schematic (A4).
README.txt         This file (English).   README-fi.txt   The same in Finnish.

What you need for the test (real strip)
---------------------------------------
  - A short piece of the strip: 3 to 6 pieces of 10 cm (30 to 60 cm). Cut ONLY at the marked cut
    lines. Solder wires to the pads at the DIN end (+12 V, DIN, GND), or use clip leads.
  - A 12 V DC supply, 1 A or more (a plug-in adapter is the safe choice: no mains wiring). For a short piece
    (30-60 cm) at brightness 60 even the USB-to-12 V step-up converter ADA2778 (500 mA) is enough:
    https://www.partco.fi/fi/teholaehteet/dcdc-jaennitemuuntimet/step-up-dcdc-jaennitettae-nostava/23817-ada2778.html
    partco.fi has a screw-terminal adapter for a DC plug: DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA
    (SKU DCJ21RUU) https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - A 330 ohm resistor (R1) for the data line.
  - Optional: a push button (pin 3 to GND) and a 1000 uF capacitor rated 25 V or more across the strip power.
  - Jumper wires. The Arduino Mega is powered from USB.
  (These parts are not part of the sign's parts list; see ../parts-list.* for that.)

Wiring (exactly)
----------------
    Arduino pin D6    -> 330 ohm resistor -> DIN of the strip (the end the printed arrows point away from)
    12 V supply +     -> +12 V of the strip
    12 V supply -     -> GND of the strip AND the Arduino GND (common ground)
    Arduino D3        -> optional push button -> GND (next setting)
  The strip's +12 V must NOT go to the Arduino 5V pin, and no 12 V may go to any Arduino pin.
  See wiring-diagram.svg for the picture.

Running it
----------
Real hardware: open sketch.ino in the Arduino IDE (library: Adafruit NeoPixel), choose Arduino Mega 2560,
upload, open the Serial Monitor at 115200 baud.
Wokwi (simulator): create a new Arduino Mega project, paste sketch.ino and diagram.json, add the library
"Adafruit NeoPixel" (libraries.txt), press Play. The simulator shows an ideal strip (every pixel
individually), so it only shows that the program works; the real strip is what we want to learn about.

What the test does
------------------
It goes through 12 settings (6 colour orders x 2 bit speeds, 12 seconds each; the push button on D3 jumps to
the next one at once):
  setting  1: GRB, 800 kHz     setting  7: GRB, 400 kHz
  setting  2: RGB, 800 kHz     setting  8: RGB, 400 kHz
  setting  3: BRG, 800 kHz     setting  9: BRG, 400 kHz
  setting  4: RBG, 800 kHz     setting 10: RBG, 400 kHz
  setting  5: GBR, 800 kHz     setting 11: GBR, 400 kHz
  setting  6: BGR, 800 kHz     setting 12: BGR, 400 kHz
In every setting:
  - The first 3 seconds: pixel 1 should be RED, pixel 2 GREEN, pixel 3 BLUE.
  - After that: one WHITE pixel walks along the strip, one step every half second.
  - The built-in LED (pin 13) blinks the number of the setting at the start; the Serial Monitor shows the
    number and the colour order.
The settings with the wrong colour order show the wrong colours (for example green, red, blue). A wrong speed
usually shows nothing, white or random colours, or flicker.

What to tell me (copy this and fill it in)
------------------------------------------
  1. Setting number(s) where pixels 1-3 are red, green, blue in that order: ______
  2. While the white pixel walks: how many LEDs light up at once (1 or 3)? ______
  3. Do all the pixels of the piece work (also the last one)? Any flicker? ______
  4. The width of the strip and the text printed on it or on the reel (brand, chip, voltage): ______
  5. Which end did you connect as DIN (does it match the printed arrows)? ______
  6. Anything else you noticed (colours too dim, a pixel stays on, nothing happens): ______

If nothing lights up: check the DIN end (the arrows), the common GND, the 12 V supply, and try a level
shifter (74HCT125) only if everything else is right.

Changing the settings
---------------------
At the top of sketch.ino: PIN_STRIP_DATA (6), PIN_BUTTON (3), TEST_PIXELS (12), BRIGHTNESS (60),
PHASE_MS (12000), AUTO_ADVANCE (false = change setting only with the button), COLOUR_TEST_MS, WALK_STEP_MS.

Tests
-----
tests/strip_test_harness.cpp (in the project's tests folder) checks the program on the computer:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/strip_test_harness tests/strip_test_harness.cpp && /tmp/strip_test_harness
(It was not run on real hardware or in Wokwi.)
