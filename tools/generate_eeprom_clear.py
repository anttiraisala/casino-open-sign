#!/usr/bin/env python3
"""Generate the drawing and the Wokwi circuit of the EEPROM clear tool (eeprom_clear/).

    python3 tools/generate_eeprom_clear.py

Output in eeprom_clear/: diagram.json (Wokwi: only the Mega) and wiring-diagram.svg (A4 landscape: the tool needs no wiring, only the
USB cable between the PC and the Mega). The PDF is made by tools/export_pdf.py.
"""

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_schematic import GREY, RED, Svg

OUT = Path(__file__).resolve().parent.parent / "eeprom_clear"


def wokwi():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -60, "left": 20,
         "attrs": {"text": "EEPROM clear tool: sets every EEPROM byte to 0xFF once at start-up (nothing is connected)"}},
        {"type": "wokwi-text", "id": "t2", "top": -42, "left": 20,
         "attrs": {"text": "Pin 13 LED: slow blink = cleared, fast blink = a byte could not be cleared. Serial 115200 baud."}},
    ]
    doc = {"version": 1, "author": "Casino sign project - EEPROM clear tool", "editor": "wokwi",
           "parts": parts, "connections": [], "dependencies": {}}
    (OUT / "diagram.json").write_text(json.dumps(doc, indent=2) + "\n")
    print("Wrote %s" % (OUT / "diagram.json"))


def wiring():
    width, height = 940, 560
    svg = Svg(width, height)
    svg.text(width / 2, 24, "EEPROM clear tool - wiring", size=17, anchor="middle", weight="bold")
    svg.text(width / 2, 44, "nothing is wired: only the USB cable between the PC and the Mega", size=12, anchor="middle", color=GREY)

    # PC
    svg.rect(60, 110, 200, 150, fill="#f3f3f3")
    svg.text(160, 170, "PC", size=15, anchor="middle", weight="bold")
    svg.text(160, 192, "Arduino IDE", size=11, anchor="middle")
    svg.text(160, 210, "Serial Monitor", size=11, anchor="middle")
    svg.text(160, 226, "115200 baud", size=11, anchor="middle")
    svg.text(60, 280, "USB port of the PC", size=11, color=GREY)

    # USB cable
    svg.circle(260, 185, 3.5, RED)
    svg.line(260, 185, 540, 185, RED, 3)
    svg.circle(540, 185, 3.5, RED)
    svg.text(400, 172, "USB cable (power, upload, Serial Monitor)", size=12, anchor="middle")
    svg.text(400, 204, "USB-B plug into the Mega's USB connector", size=10, anchor="middle", color=GREY)

    # Mega
    svg.rect(540, 90, 340, 200, fill="#e8f0fe")
    svg.text(710, 118, "Arduino Mega 2560", size=14, anchor="middle", weight="bold")
    svg.text(710, 150, "USB connector", size=12, anchor="middle")
    svg.text(710, 186, "No pin is used.", size=12, anchor="middle")
    svg.text(710, 204, "Nothing needs to be connected;", size=12, anchor="middle")
    svg.text(710, 222, "the sign hardware may stay plugged in.", size=12, anchor="middle")
    svg.text(710, 270, "Pin 13: onboard LED = sign of life", size=11, anchor="middle", color=GREY)

    notes = [
        "How to use",
        "1. Connect the USB cable (the sign hardware may stay connected: the tool does not use any pin).",
        "2. Upload eeprom_clear/sketch.ino. At start-up it sets every EEPROM byte to 0xFF, once.",
        "3. Serial Monitor at 115200 baud: 'EEPROM cleared: all bytes are 0xFF.' and one DBG line per second. LED on pin 13 blinks slowly.",
        "4. A fast blink and 'EEPROM clear FAILED' mean that some byte could not be cleared (a worn-out EEPROM).",
        "5. Upload the sign program again: it finds no saved settings and starts with the default values of the menu.",
    ]
    y = 340
    svg.text(60, y, notes[0], size=13, weight="bold")
    for k, line in enumerate(notes[1:]):
        svg.text(60, y + 24 + k * 20, line, size=11)
    svg.line(60, 480, 100, 480, RED, 2.5)
    svg.text(108, 484, "USB cable", size=11)
    (OUT / "wiring-diagram.svg").write_text(svg.render_a4(landscape=True))
    print("Wrote %s" % (OUT / "wiring-diagram.svg"))


def main():
    wokwi()
    wiring()


if __name__ == "__main__":
    main()
