#!/usr/bin/env python3
"""Generate sticks_11_test/diagram.json (Wokwi simulation of the 11 sticks test).

    python3 tools/generate_sticks_11_test_diagram.py

Same circuit as in the simulator of stick_test: 11 sticks as wokwi-led-strip parts with 10 pixels, powered from
the simulated Mega 5V pin. On real hardware the sticks are plugged into the Grove shield (see README.txt).
"""

import json
from pathlib import Path

OUTPUT = Path(__file__).resolve().parent.parent / "sticks_11_test" / "diagram.json"
PINS = ["2", "3", "4", "5", "6", "7", "8", "A0", "A1", "A2", "A3"]


def main():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -60, "left": 140,
         "attrs": {"text": "11 sticks test: 10 arrow sticks flow + the CLOSED light stick on A3 steady; pin 13 LED blinks all the time"}},
        {"type": "wokwi-text", "id": "t2", "top": -42, "left": 140,
         "attrs": {"text": "Real hardware: sticks in the Grove shield, external 5 V to the Mega 5V/GND pins"}},
        {"type": "wokwi-pushbutton", "id": "btn", "top": -110, "left": -70,
         "attrs": {"color": "green", "label": "Mode (D33)"}},
    ]
    connections = [
        ["mega:33", "btn:1.r", "orange", []],
        ["mega:GND.1", "btn:2.r", "black", []],
    ]
    for i, pin in enumerate(PINS):
        n = i + 1
        parts.append({"type": "wokwi-led-strip", "id": "s%d" % n, "top": 232 + i * 70, "left": 60, "rotate": 0,
                      "attrs": {"pixels": "10", "pixelSize": "5050"}})
        connections += [
            ["mega:%s" % pin, "s%d:DIN" % n, "green", []],
            ["mega:5V", "s%d:VDD" % n, "red", []],
            ["mega:GND.1", "s%d:VSS" % n, "black", []],
        ]
    doc = {"version": 1, "author": "Casino sign project - 11 sticks test", "editor": "wokwi",
           "parts": parts, "connections": connections, "dependencies": {}}
    OUTPUT.write_text(json.dumps(doc, indent=2) + "\n")
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
