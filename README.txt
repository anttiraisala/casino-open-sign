CASINO SIGN "CASINO OPEN" - Arduino Mega 2560 + 10 WS2812B LED strips (Wokwi project)
=====================================================================================

PROJECT STATUS (2026-11-06)
---------------------------
- The sign sketch (sketch.ino) still describes the first design: 10 WS2812B LED strips on pins D4-D12 and D31.
  Planned but NOT built: 12 V WS2811 strips, 60 x 30 cm plate and the arrow made of 10 Grove LED sticks (design B).
- Tested on real hardware by the user: stick_test_direct. All 11 Grove RGB LED sticks plugged unmodified into the
  Grove Base Shield (connectors D2..D8, A0..A3), Arduino + shield fed from the external 5 V supply, all sticks show
  their moving patterns. So the power and the data are enough for 11 sticks (stick_test, the version with modified
  cables, was therefore not needed).
- Not yet tested on real hardware: strip_test (WS2812B strip), the sign sketch itself.
- Sign of life rule (pin 13 LED + one Serial line per second) is in stick_test_direct only; the other sketches get
  it when they are next updated (see AI_CONTEXT.md).
- The parts lists are a snapshot of 2026-11-02 (prices, stock and links are not refreshed on their own).

Files
-----
DESCRIPTION.md             What this project is, what it is for and what is planned.
AI_CONTEXT.md              Detailed technical description for a new AI chat: read this first
                           when continuing the project.
sketch.ino                 Arduino program. All settings are at the top.
diagram.json               Wokwi circuit: Arduino Mega, the LED strip parts drawn in
                           the shape of the sign, a slide switch, a push button and
                           an I2C LCD and a few text labels.
libraries.txt              Required library: Adafruit NeoPixel.
tools/generate_diagram.py  Regenerates diagram.json from the settings in sketch.ino.
tools/size_calculator.py   Suggests the LED counts for a given sign size.
wiring-diagram.svg/.pdf    The electrical schematic, one A4 portrait page (print at 100 %).
layout-diagram.svg/.pdf    Where every LED strip goes on the plate: to scale, with names,
                           pins, LED counts, LED index ranges and dimensions in cm.
                           One A4 landscape page (the plate is drawn at about 1:2.4).
layout-parts.svg/.pdf      Parts list page (A4 portrait): LED counts, cutting list and the
                           LEDs and index range of every letter.
tests/                     Host-side test harness (see AI_CONTEXT.md, section 8).
parts-list.pdf/.csv/.xlsx  The complete shopping list: every component with quantity, price, stock
                           status and a link to partco.fi (PDF for printing, CSV for any spreadsheet,
                           XLSX with formulas for the line totals and sums). The master parts list.
tools/parts_data.py        The data of the parts list (edit parts, quantities and prices here).
tools/generate_parts_list.py  Makes the three parts list files.
strip_test/                A separate small test project for the LED strip: cut a short piece of the strip, run
                           strip_test/sketch.ino (Arduino or Wokwi with strip_test/diagram.json) and tell what you
                           see (colour order, bit speed, LEDs per pixel). See strip_test/README.txt.
stick_test/                A separate test for 10 Grove RGB LED sticks (planned stick arrow) on a separate 5 V supply:
                           upload stick_test/sketch.ino with the Arduino IDE. See stick_test/README.txt.
stick_test_direct/        Second stick test: up to 11 sticks plugged unmodified into the Grove shield, always-moving patterns,
                           external 5 V on the Mega 5V/GND pins. See stick_test_direct/README.txt.
tools/export_pdf.py        Makes the three PDFs from the three SVGs (needs Playwright).
tools/generate_schematic.py  Redraws wiring-diagram.svg from the settings in sketch.ino.
tools/generate_layout.py   Redraws layout-diagram.svg and layout-parts.svg from sketch.ino.
tools/sign_data.py         Shared strip data used by the two drawing scripts.
README.txt                 This file (English).
README-fi.txt              The same in Finnish.

Contents
--------
 1. Opening the project in Wokwi
 2. The sign in Wokwi
 3. Sign size and LED counts
 4. Wiring: what is connected to which pin
 5. Parts to buy (partco.fi)
 6. Strips, LED order and cutting list
 7. Changing the sizes or the text
 8. Animations and inputs
 9. Real hardware notes


