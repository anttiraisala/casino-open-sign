LED STRIP SINGLE LED - walk one pixel along the whole strip (three tests at once)
=================================================================================

Purpose
-------
Find out how many pixels (control chips) the 12 V WS2811 strip really has, and check the colour order.
The strip (5 m, 3 LEDs per 10 cm = 150 LEDs) most likely has only 50 pixels: one WS2811 chip drives 3 LEDs.
Test 1 shows it: the lit spot is 10 cm long and after step 50 the strip stays dark. If the strip really has
150 pixels, the walking light is 3 cm long and runs to the end. Either way the Serial Monitor tells the step.

Files
-----
sketch.ino             The program (settings at the top). Baud rate 115200.
diagram.json           Wokwi circuit (3 strips of 150 pixels, 330 ohm resistors; paste into the diagram.json tab).
libraries.txt          Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Electrical schematic (A4 portrait).
layout-diagram.svg/.pdf  What to look for in the three tests (A4 landscape).
README.txt / README-fi.txt  This file (English / Finnish).

The three tests (they run at the same time, one per data pin)
-------------------------------------------------------------
  Test 1, D9:  ONE white pixel walks from pixel 1 to pixel 150 (150 ms per step, 22.5 s), then starts again.
  Test 2, D10: one pixel walks (60 ms per step). The colour changes every pass: red, green, blue, white.
               Pass 1 must look RED. If it looks green or blue, change COLOR_ORDER (see below).
  Test 3, D11: a blinking ruler (500 ms on, 500 ms off): every 10th pixel blue, every 50th pixel white
               (pixels 1, 51, 101 are white). Count the white ones: that is the number of 50-pixel blocks.
You can connect only one strip: for test 1 use D9, for test 2 D10, for test 3 D11. The unused pins do no harm.

Wiring (exactly)
----------------
    Arduino D9   -> 330 ohm resistor R1 -> DIN of strip 1 (the end the printed arrows point away from)
    Arduino D10  -> 330 ohm resistor R2 -> DIN of strip 2
    Arduino D11  -> 330 ohm resistor R3 -> DIN of strip 3
    12 V supply +  -> +12 V of every strip (supply only, never an Arduino pin)
    12 V supply -  -> GND of every strip AND Arduino GND (common ground)
    Arduino        -> USB cable to the computer (power + Serial Monitor)
    Optional: 1000 uF capacitor, 25 V or more, across +12 V and GND at the strip start (plus to +12 V).
  No 12 V on any Arduino pin, and the strip's +12 V never goes to the Arduino 5V pin.
  The data pin gives 5 V into a 12 V strip: this normally works, but it is not verified; the test shows it.
  Power: the strip is 7 W per metre at full white; the sketch lights ONE pixel at TEST_LEVEL 80 of 255, so the
  current is tiny. A 12 V 1 A supply is enough for all three strips.
  See wiring-diagram.svg for the picture.

Parts to buy (partco.fi, prices and stock are a snapshot of 2026-11-02, check before buying)
  - 12 V addressable strip WS2811, 5 m (the strip you already have; SKU LED-N-RGB-W2812X150-5M):
    https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
  - 12 V DC supply, 1 A or more (plug-in adapter) and a screw-terminal plug adapter
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 3 pcs 330 ohm resistors, 1000 uF 25 V capacitor (optional), jumper wires.
  (These parts are not part of the sign's parts list; see ../parts-list.* for that.)

Running it
----------
Real hardware: open sketch.ino in the Arduino IDE (library: Adafruit NeoPixel), board Arduino Mega 2560,
upload, open the Serial Monitor at 115200 baud.
Wokwi: new Arduino Mega project, paste sketch.ino and diagram.json, add the library "Adafruit NeoPixel",
press Play. The simulator shows ideal pixels, so it only shows that the program works.
Sign of life: the built-in LED (pin 13) blinks while the program runs (shortly inverted when a pass ends,
faster when many changes happen). The Serial Monitor prints one line per second:
    DBG up=<s> t1=<step>/150 t2=<step>/150 pass=<0..3> t3=on|off events=<n> beats=<n>

How to read it
--------------
  - Test 1: note the step number (t1=...) when the strip goes dark. That number is the real pixel count
    (about 50 if one chip drives 3 LEDs). Note how many LEDs light at once (1 or 3).
  - Test 2: the colour of pass 1 (red expected). Wrong colours = wrong colour order.
  - Test 3: how many white marks you see along the strip.
  - Nothing lights: check DIN end (arrows), common GND, the 12 V supply; try a level shifter (74HCT125) last.

What to tell me (fill in)
  1. Step at which test 1 goes dark: ______   LEDs lit at once: ______
  2. Colour of pass 1 / 2 / 3 in test 2: ______ / ______ / ______
  3. White marks seen in test 3: ______
  4. Flicker, dead pixel, other: ______

Changing the settings
---------------------
At the top of sketch.ino: STRIP_PIXELS (150), COLOR_ORDER (NEO_GRB + NEO_KHZ800; try NEO_RGB + NEO_KHZ800),
TEST_LEVEL (80), STEP1_MS, STEP2_MS, RULER_MS, RULER_STEP (10), RULER_BIG_STEP (50), PIN_STRIP_1..3 (9, 10, 11).

Tests
-----
tests/strip_single_led_test_harness.cpp checks the program on the computer, tests/mutate_strip_single_led.py
checks the tests with mutants:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/ssl tests/strip_single_led_test_harness.cpp && /tmp/ssl
(Not run on real hardware or in Wokwi.)
