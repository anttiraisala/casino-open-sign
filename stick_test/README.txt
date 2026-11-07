STICK TEST - can the Arduino and a separate 5 V supply drive 10 Grove RGB LED sticks?
======================================================================================

Purpose
-------
A stand-alone test, independent of the casino sign sketch. It runs 10 Grove RGB LED sticks
(10x WS2813 mini, 10 LEDs each = 100 LEDs) from an Arduino Mega 2560 whose data pins drive the sticks,
while the sticks take their power from a SEPARATE 5 V supply. It shows whether the whole set works
(wiring, power, ground, data) and how much power the sticks draw as the brightness rises.
Upload it with the Arduino IDE.

Files
-----
sketch.ino          The test program. Settings are at the top (ACTIVE_STICKS, MAX_BRIGHTNESS, pins).
diagram.json        Wokwi circuit for the simulator (10 strips of 10 pixels; the simulation powers them from 5V).
libraries.txt       Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf   The wiring as an electrical schematic (A4).
layout-diagram.svg/.pdf   The 10-stick arrow (design B), stick numbers and pins (A4 landscape).
README.txt          This file (English).   README-fi.txt   The same in Finnish.

Parts for the test (partco.fi; prices and stock are a snapshot of 2026-11-02, check before buying)
---------------------------------------------------------------------------------------------
  - 10 x Grove RGB LED Stick (10x WS2813 mini), 80 x 10 x 10 mm, 3.3/5 V (you already have 9;
    buy the missing one from partco.fi, search "Grove RGB LED Stick").
  - 1 x Mean Well LRS-50-5, 5 V 10 A supply, 23.79 EUR
    https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x blade fuse 10 A (SUA-10A), 0.31 EUR, with a fuse holder
    https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x capacitor 1000 uF 25 V (ELKO105-1000U-25V), 1.73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - 10 x resistor 330 ohm (VASTUS-2W-330R), 0.45 EUR each (recommended, in the data lines)
    https://partco.fi/tuote/metallioksidikalvovastus-2w-330ohm-9515
  - Arduino Mega 2560 and (optionally) a Grove Base Shield for it, USB cable, wire (1.5 mm2 for the +5 V and
    0 V rails), optional push button.
  The 230 V side of the supply must be in an enclosure and wired by someone who knows how.

How the sticks are powered (important)
--------------------------------------
Full white on 100 LEDs can draw about 5 A (about 0.5 A per stick). The Arduino and the Grove shield cannot
supply that, so the sticks take +5 V and 0 V from the PSU. The Arduino provides ONLY the data (yellow wire)
and the common ground. The sketch limits the brightness (MAX_BRIGHTNESS 128, about half) so the test never
asks for the full 5 A; raise it only after the first run was fine.

Grove cable change: in the shield-side Grove connector of each stick cable, pull the red (VCC) and black (GND)
terminals out of the plastic housing (lift the small latch with a needle) and insulate them, so the shield
5 V and the stick supply are not connected through the cables. Keep yellow (SIG). Do the same for all 10 cables.
The stick-side ends of the red and black wires go to the PSU rails.

Wiring (exactly; see wiring-diagram.pdf)
----------------------------------------
  Stick  Data pin (via 330 ohm)      Stick  Data pin (via 330 ohm)
   1     D2                           6     D7
   2     D3                           7     D8
   3     D4                           8     A0
   4     D5                           9     A1
   5     D6                          10     A2
  Yellow wire (DIN) of stick n -> 330 ohm resistor -> the pin above.
  Red wire of every stick   -> +5 V rail (PSU V+ through the 10 A fuse).
  Black wire of every stick -> 0 V rail (PSU V-).
  1000 uF capacitor across the rails, plus on +5 V.
  Arduino GND -> 0 V rail with ONE wire (the only ground link). Do not connect the Arduino 5V pin to the rails.
  Optional push button between D22 and GND: skips to the next phase.
  Shield 3V3/5V switch (if used) must be on 5 V.

Switching on (order)
--------------------
  1. Check the wiring, +5 V against 0 V with no short.
  2. Switch the 5 V supply on first (the sticks stay dark).
  3. Plug in the USB cable (the Arduino starts the test). Switch off in the reverse order.
  4. Arduino IDE: install the library "Adafruit NeoPixel", board Arduino Mega or Mega 2560, open sketch.ino,
     Upload. Serial Monitor at 115200 baud shows the phases.

What the test does (it repeats)
-------------------------------
  - IDENTIFY: each stick lights white in turn, 1.2 s each (stick 1..10). Check the numbering/pins.
  - COLOURS: all sticks red, then green, then blue. Check the colour order is right.
  - WALK: one white LED runs through all 100 LEDs. Check every LED works and the chain is fine.
  - RAMP: all LEDs white, brightness rising in steps (16, 32, 64, 96, 128), 5 s each. Watch for dimming,
    colour shifts toward red/yellow at the far sticks, flicker or resets. The Serial Monitor shows the
    estimated current. The pin 13 LED is on during RAMP and SOAK.
  - SOAK: 30 s at the highest brightness; then a 3 s rest and the cycle starts again.
  The button (D22) skips the current phase.

What to report back
-------------------
  - Does every stick light, in the right order and colours?  - Any flicker, reset, dimming or yellowish tint?
  - Does the PSU stay cool and the voltage stay near 5 V at full brightness (a multimeter at the last stick)?
  - Do the sticks work with the 330 ohm resistors? Did you use ACTIVE_STICKS 9 or 10?
  Note: the "estimated current" is a calculation (20 mA per channel), not a measurement.

Settings
--------
  ACTIVE_STICKS = 10 (set 9 if you have only nine; the pin list stays the same).
  MAX_BRIGHTNESS = 128 (0..255). IDENTIFY/COLOUR/WALK brightness values are separate and low.

Notes
-----
  - Hot swapping Grove sticks is not supported: power everything off before changing cables.
  - This test is separate from the casino sign and changes nothing in it.
  - The host test tests/stick_test_harness.cpp checks the sketch logic with a mock library (not real hardware).