1. Opening the project in Wokwi
-------------------------------
1. Go to https://wokwi.com/projects/new/arduino-mega
2. Every file goes into the tab with the same name:
     sketch.ino   -> the sketch.ino tab (C++ code, starts with a /* comment)
     diagram.json -> the diagram.json tab (JSON, starts with { and "version": 1)
   Select everything in the tab (Ctrl+A) and paste the whole file over it. If the
   sketch ends up in the diagram.json tab, the Play button disappears because the
   diagram is not valid JSON.
3. Add the library: either put libraries.txt into the libraries.txt tab (create the tab
   with the "+" button if it is missing), or use the Library Manager and add
   "Adafruit NeoPixel".
4. Press the green Play button. Use the zoom (+ / - / F keys in the diagram) to see
   the whole sign.


2. The sign in Wokwi
--------------------
Wokwi has no drawing or background-image part, so the sign is built from the LED
strips themselves. Each strip part sits where it would be on the real sign:
  - the frame is a ring of four edges,
  - "CASINO" is above "OPEN"; behind each letter sits a group of single LEDs drawn as
    a circle, with the letter written in the middle (a Wokwi text label; its size is
    fixed by Wokwi and may not fit the circle exactly),
  - under the OPEN row a short white strip (the CLOSED light, 6 LEDs) with a text label; on the
    real sign it sits behind the plate under a hole text "CLOSED",
  - the arrow is a rectangle with a triangular head on each end,
  - the inputs (switches, button, DIP bank) are above the Arduino with their captions above them,
    and the I2C LCD is below the Arduino.
In Wokwi the 60 LEDs per metre strip is drawn at 16 pixels per LED, so 16 pixels in
the diagram are 16.7 mm on the real sign and the whole frame is 480 x 432 pixels,
which is 50 x 45 cm.
Neighbouring physical strips are drawn one LED apart so that every strip can be seen
on its own. On the real sign the strips touch each other. Some parts in the diagram
are only pieces of one bent strip (for example the single LEDs of one letter, or the
two wings of an arrowhead); they are chained inside Wokwi. Text labels on the right
list which Arduino pin drives which strip. A DIP switch bank stands in for the two speed
switches (see section 8). The casino closed switch SW5 starts in the open position
(handle right); move the handle left to close the casino.

If parts overlap slightly or leave small gaps in Wokwi, drag them or tune PITCH,
THICKNESS and LENGTH_PADDING at the top of tools/generate_diagram.py and run it again.


3. Sign size and LED counts
---------------------------
Starting point (it may still change):
  LED strip        WS2812B, 5 V, 60 LEDs per metre (LED pitch 16.7 mm)
  sign (plate)     50 cm wide, 45 cm high
  arrow            40 cm wide, 15 cm high
  CLOSED light     6 white LEDs behind the plate, hole text "CLOSED" about 10 x 2.5 cm

Resulting LED counts (rounded to whole LEDs):
  frame            2 x 53 = 106 LEDs   (top and bottom edge 28 LEDs, left and right edge 25)
                   outer edge of the LED ring: 50.0 x 45.0 cm
  letters          58 LEDs             (6 per letter, 4 for the narrow "I")
  arrow            68 LEDs             (shaft edges 2 x 10, end caps 2 x 4, arrowheads 2 x 2 x (2 + 8))
                   size: 39.1 x 16.7 cm (an arrow of exactly 15 cm is not possible with whole LEDs
                   and an even cap; 13.3 cm would be the other choice)
  CLOSED light     6 LEDs              (white, own data pin; lit only while the casino is closed)
  total            238 LEDs = 3.97 m of strip, so one 5 m reel (300 LEDs) is enough.

Vertical layout of the 45 cm plate, from the top: frame edge, the two text rows (CASINO,
OPEN), the CLOSED hole text (about 2.5 cm tall and 10 cm wide), the arrow (16.7 cm high) and
the frame edge. About 5 cm of free space is left under the arrow.

Current: a WS2812B LED takes up to 60 mA (full white, full brightness). 238 LEDs would
take up to 14.3 A, which the sketch never comes close to: BRIGHTNESS is 80 of 255, so
even with every LED white the draw would be 4.5 A, and the animations light only part
of the LEDs. A 5 V / 10 A supply therefore has plenty of margin. Do not raise
BRIGHTNESS above about 150 with this supply.

For other sizes run
    python3 tools/size_calculator.py --width 60 --height 45 --arrow-width 45 --arrow-height 18
(with no options it prints the values above) and copy the printed constants into the
CONFIGURATION section of sketch.ino. Then run tools/generate_diagram.py.


4. Wiring: what is connected to which pin
-----------------------------------------
The electrical schematic is in wiring-diagram.svg (a drawing with symbols, all ten
strips, the resistors, the switches and the button). The text below says the same.

Overview (5 V power and data):

    230 V AC mains
         |   (fixed wiring by someone who knows mains wiring, inside an enclosure)
    [ Mean Well LRS-50-5 :  5 V, 10 A ]
         | V+                                   | V-
    [ 10 A fuse ]                               |
         |                                      |
         +------ +5 V bus ----+---- GND bus ----+------------------- Arduino GND (any GND pin)
         |                    |                 |
         |            [ 1000 uF capacitor ]     |
         |             (+ to +5 V, stripe = -)  |
         |                                      |
         +--> +5 V of every LED strip piece     +--> GND of every LED strip piece

    Arduino Mega 2560 <--- USB cable (computer or USB charger) : this powers only the Arduino
    Arduino 5 V pin is NOT connected to the LED supply.

Data lines (Arduino pin -> 330 ohm resistor -> DIN of the first LED of the strip):

    Pin   Resistor   Goes to                     Strip (LEDs)
    D4    330 ohm    DIN of arrowCapLeft         arrow, left end cap (4)
    D5    330 ohm    DIN of arrowCapRight        arrow, right end cap (4)
    D6    330 ohm    DIN of frameCw              frame, top edge then right edge (53)
    D7    330 ohm    DIN of frameCcw             frame, left edge then bottom edge (53)
    D8    330 ohm    DIN of the first letter     letters, C ... O, then O P E N (58)
    D9    330 ohm    DIN of arrowBodyTop         arrow, shaft top edge (10)
    D10   330 ohm    DIN of arrowBodyBottom      arrow, shaft bottom edge (10)
    D11   330 ohm    DIN of arrowHeadLeft        arrow, left head (20)
    D12   330 ohm    DIN of arrowHeadRight       arrow, right head (20)
    D31   330 ohm    DIN of closedLight          CLOSED light, 6 white LEDs behind the plate (6)

Inputs (internal pull-up, the sketch uses INPUT_PULLUP):

    D2   -> middle (common) contact of the slide switch
            one outer contact of the slide switch -> Arduino GND
            (handle on the GND side = pin LOW = arrow points left;
             handle on the other side = pin HIGH = arrow points right)
    D3   -> one contact of the push button
            the other contact of the push button -> Arduino GND
            (each press = next letter mode)
    D30  -> middle (common) contact of the slide switch SW5 "casino closed"
            one outer contact of the slide switch -> Arduino GND
            (handle on the GND side = pin LOW = casino closed: the OPEN letters go dark and the
             CLOSED light turns on; handle on the other side = pin HIGH = casino open)
    D22, D23, D24, D25 -> pins 1, 2, 4, 8 of the hex rotary switch SW3 (speed of the frame and the arrow)
    D26, D27, D28, D29 -> pins 1, 2, 4, 8 of the hex rotary switch SW4 (speed of the letters)
    the common pin (C) of each hex switch -> Arduino GND
            (a switch position connects its bit pins to the common pin; the position counts in
             binary, 0..F. The switch has two common pins in the middle: connect one or both to GND.)

I2C LCD 2x16 (LCD1, with an I2C backpack; 4 wires):

    LCD GND  -> Arduino GND
    LCD VCC  -> Arduino 5V pin (about 30 mA with the backlight; the LED supply 5 V is NOT used)
    LCD SDA  -> Arduino D20 (SDA)
    LCD SCL  -> Arduino D21 (SCL)
            (the backpack has a contrast trimmer on it; its I2C address is 0x27 in Wokwi. If a real
             display stays blank, turn the contrast trimmer, then try the address 0x3F in
             LCD_I2C_ADDRESS. The display is optional: if nothing answers at start-up, the sign
             simply runs without it.)

Power for each LED strip (all strips run from the same 5 V supply):

    Strip piece        +5 V and GND connected to the bus at
    frameCw            start of the top edge AND end of the right edge
    frameCcw           start of the left edge AND end of the bottom edge
    letters            every letter group (feed each group from the bus)
    arrowBodyTop       start
    arrowBodyBottom    start
    arrowCapLeft       start
    arrowCapRight      start
    arrowHeadLeft      start of the first piece AND end of the last piece
    arrowHeadRight     start of the first piece AND end of the last piece
    closedLight        start

Where a strip is made of several pieces (the two edges of a frame strip, the four
pieces of an arrowhead, the letter groups), join consecutive pieces with three wires:
DOUT of the earlier piece -> DIN of the next piece, and +5 V and GND of both pieces to
the bus. Always connect the DIN end of a strip to the data line and the DOUT end to
the next piece (the arrow printed on the strip shows the direction).

Notes
  - The Mega's 5 V logic level drives WS2812B directly, no level shifter is needed.
  - Put the 330 ohm resistors close to the first LED of each strip.
  - Use thick wire for the +5 V / GND bus (at least 1 mm2, shorter is better) and
    keep the data wires short.
  - Never connect the LED supply +5 V to the Arduino 5 V pin while USB is connected.
  - Always connect the supply GND to the Arduino GND, otherwise the data lines do
    not work.


5. Parts to buy (partco.fi)
---------------------------
Checked on 2026-11-02 at https://partco.fi. Prices include VAT (25.5 %). Stock
status changes, so check it again before ordering. Plain wire is not listed here.
The complete shopping list (every component, quantities, totals and links) is in parts-list.pdf / .csv / .xlsx.
NOTE: parts-list.* is made for the PLANNED 12 V colour strip (12 V supply, 5 A fuse, 25 V capacitor, strip test parts,
insurance rows); the table below still describes the current 5 V design of the sketch and the drawings.
The prices, stock status and links are a snapshot from 2026-11-02 and are NOT updated unless the user explicitly
asks for it: check them again before ordering.

  Part                  Qty  Product (partco.fi)                         SKU                     Price      Stock (2026-11-02)
  Arduino Mega 2560     1    Arduino Mega 2560 R3 (A000067)              ARD-A000067             58.00 EUR  in stock (15 pcs on 2026-11-02)
                             https://partco.fi/tuote/arduino-mega-2560-r3-a000067-2178
     alternative        1    Seeeduino Mega (ATmega2560)                 SEEED-102010007         46.70 EUR  in stock
                             https://partco.fi/tuote/seeeduino-mega-atmega2560-2531
                             (Mega compatible; set its 5 V / 3.3 V switch to 5 V. I have not checked
                              its pin numbering against the Arduino Mega, so check before you rely on it.)
  LED strip WS2812B     1    RGB-LEDNAUHA 60kpl/m WS2812 5m              LED-N-RGB-W2812X300-5M  136.00 EUR sold out, availability unknown
  60 LEDs/m, 5 V, 5 m        https://partco.fi/tuote/rgb-lednauha-60kpl-m-ws2812-5m-104
                             (5 V, WS2812, LEDs about 17 mm apart, cuttable between LEDs. At the time of
                              checking this was the only suitable strip and it was not in stock.)
     do NOT buy         -    RGB-LEDNAUHA 30kpl/m WS2811 5m              LED-N-RGB-W2812X150-5M  70.37 EUR  in stock, but it is a 12 V
                             https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103         WS2811 strip: it does not work with this 5 V sketch
     smaller alternative-    Digital RGB LED Flexi-Strip 60 LED - 1 Meter  SEEED-104990014       29.92 EUR  sold out, availability unknown
                             https://partco.fi/tuote/digital-rgb-led-flexi-strip-60-led-1-meter-122
                             (you would need four of these)
  5 V power supply      1    HAKKURI MODULI 50W 5VDC 10A (Mean Well)     LRS-50-5                23.79 EUR  in stock
                             https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
                             (a mains-powered module with screw terminals: it needs an enclosure, a mains
                              cord and a plug, and the mains side must be wired by someone who knows how)
  Fuse holder + fuse    1+1  AUTOSULAKEPESÄ JOHDOT PÄÄSTÄ (with leads)   SULPES-APUR             3.56 EUR   in stock
                             https://partco.fi/tuote/autosulakepes%C3%A4-johdot-p%C3%A4%C3%A4st%C3%A4-4732
                             AUTOSULAKE 32V 10A punainen                 SUA-10A                 0.31 EUR   in stock
                             https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  Capacitor             1    ELEKTROLYYTTIKOND. 1000uF 16V 10x16mm       ELKO105-1000U-16V       1.62 EUR   in stock
                             https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-16v-10x16mm-12237
  Slide switch          2    1-NAPAINEN LIUKUKYTKIN ON/ON                KYT-LIU15               0.50 EUR   in stock
  (SPDT, arrow + casino closed) https://partco.fi/tuote/1-napainen-liukukytkin-on-on-4857
  Push button           1    a momentary push button (normally open)     -                       -          not looked up in detail
  (letter mode)              Look in the "Painokytkimet" (pushbuttons) subcategory of Kytkimet:
                             https://partco.fi/kategoria/komponentit/s%C3%A4hk%C3%B6mekaaniset-komponentit/kytkimet
                             (the pushbuttons visible on the first page were latching or illuminated ones;
                              pick a small momentary (ON) button.)
  LCD display           1    LCD-NÄYTTÖ 2x16 SINIVALKO I2C-LIITÄNTÄ      LCD-2X16-I2C            13.40 EUR  in stock (45 pcs)
  (I2C, 2x16, blue)          https://partco.fi/tuote/lcd-n%C3%A4ytt%C3%B6-2x16-sinivalko-i2c-liit%C3%A4nt%C3%A4-5193
                             (Joy-IT SBC-LCD16x2: an I2C backpack with a PCF8574AT is soldered on the back; 4 pins
                              GND, VCC, SDA, SCL; 5 V; contrast trimmer and backlight jumper on the board; module
                              80 x 36 mm. The page says the I2C address is 0x27 and changeable with solder jumpers.)
  Hex rotary switch     2    HEX ENKOODERI/KIERTOKYTKIN TAPILLA 0..F     KIERTOKYTKIN-216        3.43 EUR   in stock (4 pcs)
  (speed, SW3 + SW4)         https://partco.fi/tuote/hex-enkooderi-kiertokytkin-tapilla-0-f-4851
                             (a 16-position, binary coded rotary switch for PCB mounting, body 10 x 10 x 5.5 mm,
                              shaft diameter 3.8 mm, length 7 mm. It is NOT a pulse encoder: it gives a position 0..F
                              on four pins, which is what the speed control uses. The page does not say which pins
                              are which (two common pins in the middle, the rest are the binary pins 1, 2, 4, 8) or
                              whether the code is true or complement coded; test it and set SPEED_BIT_IS_LOW if needed.)
  Knob for the switch   2    a knob that fits a 3.8 mm shaft              -                       -          not looked up
  Perfboard             1    KYTKENTÄLEVY 200x100mm JUOTOSLIUSKOILLA PERTINAX   KL-PL200          6.50 EUR   in stock (10 pcs)
  (control board)            https://partco.fi/tuote/kytkent%C3%A4levy-200x100mm-juotosliuskoilla-pertinax-14786
                             (200 x 100 mm, 2.54 mm pitch, 1.5 mm thick, 1.0 mm holes. The page does not say how the
                              copper strips run. Use it for the ten 330 ohm resistors, the capacitor, the fuse holder,
                              the two hex switches and the connections between the Arduino and the strips.)
  Resistors 330 ohm     10+  through-hole 0.25 W 330 ohm resistors       -                       -          not looked up in detail
  (data lines)               In the Vastukset (resistors) category use the "Resistanssi 330ohm" filter
                             (15 products at the time of checking):
                             https://partco.fi/kategoria/komponentit/passiivikomponentit/vastukset
                             (parts-list.xlsx uses the 2 W type VASTUS-2W-330R, 0.45 EUR, in stock: the 0.5 W type
                              FRES-05W-330R has been discontinued.) Buy at least 12 so that you have spares.
  USB cable A-B         1    A USB A to USB B cable for the Arduino Mega  -                      -          not found on the pages I checked
                             (you probably have one; otherwise see Tietokonekaapelit:
                              https://partco.fi/kategoria/kaapelit/tietokonekaapelit )

Also needed but not looked up: a mains cord with plug and an enclosure for the power
supply, solder and heat-shrink tubing, the plate (with the CLOSED hole text cut out),
frosted plastic to put behind that hole text, and the plexiglass letters of the sign itself. No Raspberry Pi is needed. A computer is only needed to upload the sketch.


6. Strips, LED order and cutting list
-------------------------------------
layout-diagram.svg shows all of this as a drawing: every strip in its place on the
50 x 45 cm plate, with its name, pin, LED count, the LED count of each piece and the
LED index range of each piece. The "letters" strip is ONE strip (one pin, 58 LEDs)
that is cut into 10 pieces, one per letter; the table at the bottom of the drawing
lists the LEDs and index range of every letter.

Every logical strip is its own Adafruit_NeoPixel object with its own data pin. In
diagram.json the first part of each strip gets DIN from the data pin, VDD from 5V and
VSS from GND; the following parts are chained (DOUT -> DIN, VDD.2 -> VDD, VSS.2 -> VSS).

Frame      frameCw: top edge left to right, then right edge downward.
           frameCcw: left edge downward, then bottom edge left to right.
           Both strips have FRAME_HALF_LEDS LEDs. The top and bottom edge have
           FRAME_TOP_LEDS LEDs, the left and right edge FRAME_HALF_LEDS - FRAME_TOP_LEDS.
           The sketch needs FRAME_TOP_LEDS to know where one edge ends and the next
           begins. The corner is left dark: cut each strip at the corner and join the two
           pieces with wires (data, 5 V, GND).
Letters    One group of LEDs per letter, in reading order: C A S I N O, then O P E N.
           Cut the strip between the groups and join them with wire jumpers (data,
           5 V, GND), so no LEDs are wasted between the letters. The LEDs of a group
           can be arranged behind the letter in any shape; the number per letter is
           free (usually 4 to 8). In Wokwi they are drawn as circles only for clarity.
Arrow      Shaft edges run left to right, the end caps run top to bottom. Each
           arrowhead is one strip: base piece above the cap (starting next to the cap
           and going up), the upper wing (to the tip), the lower wing (from the tip),
           and the base piece below the cap (ending next to the cap). The cap is the
           middle of the triangle's base, the two base pieces are the rest of it.
           A strip cannot be bent sharply in its plane, so cut an arrowhead into its four
           pieces and join them with wires at the barbs and at the tip.

CLOSED     closedLight: one piece of 6 LEDs on pin D31. It sits BEHIND the plate under the OPEN
light      row and lights the hole text CLOSED (about 10 x 2.5 cm, cut out of the plate). Put
           frosted plastic between the LEDs and the hole, otherwise the six LEDs show as six
           bright dots. Unlike all other strips these LEDs face the back of the plate.

Cutting list with the default sizes (cut the strip only at the marked cut lines
between two LEDs, every 16.7 mm):
  frameCw          28 (top edge) + 25 (right edge)          2 pieces, 1 joint
  frameCcw         25 (left edge) + 28 (bottom edge)        2 pieces, 1 joint
  letters          6 6 6 4 6 6   6 6 6 6                    10 pieces, 9 joints
  arrowBodyTop     10                                       1 piece
  arrowBodyBottom  10                                       1 piece
  arrowCapLeft     4                                        1 piece
  arrowCapRight    4                                        1 piece
  arrowHeadLeft    2 + 8 + 8 + 2                            4 pieces, 3 joints
  arrowHeadRight   2 + 8 + 8 + 2                            4 pieces, 3 joints
  closedLight      6                                        1 piece
  Together 27 pieces and 238 LEDs.


7. Changing the sizes or the text
---------------------------------
Edit the CONFIGURATION section at the top of sketch.ino:

  FRAME_HALF_LEDS       LEDs in each of the two frame strips
  FRAME_TOP_LEDS        LEDs in the top and bottom edge (the rest goes to the sides);
                        the animation uses it, so it must match the real frame
  LINE_1_TEXT, LINE_2_TEXT   the two text lines (letters A-Z)
  LETTER_LEDS[]         LEDs behind each letter, one entry per letter of line 1
                        followed by line 2. Any number is allowed. Defaults: 6 per
                        letter, 4 for the narrow "I".
  ARROW_BODY_LEDS       LEDs in each shaft edge strip (top and bottom)
  ARROW_CAP_LEDS        LEDs in each shaft end cap (even number)
  ARROW_HEAD_BASE_LEDS  LEDs in each base piece of an arrowhead (above and below the cap)
  ARROW_HEAD_WING_LEDS  LEDs in each wing of an arrowhead
  PIN_SPEED_FRAME_ARROW[]  the four pins of the speed switch SW3 (bit values 1, 2, 4, 8)
  PIN_SPEED_LETTERS[]   the four pins of the speed switch SW4 (bit values 1, 2, 4, 8)
  SPEED_BIT_IS_LOW      true (default): a bit counts as set when its pin reads LOW; false for a
                        complement-coded switch
  SPEED_MIN_MULTIPLIER  speed at switch position 0 (default 0.25 = four times slower)
  SPEED_MAX_MULTIPLIER  speed at switch position F (default 4.0 = four times faster)
  MIN_UPDATE_MS         the frame and the arrow never update more often than this (default 8 ms)
  PIN_CASINO_CLOSED_SWITCH  pin of the casino closed switch SW5 (default D30)
  CASINO_CLOSED_WHEN_LOW    true (default): handle on the GND side = closed; false reverses it
  PIN_CLOSED_LIGHT      data pin of the CLOSED light (default D31)
  STARTUP_TEST_MS       length of the power-on LED test (default 10000; 0 = no test)
  STARTUP_TEST_PASSES   passes of the test (default 2: white, then red-green-blue)
  STARTUP_TEST_WINDOW_LEDS  LEDs lit at the same time in each strip (default 5)
  LCD_TEST_SWAP_MS, LCD_TEST_SCROLL_MS  row swap time (2500) and scroll step (250) of the test screen
  LCD_I2C_ADDRESS       I2C address of the display (0x27 in Wokwi; 0x3F on some backpacks)
  LCD_PAGE_MS           time one LCD page is shown while rotating (default 3000)
  LCD_HOLD_MS           time a changed setting's page stays on screen (default 6000)
  LETTER_START_MODE     letter mode after power-up (default 0)
  LETTER_RANDOM_AUTO_MS time between the random picks of mode 10 (default 120000 = 2 minutes)
  LETTER_SCANNER_MS, LETTER_SPARKLE_STEP_MS, LETTER_CHASE_STEP_MS, LETTER_PULSE_MS   timing of the new modes
  CLOSED_LIGHT_LEDS     LEDs of the CLOSED light (default 6)
  CLOSED_STARTUP_MS, CLOSED_STEADY_MIN_MS / _MAX_MS, CLOSED_FLICKER_MIN_MS / _MAX_MS,
  CLOSED_HOLD_MIN_MS / _MAX_MS    timing of the CLOSED light (pattern milliseconds, see section 8)

tools/size_calculator.py computes the frame and arrow values from the size of the sign.
Then run:  python3 tools/generate_diagram.py
and paste the new diagram.json into Wokwi. Run  python3 tools/generate_schematic.py  and
python3 tools/generate_layout.py  to redraw wiring-diagram.svg, layout-diagram.svg and layout-parts.svg
with the new counts, then  python3 tools/export_pdf.py  for the PDFs. (generate_layout.py takes --leds-per-m if the strip density is not 60.) The animation code derives everything
from the real strip lengths, so it needs no other change. The generator checks that
the LED counts in the diagram match the sketch and prints a warning if the text or the
arrow no longer fits inside the frame or if the arrowhead wings are too short to reach
the tip.


8. Animations and inputs
------------------------
Inputs (internal pull-up resistors, so each input only needs a wire to GND):
  D2  slide switch  arrow direction and frame direction. Handle left = pin 1 shorted to
                    common = LOW = arrow points left. Handle right = open = HIGH =
                    arrow points right. The switch stays in the position you click it
                    to; each click moves the handle to the other side.
                    (Set ARROW_POINTS_LEFT_WHEN_LOW to false to reverse it.)
  D3  push button   each press selects the next letter mode.
  D30 slide switch  casino closed (SW5). Handle on the GND side = pin LOW = casino closed;
                    handle on the other side = casino open (set CASINO_CLOSED_WHEN_LOW to
                    false to reverse it). While the casino is closed the OPEN letters are
                    dark in every letter mode and the CLOSED light is on; everything else
                    (CASINO letters, frame, arrow, speeds) works as usual.
All inputs are debounced in software (DEBOUNCE_MS). The Wokwi Serial Monitor shows
the selected letter mode and the arrow direction.

Speed switches (SW3 and SW4, hex rotary switches with positions 0..F):
  SW3 (D22-D25) sets the speed of the frame and of the arrow, SW4 (D26-D29) the speed of the
  letter patterns. Position 0 is the slowest and F the fastest. The speed multiplier grows
  geometrically from SPEED_MIN_MULTIPLIER to SPEED_MAX_MULTIPLIER (default 0.25 .. 4.0).
  "frame (ms)" and "arrow (ms)" are the time between two animation steps (shorter = faster):

    position          0      1      2      3      4      5      6      7
    speed         x0.25  x0.30  x0.36  x0.44  x0.52  x0.63  x0.76  x0.91
    frame (ms)      400    332    276    230    191    159    132    110
    arrow (ms)      140    116     97     80     67     56     46     38

    position          8      9      A      B      C      D      E      F
    speed         x1.10  x1.32  x1.59  x1.91  x2.30  x2.76  x3.32  x4.00
    frame (ms)       91     76     63     52     44     36     30     25
    arrow (ms)       32     27     22     18     15     13     11      9

  The letter patterns (modes 0-9) run on their own
  clock that advances by the same multiplier, so a letter pattern at x2.00 is twice as fast. Turning
  a switch never makes a pattern jump. The random flicker of one letter keeps its real-time schedule.
  To try other speed ranges change SPEED_MIN_MULTIPLIER and SPEED_MAX_MULTIPLIER (and, for the base
  speed, FRAME_SPEED_MS and ARROW_SPEED_MS) and upload the sketch again. A turning switch passes
  through the positions in between, so a position only counts after it has stayed the same for
  DEBOUNCE_MS. The Serial Monitor prints the position and the multiplier.
  A hex rotary switch is not a pulse encoder: it always tells its position directly, so the speed
  is the same after every power-up and nothing has to be counted.
  In Wokwi a DIP switch bank stands in for the two hex switches (switches 1-4 = SW3, switches 5-8 =
  SW4, ON = bit set; click the part and press keys 1-8). All OFF is position 0, the slowest speed.

Power-on  For 10 seconds after power-up (or a reset, or pressing Play in Wokwi) the sign runs
LED test  an LED test instead of the normal animations. All 10 strips run at the same time; in each
          strip a window of 5 LEDs moves from the first LED to the last one, and all strips
          finish together (a short strip moves slower than a long one). There are 2 passes of
          5 seconds: pass 1 is white, pass 2 lights the LEDs red, green, blue, red, ... (by LED
          number), so a wrong colour order, a swapped strip or a dead LED is easy to see. At most
          5 LEDs per strip (about 50 in total, roughly 0.9 A at BRIGHTNESS 80) are lit at once.
          The LCD shows two rows that swap places every 2.5 seconds, so every character is seen
          on both rows: one row scrolls "press D3 to skip", the other shows the elapsed seconds
          in the middle of a progress bar made of # characters, for example "#####4/10 s.....".
          Pressing the push button D3 during the test skips it; that press does not change the
          letter mode. A button that is already held down at power-up does not skip the test: it
          has to be released and pressed again. The other switches (arrow, casino closed, speeds)
          are read again when the test ends, so they take effect only afterwards, and during the
          test the OPEN letters and the CLOSED light are lit whatever SW5 says. After the test
          the sign starts normally in mode 0. Serial Monitor: "Power-on LED test", then
          "LED test done" or "LED test skipped". Set STARTUP_TEST_MS to 0 to turn the test off.
Frame      5 LEDs white, 5 LEDs dark, repeating along two paths that start in the same
           corner and meet in the opposite corner. Arrow pointing right: they start in
           the top-left corner (one path along the top and right edge, the other along
           the left and bottom edge) and meet in the bottom-right corner. Arrow
           pointing left: they start in the top-right corner (one path along the top
           edge and the left edge, the other along the right and bottom edge) and meet
           in the bottom-left corner. The pattern is computed from the distance from
           the start corner, so any edge length works; it restarts from the new
           corner when the switch is moved.
Letters    Every letter is its own group of LEDs with its own brightness. The button
           steps through eleven modes (after the last one it wraps to mode 0):
             0 breathe     all letters breathe slowly, each slightly out of phase
                           with its neighbour (default)
             1 wave        a glow runs through the letters from left to right
             2 typewriter  letters switch on one by one, hold, then all go off
             3 ignite      letters start up one by one like cold neon tubes,
                           stuttering before they burn steadily
             4 word blink  CASINO stays steady, OPEN blinks
             5 alternate   odd and even letters breathe in opposite phase
             6 scanner     a bright spot sweeps back and forth over the letters, the letters
                           behind it glow on as a tail
             7 sparkle     random letters flash up and fade out softly, like twinkling stars
             8 chase       one letter of each word is dim and the dim spot moves on; CASINO and
                           OPEN run separately (they are different lengths, so they drift apart)
             9 pulse       all letters beat together like a heart: a strong beat, a weaker
                           beat, then a pause
            10 random auto picks a random pattern from modes 0..9 and changes to another random
                           one every LETTER_RANDOM_AUTO_MS (default 120000 = 2 minutes); it
                           never picks the pattern that is showing and never picks itself
                           (the Serial Monitor prints the chosen pattern)
           The sign starts in mode 0 (LETTER_START_MODE). All modes follow the letter speed
           switch SW4, and while the casino is closed the OPEN letters stay dark in every mode
           (in chase and scanner the pattern still runs, the OPEN part is just not visible).
           In every mode one random letter flickers like a faulty neon tube for
           under a second every 5 to 12 seconds.
           Set LETTER_START_MODE to choose the mode after power-up and
           LETTER_MODE_AUTO_MS to change mode automatically (0 = only the button).
LCD        The 2x16 I2C display shows the same information as the Serial Monitor on four pages
           that rotate every 3 seconds (LCD_PAGE_MS):
             page 0  mode:    "Mode 7" / "sparkle"; in random auto "Mode 10 (auto)" / "now: chase"
             page 1  SW3:     "SW3:8  x1.10" / "Fr91ms Ar30ms"   (position, speed multiplier, frame and
                              arrow step time in milliseconds)
             page 2  SW4:     "SW4:8  x1.10" / "Letter patterns"  (position and letter speed multiplier)
             page 3  status:  "Casino: CLOSED" (or OPEN) / "Arrow: <- LEFT" (or "Arrow: RIGHT ->")
           As soon as you change something, its page is shown at once and held for 6 seconds
           (LCD_HOLD_MS) after the last change; the push button and a mode change show page 0, SW3 page
           1, SW4 page 2, the arrow and casino switches page 3. After the hold the pages rotate again
           from the next page. The automatic change of random auto shows page 0 without a hold.
           The text is built in the sketch and only changed characters are sent, a few at a time, so
           the LED animations never wait for the display. The sketch uses only the Wire library that
           is built into the Arduino IDE (no extra library). In Wokwi the LCD is the part "lcd" in
           I2C mode (address 0x27).
CLOSED     Six white LEDs behind the plate light the CLOSED hole text while the casino is
light      closed and are off while it is open. The light behaves like a faulty fluorescent
           tube: when the casino closes it flashes a few times and then stays on; it burns
           steadily for 5 to 14 seconds, then flickers irregularly (full, dim and dark phases)
           for 0.4 to 1.5 seconds, then burns steadily again, and so on. It is always white.
           The timing follows the letter speed switch SW4. The times are the CLOSED_*
           constants at the top of sketch.ino.
Arrow      The shaft is closed at the tail by its end cap and open toward the head: only
           the cap at the tail end is lit, the cap at the head end stays dark. One
           light flows from the middle of the tail cap along the shaft edges, up the
           base pieces and along the wings of the arrowhead into the tip: blue, cyan,
           violet and ice-white colour waves, a shimmer and white sparkles. The top and
           bottom lanes are out of phase. Only the arrowhead the arrow points at is
           lit. The direction follows the slide switch.

Speeds and colours are constants in the CONFIGURATION section (FRAME_SPEED_MS,
LETTERS_SPEED_MS, ARROW_SPEED_MS, BRIGHTNESS, ...). All timing uses millis(); the
sketch never calls delay(). Note that Adafruit_NeoPixel::show() is synchronous, so
the strips are updated one after another. At roughly 30 microseconds per LED the
total update time grows with the LED count (about 7 ms for all 238 default LEDs);
with much larger signs the fastest animation speeds cannot be reached any more.


9. Real hardware notes
----------------------
- Power the strips from the separate 5 V supply, never from the Arduino. Budget up to
  60 mA per LED at full white; BRIGHTNESS and the dark LEDs reduce the real draw.
- Connect the supply GND to the Arduino GND.
- Feed power into each strip at both ends or at several points on long strips (see
  the power table in section 4).
- Put a 330 ohm resistor in series with each data line and a 1000 uF capacitor
  across the supply. The Mega's 5 V logic level drives WS2812B directly, so no
  level shifter is needed.
- Fit the 10 A fuse in the +5 V line right after the supply.
- Connect the common pin of each hex switch to GND. The speed pins use the Arduino's internal
  pull-ups, so no resistors are needed for them.
- The supply has mains-voltage terminals. Mount it in an enclosure and have the
  mains side wired by someone who knows how to do it safely.
