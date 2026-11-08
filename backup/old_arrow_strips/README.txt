BACKUP - the old arrow made of six LED strips (retired, NOT wired any more)
===========================================================================

What this is
------------
The casino sign as it was before the Grove stick arrow: sketch.ino and diagram.json of that version. It is kept only as
a backup list ("memory") of the old design. Do not wire it: its pins collide with the Grove Base Shield connectors that
the new arrow uses (the new sign uses D2-D8 and A0-A2 for the sticks).

Files
-----
sketch.ino     The old sign sketch (arrow = six WS2812B strips). Opens in Wokwi together with diagram.json.
diagram.json   The matching Wokwi circuit.
README.txt     This file (English).   README-fi.txt   The same in Finnish.

Retired parts of the old arrow (strip, data pin, LEDs)
------------------------------------------------------
  arrowBodyTop      D9    10 LEDs   shaft top edge
  arrowBodyBottom   D10   10 LEDs   shaft bottom edge
  arrowHeadLeft     D11   20 LEDs   left arrowhead (pieces 2 + 8 + 8 + 2)
  arrowHeadRight    D12   20 LEDs   right arrowhead (pieces 2 + 8 + 8 + 2)
  arrowCapLeft      D4     4 LEDs   left end cap
  arrowCapRight     D5     4 LEDs   right end cap
  Together 68 LEDs, 6 strips. Each strip had a 330 ohm resistor in its data line.

Old pins of the rest of that version: frame D6/D7, letters D8, CLOSED light D31, arrow switch D2, button D3.
(The new sign: frame D9/D10, letters D11, CLOSED light D31, arrow switch D32, button D33.)

Using the backup
----------------
Only for looking at or running in the Wokwi simulator: new project -> Arduino Mega, paste sketch.ino and diagram.json
into the tabs of the same name, add the library Adafruit NeoPixel. Nothing here needs buying.
