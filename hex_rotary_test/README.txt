HEX ROTARY SWITCH TEST - a hex rotary switch (0-F) changes a number A, shown on an I2C LCD
==========================================================================================

Purpose
-------
Find out how the hex rotary switch behaves before it is used as the control of the planned settings menu
(see ../RDD-REQUIREMENTS.md, REQ-005). The switch is a free-running 16-position switch with an absolute 4-bit code
(legs 1, 2, 4, 8 and a common leg C); it is not an incremental encoder. The test reads all four legs and works out
the direction and the number of steps from the change of the position. It has nothing to do with the LED strips
or sticks of the casino sign; no LEDs are connected.

What you see
------------
The program keeps one number A in memory.
  - A goes from 2 to 51 and does NOT wrap: it stops at the ends. One switch step changes A by 1.
  - Turning the switch clockwise makes A bigger, counter-clockwise makes it smaller.
  - A is 26 at power-up (in the middle, so both directions can be tried at once).
The LCD (2x16) shows, all the time:
    row 0   A=27  (2..51)
    row 1   Pos:7 CW  n:123      raw switch position 0-F, last direction (CW / CCW, "--" before the first move),
                                 switch steps counted so far (the display stops at 9999)
How the direction is found: delta = (new position - old position) mod 16.
  1..7 = that many steps forward, 9..15 = (16 - delta) steps backward, 8 = half a turn at once: the direction cannot
  be told, so it is ignored (the position on the LCD still follows). A fast turn that skips positions is counted
  correctly up to 7 steps at a time. A new position must stay unchanged for 10 ms (contact bounce is ignored).

Two settings in sketch.ino (the first lines under CONFIGURATION)
-----------------------------------------------------------------
  reverseDirection   one bit in memory. false = a rising position code is clockwise. If A goes the wrong way when you
                     turn the switch, change false to true (no wires need to be moved).
  HEX_BIT_IS_LOW     true = a leg connected to C reads LOW and counts as a set bit (normal). If position 0 shows as F on
                     the LCD, the switch is complement-coded: set this to false.

What to try and write down
--------------------------
  1. Turn clockwise one click at a time: does A grow by 1 and does row 1 say CW? (If not: reverseDirection.)
  2. Turn the switch to the first click: is the position on row 1 0? (If it is F: HEX_BIT_IS_LOW.)
  3. Do all 16 positions show 0-F in order, with no repeated or skipped number? (A wrong order means two legs are swapped.)
  4. Turn quickly several clicks: A must change by the same number of steps.
  5. Keep turning in one direction: A must stop at 51 (or 2) and stay there.
The Serial Monitor prints the same values, so you can also read them there.

Files
-----
sketch.ino          The test program. Settings are at the top (pins, limits, reverseDirection, LCD address, timings).
diagram.json        Wokwi circuit for the simulator (a DIP switch bank 1-4 stands in for the rotary switch).
wiring-diagram.svg/.pdf   The wiring as an electrical schematic (A4).
layout-diagram.svg/.pdf   Where the wires go on the Mega header and the switch legs table (A4 landscape).
README.txt          This file (English).   README-fi.txt   The same in Finnish.
No extra library is needed: the program uses only Wire, which is built into the Arduino IDE.
Host test: tests/hex_rotary_test_harness.cpp, mutation test: tests/mutate_hex_rotary.py (see the project root).

Sign of life (rule for the whole project)
-----------------------------------------
  - The LED on pin 13 (marked L on the board) blinks all the time while the program runs (every 500 ms). It is shortly
    inverted (80 ms) at every position change and blinks four times as fast (125 ms) while the switch is turned a lot
    (three or more position changes within about a second). If it stops blinking, the program is stuck or has reset.
  - Serial Monitor speed: 115200 baud (stated at the top of sketch.ino and printed at start-up). Once per second a line is
    printed, for example:
      DBG up=12 pos=7 A=27 dir=CW steps=123 chg=40 raw=7 beats=24 lcd=1
    up = seconds since start, pos = accepted switch position (hex), A, dir = last direction, steps = switch steps counted,
    chg = position changes, raw = the four leg bits as read right now (hex), beats = LED toggles, lcd = 1 while the
    display answers. up and beats change from line to line, so you see that the program is alive.
  - At start-up an I2C scan lists every device found ("found 0x27 (LCD address in this sketch)") and warns when nothing
    answers on the LCD address.

The LCD in this test
--------------------
The display is checked all the time. If it does not answer at start-up, or stops answering, the program tries to start it
again once per second. Every 5 seconds the display is also started from scratch and redrawn (a brief flicker). This
recovers an LCD whose controller was reset by a voltage dip while the backlight stayed on, which looks like "light
but no text". (Set LCD_REINIT_MS to 0 to turn the periodic restart off.)
If the LCD shows no text but its light is on:
  - turn the blue contrast trimmer on the I2C backpack with a small screwdriver;
  - measure between the LCD VCC and GND pins: at least 4.75 V is needed (the Mega's own regulator needs 7 V or more on the
    round power jack; USB is fine for this test);
  - read the Serial Monitor: the scan must list the display (usually 0x27, sometimes 0x3F: set LCD_I2C_ADDRESS).

Parts
-----
  - Arduino Mega 2560 (USB cable for power, upload and Serial Monitor).
  - 1 x hex rotary switch 0-F with legs 1, 2, 4, 8 and two C (the owner's partco "hex-enkooderi kiertokytkin tapilla 0-F").
  - 1 x 2x16 character LCD with an I2C backpack (PCF8574).
  - 9 jumper wires (5 for the switch: four legs and C, 4 for the LCD), female-female or as the parts need.

Wiring (exactly; see wiring-diagram.pdf and layout-diagram.pdf)
---------------------------------------------------------------
  Hex rotary switch leg     Arduino Mega pin
    1                       D34
    2                       D35
    4                       D36
    8                       D37
    C (one of the two)      GND   (any GND pin; the other C leg stays free)
  No resistors: the Mega's internal pull-ups are used (INPUT_PULLUP). A leg connected to C reads LOW = bit set.
  I2C LCD                   Arduino Mega pin
    GND                     GND
    VCC                     5V
    SDA                     D20 (SDA)
    SCL                     D21 (SCL)
  Pin 13 (the onboard LED) is used for the sign of life.

Switching on
------------
  1. Arduino IDE: board Arduino Mega or Mega 2560, open sketch.ino, Upload over USB.
  2. Open the Serial Monitor at 115200 baud: the start text, the I2C scan and then one DBG line per second appear.
  3. Turn the switch: A changes on the LCD and in the DBG line.
  Simulator: open diagram.json and sketch.ino in Wokwi. Wokwi has no hex rotary switch, so a DIP switch bank stands in:
  switch 1 = leg 1 (D34), 2 = leg 2 (D35), 3 = leg 4 (D36), 4 = leg 8 (D37); ON = leg connected to C = bit set. Setting the
  four switches to the binary number of the next position moves A one step.
