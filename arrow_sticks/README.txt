STICK ARROW - 10 Grove RGB LED sticks as an arrow (design B, barbed head): drill template, layout and wiring
===========================================================================================================

What this is
------------
The arrow of the casino sign made of 10 Grove RGB LED sticks (10x WS2813 mini, 10 LEDs each). This folder holds the
exact drilling template, an indicative layout drawing and the wiring. All three come from one geometry file
(tools/arrow_geometry.py), so the stick names are the same everywhere. The program for the arrow is NOT written yet.
Hardware is proven: stick_test_direct showed that 11 sticks plugged unmodified into the Grove Base Shield and fed
from the external 5 V supply work.

Files
-----
drill-template-A3.pdf        Drill template 1:1 on one A3 landscape page (420 x 297 mm). Print at 100 %.
drill-template-A4-tiled.pdf  The same 1:1 on two A4 landscape pages: left half on page 1, right half on page 2; cut along the dashed CUT LINE, butt the edges together, tape.
layout-diagram.svg/.pdf      Indicative layout (A4 landscape): names, colours, DIN ends and shield connectors.
wiring-diagram.svg/.pdf      Electrical schematic (A4 portrait): supply, Grove Base Shield connectors, the named sticks.
README.txt                   This file (English).   README-fi.txt   The same in Finnish.
Generators: tools/arrow_geometry.py (geometry), tools/generate_arrow_drill_template.py, tools/generate_arrow_layout.py,
tools/generate_arrow_wiring.py. Check of the dimensions: python3 tests/arrow_geometry_test.py

Dimensions (exact; mm)
----------------------
  - Stick: outer body 83 x 10 mm; LED window 74 x 4 mm, centred in the body; 10 LEDs. The LED pitch is taken as
    74 / 10 = 7.4 mm (the datasheet does not state it: check on a real stick). Grove connector in the middle of the stick.
  - 2 mm free space between every two sticks (also between the two wings at the tip).
  - The LED rows of the rails are on LED 3 and LED 8 of the vertical bars (3rd from the top, 3rd from the bottom):
    the two rails are 37 mm apart (centre lines), 27 mm between their inner edges.
  - The vertical bars are 172 mm apart (inner faces), centre to centre 182 mm.
  - Wings at 30 degrees from the horizontal (83 mm wing, 83 mm bar). Each wing starts 2 mm from its bar and the two
    wings of a head are 2 mm apart at the tip, so the wings stand about 10 mm out above and below the bar.
  - Whole arrow: 349.76 x 102.32 mm.
  Drilling positions (centre of the stick = Grove connector = cable hole), mm from the top-left corner of the arrow.
  angle = direction (screen degrees, y down) from the ball end of the stick (the black dot) to its other end:
  L-CAP      x=  83.88  y=  51.16  angle= 90 deg  D2   left vertical bar
  L-WING-UP  x=  38.44  y=  25.08  angle=330 deg  D3   left upper wing
  L-WING-DN  x=  38.44  y=  77.24  angle= 30 deg  D4   left lower wing
  RAIL-UP-L  x= 132.38  y=  32.66  angle=  0 deg  D5   upper rail, left stick
  RAIL-UP-R  x= 217.38  y=  32.66  angle=180 deg  D6   upper rail, right stick
  RAIL-DN-L  x= 132.38  y=  69.66  angle=  0 deg  D7   lower rail, left stick
  RAIL-DN-R  x= 217.38  y=  69.66  angle=180 deg  D8   lower rail, right stick
  R-CAP      x= 265.88  y=  51.16  angle= 90 deg  A0   right vertical bar
  R-WING-UP  x= 311.32  y=  25.08  angle=210 deg  A1   right upper wing
  R-WING-DN  x= 311.32  y=  77.24  angle=150 deg  A2   right lower wing
  The cable hole diameter is a guess (8 mm, must be below the 10 mm stick width): check it against your Grove connector
  (the setting CABLE_HOLE_D is in tools/arrow_geometry.py).
  Black dot = the ball: every real stick has a round marker (the "ball") on its board. Mount every stick with its
  ball at the black dot of the drawing. The dots point the same way as the half of the arrow they are in: on the left
  half they point left (rails, wings: the wings have the dot at the tip), on the right half they point right, and on
  both vertical bars (caps) they point up. The CLOSED stick (A3, behind the plate) is mounted with its ball to the left.
  Whether the ball is at the DIN end (LED 1) of the stick is set by STICK_BALL_AT_DIN_END in sketch.ino (true = the
  ball is at the DIN end, false = at the other end): if the animation runs backwards along the sticks, flip that
  value; do not re-mount the sticks. The drill template is for the FRONT of the plate (print it mirrored if you drill
  from the back).

Wiring (exactly)
----------------
  Each stick is plugged with its own unmodified Grove cable into a Grove connector of the Grove Base Shield on the
  Arduino Mega 2560 (shield switch on 5 V):
    L-CAP D2, L-WING-UP D3, L-WING-DN D4, RAIL-UP-L D5, RAIL-UP-R D6, RAIL-DN-L D7, RAIL-DN-R D8,
    R-CAP A0, R-WING-UP A1, R-WING-DN A2.  (A3 carries the CLOSED light stick of the sign; it is not part of the arrow.)
  The Arduino + shield is fed from the external 5 V supply: LRS-50-5 V+ -> 10 A fuse -> the Mega 5V pins (both, at the
  end of the long header), V- -> the Mega GND pins (both); 1000 uF 25 V capacitor across the supply wires, plus on 5 V.
  No USB cable while the supply is on. The 230 V side of the supply must be in an enclosure and wired by someone who
  knows how. See wiring-diagram.pdf.
  Cable length: the standard Grove cable is short; check that every cable reaches from the shield to its stick.

Parts to buy (partco.fi; snapshot of 2026-11-02, nothing refreshed): see ../parts-list.csv and ../stick_test_direct/README.txt
  - 10 x Grove RGB LED Stick (10x WS2813 mini) with Grove cables (11 owned)
  - Mean Well LRS-50-5 5 V 10 A supply, 10 A blade fuse + holder, capacitor 1000 uF 25 V, Arduino Mega 2560, Grove Base Shield
  - Longer Grove cables if the standard ones do not reach (not looked up)

Open points
-----------
  - LED pitch and the exact position of the Grove connector are assumed (see above); the cable hole diameter is a guess.
  - The older layout drawings of stick_test/ and stick_test_direct/ still show the first sketch of the arrow with 80 mm
    sticks; this folder replaces it.
  - The arrow program (patterns, mapping of the 100 LEDs to the names above) is not written.
