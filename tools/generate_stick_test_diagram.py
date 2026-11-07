#!/usr/bin/env python3
"""Generate stick_test/diagram.json (Wokwi simulation of the stick test).

    python3 tools/generate_stick_test_diagram.py

In the simulation the sticks are wokwi-led-strip parts with 10 pixels each, powered from the simulated
Mega 5V pin. On real hardware the sticks use a separate 5 V supply (see stick_test/README.txt).
"""

import json
from pathlib import Path

OUTPUT = Path(__file__).resolve().parent.parent / "stick_test" / "diagram.json"
PINS = ["2", "3", "4", "5", "6", "7", "8", "A0", "A1", "A2"]


def main():
    parts = [
        {"type": "wokwi-arduino-mega", "id": "mega", "top": 0, "left": 0, "attrs": {}},
        {"type": "wokwi-text", "id": "t1", "top": -60, "left": 140,
         "attrs": {"text": "Stick test: 10 sticks x 10 LEDs, Serial Monitor (115200) shows the phases"}},
        {"type": "wokwi-text", "id": "t2", "top": -42, "left": 140,
         "attrs": {"text": "Real hardware: sticks on a separate 5 V supply, common GND"}},
        {"type": "wokwi-pushbutton", "id": "btn", "top": -110, "left": -70,
         "attrs": {"color": "green", "label": "Skip phase (D22)"}},
    ]
    connections = [
        ["mega:22", "btn:1.r", "orange", []],
        ["mega:GND.1", "btn:2.r", "black", []],
    ]
    for i, pin in enumerate(PINS):
        n = i + 1
        top = 232 + i * 70
        parts.append({"type": "wokwi-resistor", "id": "r%d" % n, "top": top - 20, "left": 120,
                      "attrs": {"value": "330"}})
        parts.append({"type": "wokwi-led-strip", "id": "s%d" % n, "top": top, "left": 60, "rotate": 0,
                      "attrs": {"pixels": "10", "pixelSize": "5050"}})
        connections += [
            ["mega:%s" % pin, "r%d:1" % n, "green", []],
            ["r%d:2" % n, "s%d:DIN" % n, "green", []],
            ["mega:5V", "s%d:VDD" % n, "red", []],
            ["mega:GND.1", "s%d:VSS" % n, "black", []],
        ]
    doc = {"version": 1, "author": "Casino sign project - stick test", "editor": "wokwi",
           "parts": parts, "connections": connections, "dependencies": {}}
    OUTPUT.write_text(json.dumps(doc, indent=2) + "\n")
    print("Wrote %s" % OUTPUT)


if __name__ == "__main__":
    main()
