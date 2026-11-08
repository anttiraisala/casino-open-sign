LED STRIP KNIGHT RIDER - one light runs back and forth on three strips
======================================================================

Purpose
-------
A demo for three 12 V WS2811 strips of 50 pixels each (the 5 m roll: one control chip per 3 LEDs, so 50 chips,
150 LEDs). One pixel is lit at a time and it bounces between the two ends like the light of Knight Rider.
It also shows how the addressing works: pixel 1 is the chip next to the DIN end, the lit pixel is chosen only by
its position in the data the Arduino sends. See "How the addressing works" below.

  Pin D9:  RED   light, 1 pixel per second   (one cycle end to end and back = 98 s)
  Pin D10: BLUE  light, 4 pixels per second  (one cycle = 24.5 s)
  Pin D11: GREEN light, 10 pixels per second (one cycle = 9.8 s)
The lit pixel is 10 cm long (3 LEDs) if the strip is built like the roll described above.
A cycle is 98 steps: pixel 1, 2, ... 50, 49, ... 2, then 1 again (the end pixels are shown once per turn).

Files
-----
sketch.ino             The program (settings at the top). Baud rate 115200.
diagram.json           Wokwi circuit (3 strips of 50 pixels, 330 ohm resistors; paste into the diagram.json tab).
libraries.txt          Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Electrical schematic (A4 portrait).
layout-diagram.svg/.pdf  What you should see (A4 landscape).
README.txt / README-fi.txt  This file (English / Finnish).

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
  The data pin gives 5 V into a 12 V strip: this normally works, but it is not verified.
  Power: one lit pixel per strip at TEST_LEVEL 80 of 255 takes very little current; 12 V 1 A is plenty.
  You can connect only one or two strips; the unused pins do no harm. See wiring-diagram.svg.

Parts to buy (partco.fi, prices and stock are a snapshot of 2026-11-02, check before buying)
  - 12 V addressable strip WS2811, 5 m (SKU LED-N-RGB-W2812X150-5M): https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
  - 12 V DC supply, 1 A or more (plug-in adapter) and the screw-terminal plug adapter
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 3 pcs 330 ohm resistors, 1000 uF 25 V capacitor (optional), jumper wires.
  (Not part of the sign's parts list; see ../parts-list.*.)

How the addressing works (short)
--------------------------------
The chips have no address. Each chip takes the first 24 bits (one colour) from the data it receives on DIN,
keeps them and passes the rest on from DOUT to the next chip. So the chip next to DIN gets the first colour (pixel 1),
the second chip the second colour, and so on. After the data a pause of over 50 microseconds latches all colours.
If you cut the strip at a cut line, the piece is a complete strip: feed the DIN end, and the chip next to it is
pixel 1. The last chip does not "know" it is the last; its DOUT just goes nowhere. Colours sent for more pixels than
exist are lost, so STRIP_PIXELS must match the real number. Feeding from the DOUT end does not work (one direction).

Running it
----------
Real hardware: open sketch.ino in the Arduino IDE (library: Adafruit NeoPixel), board Arduino Mega 2560, upload,
open the Serial Monitor at 115200 baud. Wokwi: new Arduino Mega project, paste sketch.ino and diagram.json, add the
library "Adafruit NeoPixel", press Play.
Sign of life: the built-in LED (pin 13) blinks while the program runs (shortly inverted when a light turns around at
an end). The Serial Monitor prints one line per second:
    DBG up=<s> d9=<pixel>/50 d10=<pixel>/50 d11=<pixel>/50 events=<n> beats=<n>

Changing the settings
---------------------
At the top of sketch.ino: STRIP_PIXELS (50), COLOR_ORDER (NEO_GRB + NEO_KHZ800; try NEO_RGB + NEO_KHZ800 if red is
not red), TEST_LEVEL (80), STEP1_MS (1000), STEP2_MS (250), STEP3_MS (100), PIN_STRIP_1..3 (9, 10, 11).

Tests
-----
tests/strip_knight_rider_test_harness.cpp checks the program on the computer, tests/mutate_strip_knight_rider.py
checks the tests with mutants:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/kr tests/strip_knight_rider_test_harness.cpp && /tmp/kr
(Not run on real hardware or in Wokwi.)
