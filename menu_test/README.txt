MENU TEST - the settings menu of Casino Open Sign V2 on its real controls and an I2C LCD
========================================================================================

Purpose
-------
Try the planned settings menu (see ../RDD-REQUIREMENTS.md REQ-012..017) on real hardware before it goes into the sign:
two hex rotary switches, the OK / BACK / SHIFT buttons, two slide switches and the 2x16 LCD. No LED strips or sticks are
connected; the Mega is powered over USB. The menu content comes from ../tools/menu-editor/menu-v2.json (edit it with the
Menu Editor); sketch.ino is generated from it (node tools/generate_menu_test.js). This test does NOT change the sign program
(casino_open_sign_v2/): nothing of the menu is in the sign yet.

Controls
--------
  Program hex switch   legs 1, 2, 4, 8 on D34-D37, one C leg to GND. Absolute position 0..F = the program number on the
                       status page. It is read all the time; turning it while the menu is open closes the menu.
  Menu hex switch      legs 1, 2, 4, 8 on D42-D45, one C leg to GND. Free-running: turning it moves the selection or
                       changes the value (direction and steps from the position change, as in hex_rotary_test).
  OK D48, BACK D49, SHIFT D50   push buttons to GND. SHIFT held while turning makes the step ten times larger.
  Arrow slide switch   D32 (middle contact), side contact to GND. Handle on the GND side = arrow points left.
  Casino slide switch  D30 (middle contact), side contact to GND. Handle on the GND side = casino closed.
  LCD                  I2C backpack 0x27: GND, VCC to 5V, SDA D20, SCL D21.
Exact wiring: wiring-diagram.svg/.pdf (schematic) and layout-diagram.svg/.pdf (Mega header pins and a table).

What you see and do
-------------------
  Status page   row 0 "#<program> Casino Open|Closed", row 1 arrow + "Customer right|left". In program F row 1 changes every
                3 seconds to "Change in M:SS" (the countdown is only simulated here, from the "Change delay" property).
  OK            opens the menu at the root (title "Menu" and a countdown "NNs"). In a list: OK enters a submenu or starts
                editing a value. BACK goes up one level, or leaves the menu at the root.
  Menu switch   moves the selection (the list stops at both ends), or changes the value while editing (live; the Serial
                Monitor prints a "CB ..." line for every callback). OK saves the value to EEPROM, BACK cancels (old value).
  Restore defaults  asks "OK=yes BACK=no" (the question "Restore defaults?" is 17 characters and scrolls on the 16-character row).
  Time-out      the menu closes after 60 s without input, only at the root list (the seconds left are shown).
  Switches      changing the arrow or casino switch (or the program switch) while the menu is open closes it / cancels an edit.
                "Arrow reverse" and "Casino reverse" swap what the slide switches mean, at once.
  Long texts    a row that does not fit waits 1 s and then scrolls one character per 400 ms.
The arrows on row 1 are the LCD's ROM characters 0x7E / 0x7F. If your display shows other symbols there, tell the AI
(the menu JSON then gets "arrowAscii": true and the status page uses "<-" / "->").

Settings in sketch.ino (CONFIGURATION at the top)
-------------------------------------------------
  HEX_BIT_IS_LOW (true: a leg connected to C is a set bit), MENU_SWITCH_REVERSED (turning direction),
  ARROW_POINTS_LEFT_WHEN_LOW, CASINO_CLOSED_WHEN_LOW, pins, debounce times, LCD_I2C_ADDRESS, MENU_EEPROM_BASE.
Wrong turning direction: set MENU_SWITCH_REVERSED to true. Position 0 reads as F: set HEX_BIT_IS_LOW to false.

What to try and write down
--------------------------
  1. Status page: do the program number, Casino and the arrow follow the three switches (and the Reverse properties)?
  2. OK opens the menu; does the menu switch move one item per click, in the right direction, and stop at the ends?
  3. Edit a value (for example Brightness > Frame > Max): does it change live, does SHIFT make steps of 10, does OK save,
     does BACK restore? Power the Mega off and on: is the saved value still there (Serial Monitor "EEPROM: saved ...")?
  4. Restore defaults: confirm page, OK = yes, BACK = no.
  5. Do the arrow symbols on row 1 look like arrows? (Otherwise "arrowAscii".)
  6. Is the long text scrolling readable (for example "Restore defaults?")?
  7. Turn the program switch to F: row 1 alternates with "Change in M:SS".

Files
-----
sketch.ino              The test program (generated; engine in ../tools/menu_engine/engine.inc, shell in test_shell.ino.in).
diagram.json            Wokwi circuit (DIP banks stand in for the rotary switches).
wiring-diagram.svg/.pdf Electrical schematic (A4).   layout-diagram.svg/.pdf  Header pins and wiring table (A4 landscape).
README.txt              This file.   README-fi.txt  The same in Finnish.
No extra library: only Wire and EEPROM, both built into the Arduino IDE.
Host tests (project root): tests/menu_diff_test.py (JS engine versus C++ engine, random events), tests/menu_engine_harness.cpp
with tests/menu_engine_checks.inc (EEPROM image, texts, pins, debounce, sign of life) and tests/mutate_menu_engine.py.

Sign of life (rule for the whole project)
-----------------------------------------
  - The LED on pin 13 blinks all the time (500 ms), is shortly inverted (80 ms) at every key or switch event and blinks
    four times faster (125 ms) while many events happen.
  - Serial Monitor speed: 115200 baud (SERIAL_BAUD at the top of sketch.ino, printed at start-up). Once per second:
      DBG up=12 mode=S prog=F arrow=R casino=O menu=3 keys=5 cb=2 eeprom=saved lcd=1
    mode S status / M menu / E edit / C confirm, prog hex, arrow L/R, casino C/O, menu = menu switch steps, keys = OK/BACK
    presses, cb = callbacks, eeprom saved or defaults, lcd = 1 while the display answers.
  - At start-up an I2C scan lists the devices and warns if nothing answers on the LCD address.
  - The display is checked all the time: retried every second when lost and fully re-initialised every 5 s (LCD_REINIT_MS).
    If the LCD shows light but no text, turn the contrast trimmer on the backpack and check that VCC measures at least 4.75 V.
