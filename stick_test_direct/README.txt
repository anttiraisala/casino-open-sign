DIRECT STICK TEST - up to 11 Grove RGB LED sticks plugged UNMODIFIED into the Grove Base Shield
===============================================================================================

Purpose
-------
A proof of concept that the LED sticks work and that they can all run at the same time. Each stick is plugged with
its ordinary Grove cable into a connector of the Grove Base Shield on the Arduino Mega 2560; nothing is moved or
modified. The whole Arduino + shield is fed from an external 5 V supply, so the question is: does the power reach
all sticks through the shield and the Grove connectors? Upload with the Arduino IDE. The test has nothing to do
with the casino sign itself.
(The other test, ../stick_test, feeds the sticks straight from the supply with modified Grove cables.)

What you see
------------
Every stick shows a moving pattern ALL THE TIME, from the moment the program starts (no dark pause, no test phases).
  - 11 patterns: scanner ("Knight Rider"), chase, comet, rainbow comet, theater chase, two bouncing dots, fill and
    empty, odd/even blink, sparkle, police lights, reverse runner. Each pattern lights only 1-6 LEDs at a time.
  - SPOTLIGHT: one stick at a time (1.2 s each) lights all 10 of its LEDs white; the spotlight walks stick 1, 2, ... 11.
  - PATTERN SHIFT: every 9.6 s the patterns move one stick further, so every stick shows every pattern in turn.
  - LOAD TEST: hold the push button (D22 to GND): all LEDs of all sticks light white at MAX_BRIGHTNESS
    (worst case, about 3.3 A estimated with 11 sticks at 128) until you let go. Do this for a minute and feel
    whether the shield, the connectors and the wires get warm.
The Grove Base Shield for the Mega has 11 connectors with a data pin (D2..D8 and A0..A3), so up to 11 sticks
work. Set ACTIVE_STICKS in sketch.ino to the number you have (default 11; unused connectors do no harm).

Files
-----
sketch.ino          The test program. Settings are at the top (ACTIVE_STICKS, MAX_BRIGHTNESS, timings).
diagram.json        Wokwi circuit for the simulator (11 strips of 10 pixels; the simulation powers them from 5V).
libraries.txt       Library: Adafruit NeoPixel.
wiring-diagram.svg/.pdf   The wiring as an electrical schematic (A4).
layout-diagram.svg/.pdf   The 10-stick arrow (design B) and the pin of each stick (A4 landscape).
README.txt          This file (English).   README-fi.txt   The same in Finnish.

Sign of life (rule for the whole project)
-----------------------------------------
  - The LED on pin 13 (marked L on the board) blinks all the time while the program runs (every 250 ms).
    It shows the load: it is shortly inverted when something changes (the spotlight moves or the patterns shift),
    and it blinks twice as fast while many changes happen or the load test is on.
    If it stops blinking, the program is stuck or the Arduino has reset.
  - Serial Monitor speed: 115200 baud (stated at the top of sketch.ino and printed at start-up).
    Once per second a line is printed, for example:
      DBG up=12s fps=50 spot=11 set=1 events=10 beats=47 load=0
    up = seconds since start, fps = frames per second, spot = the stick that has the spotlight, set = pattern set,
    events = state changes so far, beats = LED toggles, load = 1 while the button is held. The values change from
    line to line, so you see that the program is alive.
    The Serial Monitor needs the USB cable, which is NOT plugged in while the supply is on (see below).

Parts (partco.fi; prices and stock are a snapshot of 2026-11-02, check before buying)
-------------------------------------------------------------------------------------
  - Up to 11 x Grove RGB LED Stick (10x WS2813 mini) with their Grove cables (search "Grove RGB LED Stick").
  - 1 x Mean Well LRS-50-5, 5 V 10 A, 23.79 EUR  https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x blade fuse 10 A (SUA-10A) 0.31 EUR + fuse holder  https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x capacitor 1000 uF 25 V (ELKO105-1000U-25V) 1.73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - Wire 1.5 mm2 for the supply, Arduino Mega 2560 + Grove Base Shield, optional push button.
  The 230 V side of the supply must be in an enclosure and wired by someone who knows how.

Wiring (exactly; see wiring-diagram.pdf)
----------------------------------------
  Stick  Grove connector (signal pin)     Stick  Grove connector (signal pin)
   1     D2                                7     D8
   2     D3                                8     A0
   3     D4                                9     A1
   4     D5                               10     A2
   5     D6                               11     A3
   6     D7
  Shield 3V3/5V switch: 5 V.
  PSU V+ -> 10 A fuse -> the Mega 5V pins; PSU V- -> the Mega GND pins. Use the 5V and GND pins at the end of
  the long double-row header (the shield does not cover them), both 5V pins and both GND pins, 1.5 mm2 wire.
  1000 uF capacitor across the supply wires, plus on +5 V.
  Optional push button between D22 and GND (load test while held).
  The USB cable is NOT connected while the supply is on (see below).

Switching on
------------
  1. Arduino IDE: install the library "Adafruit NeoPixel", board Arduino Mega or Mega 2560, open sketch.ino and
     Upload over USB (the supply is off).
  2. Unplug the USB cable. Plug the sticks into the connectors, wire the supply.
  3. Switch the supply on. The patterns start at once and the pin 13 LED blinks.
  Why no USB at the same time: the supply feeds the 5V pin directly, and two 5 V sources side by side
  can push current into the USB port. Without USB there is no Serial Monitor; the pin 13 LED shows that the
  program runs. (To read the Serial Monitor, unplug the supply, plug in USB and run the test with
  ACTIVE_STICKS = 1 or 2 only.)

What to check / report back
---------------------------
  - Does every stick show its pattern, and does the spotlight walk through all sticks?
  - Any flicker, wrong colours, dimming, yellow tint, or the Arduino restarting (the spotlight starts again from stick 1)?
  - Hold the button for a minute: does anything get warm (the shield, the Grove connectors, the supply wires,
    the 5V pin area)? Full white is about 0.5 A per stick; the shield traces and Grove connectors are made for
    small currents, so this is what the test checks. Voltage at the last stick with a multimeter (4.5 V or more)?
  - If something gets warm or dim: lower MAX_BRIGHTNESS (for example 64) or use ../stick_test.

Notes
-----
  - Hot swapping Grove sticks is not supported: switch everything off before changing cables.
  - The host test tests/stick_test_direct_harness.cpp checks the sketch logic with a mock library (not real hardware).
  - This test is separate from the casino sign and changes nothing in it.
