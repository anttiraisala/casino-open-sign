STRIP FRAME DEMO TEST - three colour lights on a 18-pixel frame strip and a pulsing single pixel
================================================================================================

(The owner called it "Street Frame Demo Test"; read as "Strip Frame Demo Test", folder strip_frame_demo_test.)

Purpose
-------
Demo for the LED strip of the sign frame, using the same 12 V WS2811 strip as strip_knight_rider_test (one control
chip = one "pixel" = 3 LEDs = 10 cm).

  Pin D9:  FRAME strip of 18 pixels (1.8 m, 54 LEDs) around the whole sign (60 x 30 cm). The strip starts at the
           lower-left corner and runs counter-clockwise:
             pixels  1.. 6  bottom edge, left to right
             pixels  7.. 9  right edge, going up
             pixels 10..15  top edge, right to left
             pixels 16..18  left edge, going down (18 is right above 1)
           Three Knight Rider lights run on the same strip. Their route starts at the UPPER-LEFT corner (pixel 15) and
           runs CLOCKWISE: top edge to the right (15..10), right edge down (9..7), bottom edge to the left (6..1), left
           edge up (18..16). At pixel 16, the end of the route, a light turns around and runs back along the same route
           to pixel 15, and so on:
             RED   2 pixels per second           (500 ms per step, cycle 17 s)
             GREEN 1.3 x faster than red         (385 ms per step, cycle 13.09 s)
             BLUE  1.5 x faster than red         (333 ms per step, cycle 11.32 s)
           They drift out of phase and the colours ADD where they overlap: red + green = yellow, green + blue = cyan,
           red + blue = magenta, all three = white. End pixels are shown once per turn (a cycle is 34 steps).
  Pin D10: SINGLE pixel (one chip, 3 LEDs). A pulse dark -> bright -> dark every 3 s; the colour changes at every
           pulse: red, orange, yellow, green, cyan, blue, violet, white, then again.

Peak brightness is TEST_LEVEL 80 of 255 (same as the other strip tests).
Not part of the sign software; the direction/position of the frame strip only matters for how you read the result.

Files
-----
sketch.ino             The program (settings at the top). Baud rate 115200.
diagram.json           Wokwi circuit (18-pixel strip on D9, 1-pixel strip on D10, 330 ohm resistors; paste into the diagram.json tab).
libraries.txt          Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Electrical schematic (A4 portrait).
layout-diagram.svg/.pdf  Frame route and what you should see (A4 landscape).
README.txt / README-fi.txt  This file (English / Finnish).

Wiring (exactly)
----------------
    Arduino D9   -> 330 ohm resistor R1 -> DIN of the frame strip, 18 pixels (the end the printed arrows point away from)
    Arduino D10  -> 330 ohm resistor R2 -> DIN of the single-pixel piece (one chip = 3 LEDs, cut from the same roll)
    12 V supply +  -> +12 V of both strips (supply only, never an Arduino pin)
    12 V supply -  -> GND of both strips AND Arduino GND (common ground)
    Arduino        -> USB cable to the computer (power + Serial Monitor)
    Optional: 1000 uF capacitor, 25 V or more, across +12 V and GND at the strip start (plus to +12 V).
  No 12 V on any Arduino pin, and the strip's +12 V never goes to the Arduino 5V pin.
  The data pin gives 5 V into a 12 V strip: this normally works, but it is not verified.
  Power: at most three lit pixels at TEST_LEVEL 80 of 255 take very little current; 12 V 1 A is plenty.
  See wiring-diagram.svg. You can also test with only one of the two strips connected.

Parts to buy (partco.fi, prices and stock are a snapshot of 2026-11-02, check before buying)
  - 12 V addressable strip WS2811, 5 m (SKU LED-N-RGB-W2812X150-5M): https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
    (cut at the cut lines: 18 pixels for D9 and 1 pixel for D10; the rest stays for other tests)
  - 12 V DC supply, 1 A or more (plug-in adapter) and the screw-terminal plug adapter
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 2 pcs 330 ohm resistors, 1000 uF 25 V capacitor (optional), jumper wires.
  (Not part of the sign's parts list; see ../parts-list.*. If you already own the parts of strip_knight_rider_test, you need nothing new.)

How the addressing works (short)
--------------------------------
The chips have no address. Each chip takes the first 24 bits (one colour) from the data it receives on DIN,
keeps them and passes the rest on from DOUT to the next chip. So the chip next to DIN gets the first colour (pixel 1),
the second chip the second colour, and so on. After the data a pause of over 50 microseconds latches all colours.
Colours sent for more pixels than exist are lost, so FRAME_PIXELS and SINGLE_PIXELS must match the real numbers.
Feeding from the DOUT end does not work (one direction).

Running it
----------
Real hardware: open sketch.ino in the Arduino IDE (library: Adafruit NeoPixel), board Arduino Mega 2560, upload,
open the Serial Monitor at 115200 baud. Wokwi: new Arduino Mega project, paste sketch.ino and diagram.json, add the
library "Adafruit NeoPixel", press Play.
Sign of life: the built-in LED (pin 13) blinks while the program runs (shortly inverted when a frame light turns around at
an end and at every new pulse colour; twice as fast while many changes happen). The Serial Monitor prints one line per second:
    DBG up=<s> red=<pixel> green=<pixel> blue=<pixel> d10=<colour 0-7>:<level 0-80> events=<n> beats=<n>
(strip pixels 1..18; colour numbers 0 red, 1 orange, 2 yellow, 3 green, 4 cyan, 5 blue, 6 violet, 7 white.)

Changing the settings
---------------------
At the top of sketch.ino: FRAME_PIXELS (18), ROUTE_START_PIXEL (15), SINGLE_PIXELS (1), COLOR_ORDER (NEO_GRB + NEO_KHZ800; try NEO_RGB + NEO_KHZ800
if red is not red), TEST_LEVEL (80), STEP_RED_MS (500), STEP_GREEN_MS (385), STEP_BLUE_MS (333), PULSE_TICKS (150, with
PULSE_UPDATE_MS 20 = 3 s per pulse), PIN_FRAME (9), PIN_SINGLE (10).

Tests
-----
tests/strip_frame_demo_test_harness.cpp checks the program on the computer, tests/mutate_strip_frame_demo.py checks the
tests with mutants:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/sf tests/strip_frame_demo_test_harness.cpp && /tmp/sf
    python3 tests/mutate_strip_frame_demo.py
(Not run on real hardware or in Wokwi.)
