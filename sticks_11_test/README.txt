11 STICKS - the 10 arrow sticks and the CLOSED light stick on one Grove Base Shield
==================================================================================

Purpose
-------
Keep all 11 Grove RGB LED sticks lit as in the casino sign and check that nothing gets hot: the Grove Base Shield,
the Grove connectors, the sticks, the wires, the supply. The 10 sticks of the arrow are plugged into the connectors
exactly as in the sign; the 11th stick, the "CLOSED" light (the sign's open/closed signal), is in the one connector
that is left over: A3. Upload with the Arduino IDE. The test has nothing to do with the sign's own program.
(Similar to ../stick_test_direct, but with the sign's stick order, a steady CLOSED light and the sign's brightness.)

What you see
------------
  - Stick 11 (A3, the CLOSED light): all 10 LEDs steady white at level 80 of 255, all the time.
  - Sticks 1-10 (the arrow): a white wave flows along every stick (brightness 0..80, average about half), so the
    arrow load is like the flowing arrow of the sign.
  - Button (pin 33 to GND, optional): a press switches between FLOW (default) and ALL ON = all 110 LEDs steady white at
    80, the worst case. Try ALL ON for a while too (estimated 2.1 A, a guess).
  - Everything starts at once at power-up (no dark pause).

Stick, connector and name (as in the sign; see ../arrow_sticks/)
  Stick  1 L-CAP D2        Stick  7 RAIL-DN-R D8
  Stick  2 L-WING-UP D3    Stick  8 R-CAP A0
  Stick  3 L-WING-DN D4    Stick  9 R-WING-UP A1
  Stick  4 RAIL-UP-L D5    Stick 10 R-WING-DN A2
  Stick  5 RAIL-UP-R D6    Stick 11 CLOSED light A3
  Stick  6 RAIL-DN-L D7

Files
-----
sketch.ino             The test program. Settings at the top (ARROW_LEVEL, CLOSED_LEVEL, START_ALL_ON, ...).
diagram.json           Wokwi circuit (11 strips of 10 pixels, powered from 5V in the simulation; paste into diagram.json).
libraries.txt          Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  The wiring as an electrical schematic (A4 portrait).
layout-diagram.svg/.pdf  Which stick goes to which connector and where to feel for heat (A4 landscape).
README.txt / README-fi.txt  This file (English / Finnish).

Sign of life (rule for the whole project)
-----------------------------------------
  - The LED on pin 13 blinks all the time while the program runs (every 250 ms); it is shortly inverted when the
    mode changes and blinks twice as fast if the mode changes often. If it stops, the program is stuck or reset.
  - Serial Monitor speed: 115200 baud (stated at the top of sketch.ino and printed at start-up). Once per second:
      DBG up=<s> mode=flow|all fps=50 lit=<LEDs of 110> ma=<estimated mA> events=<n> beats=<n>
    The estimated current is a guess (20 mA per colour channel at 255), not a measurement.
    The Serial Monitor needs the USB cable, which is NOT plugged in while the 5 V supply is on (see below).

Parts (partco.fi; prices and stock are a snapshot of 2026-11-02, check before buying)
-------------------------------------------------------------------------------------
  - 11 x Grove RGB LED Stick (10x WS2813 mini) with Grove cables (search "Grove RGB LED Stick"), Arduino Mega 2560 + Grove Base Shield.
  - 1 x Mean Well LRS-50-5, 5 V 10 A, 23.79 EUR  https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x blade fuse 10 A (SUA-10A) 0.31 EUR + holder  https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x capacitor 1000 uF 25 V (ELKO105-1000U-25V) 1.73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - Wire 1.5 mm2 for the supply, optional push button.
  The 230 V side of the supply must be in an enclosure and wired by someone who knows how.

Wiring (exactly; see wiring-diagram.pdf)
----------------------------------------
  Stick n plugged with its own Grove cable into the connector in the table above (connector = signal pin).
  Shield 3V3/5V switch: 5 V.
  PSU V+ -> 10 A fuse -> the Mega 5V pins; PSU V- -> the Mega GND pins. Use the 5V and GND pins at the end of the long
  double-row header (the shield does not cover them), both 5V pins and both GND pins, 1.5 mm2 wire.
  1000 uF capacitor across the supply wires, plus on +5 V.
  Optional push button between D33 and GND.
  The sticks get their power only through the shield and the Grove connectors; no wire from the supply to a stick.
  The USB cable is NOT connected while the supply is on.

Switching on
------------
  1. Arduino IDE: library "Adafruit NeoPixel", board Arduino Mega 2560, open sketch.ino and Upload over USB (supply off).
  2. Unplug USB. Plug in the sticks, wire the supply. 3. Switch the supply on: the lights start at once.
  (Two 5 V sources side by side can push current into the USB port, so no USB while the supply is on. To read the
  Serial Monitor use USB only, with the supply off and only a few sticks, or just trust the pin 13 LED.)

What to check / report back
---------------------------
  - Let it run in FLOW mode for 30-60 minutes. Feel (carefully) the shield, the Grove connectors, the cables, the
    stick backs, the CLOSED light stick (lit all the time) and the supply. Anything warm or hot?
  - Then ALL ON for 5-10 minutes (button). Same check. Switch off at once if something is hot or smells.
  - Any flicker, dimming, yellow tint, the last stick weaker, or the Arduino restarting (the wave starts over)?
  - If you have a multimeter: voltage at the last stick (4.5 V or more) and the supply current.
  - If something gets warm or dim: lower ARROW_LEVEL / CLOSED_LEVEL (for example 40).

Tests
-----
tests/sticks_11_test_harness.cpp checks the sketch logic on the computer; tests/mutate_sticks_11.py checks the tests with mutants:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/s11 tests/sticks_11_test_harness.cpp && /tmp/s11
(Not run on real hardware or in Wokwi; the current figures are estimates.)
