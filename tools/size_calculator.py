#!/usr/bin/env python3
"""Suggest the strip lengths (LED counts) of sketch.ino for a given sign size.

Example (the default values: 50 x 40 cm sign, 40 x 15 cm arrow, 60 LEDs per metre):

    python3 tools/size_calculator.py
    python3 tools/size_calculator.py --width 60 --height 45 --arrow-width 45 --arrow-height 18

The script prints the constants to copy into the CONFIGURATION section of
sketch.ino. Afterwards run tools/generate_diagram.py to update the Wokwi diagram.

Model (the same ring layout the sketch and the Wokwi diagram use; 1 pitch = the
distance between two LEDs = 100 / leds-per-m centimetres):
  * Frame: the outer edge of the LED ring is the edge of the plate. The ring has
    FRAME_TOP_LEDS + 2 columns and FRAME_HALF_LEDS - FRAME_TOP_LEDS + 2 rows,
    with an empty corner cell between neighbouring strips.
  * Arrow: height = ARROW_CAP_LEDS + 2 * ARROW_HEAD_BASE_LEDS + 2 pitches,
    width = (ARROW_BODY_LEDS + 1) pitches + twice the depth of an arrowhead.
The results are rounded to whole LEDs, so the real sizes differ from the wanted
ones by up to about one pitch.
"""

import argparse
import math


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--width", type=float, default=50, help="sign width in cm (default 50)")
    parser.add_argument("--height", type=float, default=45, help="sign height in cm (default 45)")
    parser.add_argument("--arrow-width", type=float, default=40, help="arrow width in cm (default 40)")
    parser.add_argument("--arrow-height", type=float, default=15, help="arrow height in cm (default 15)")
    parser.add_argument("--leds-per-m", type=float, default=60, help="LED density of the strip (default 60)")
    parser.add_argument("--cap", type=int, default=4, help="LEDs in each arrow end cap, even (default 4)")
    parser.add_argument("--letter-leds", default="6,6,6,4,6,6,6,6,6,6",
                        help="LEDs behind each letter, comma separated (default 6,6,6,4,6,6,6,6,6,6)")
    parser.add_argument("--closed-leds", type=int, default=6,
                        help="white LEDs of the CLOSED light behind the plate (default 6)")
    parser.add_argument("--brightness", type=int, default=80, help="BRIGHTNESS used in the sketch (default 80)")
    args = parser.parse_args()
    if args.cap < 2 or args.cap % 2:
        parser.error("--cap must be an even number of at least 2")

    pitch_cm = 100.0 / args.leds_per_m

    def half_up(x):
        return int(math.floor(x + 0.5))

    # Frame
    top = half_up(args.width / pitch_cm) - 2
    side = half_up(args.height / pitch_cm) - 2
    if top < 2 or side < 2:
        parser.error("The sign is too small for this LED density")
    half = top + side

    # Arrow
    height_pitches = args.arrow_height / pitch_cm
    base = max(1, half_up((height_pitches - 2 - args.cap) / 2))
    arrow_height_pitches = args.cap + 2 * base + 2
    half_height = arrow_height_pitches / 2.0
    # Smallest wing for which the arrowhead is about 0.6 times as deep as the arrow is high.
    wing = max(int(half_height) + 1, math.ceil(math.hypot(half_height, 0.6 * arrow_height_pitches)))
    reach = math.sqrt(wing ** 2 - half_height ** 2)          # depth of one arrowhead
    body = max(1, int(math.floor(args.arrow_width / pitch_cm - 2 * reach)) - 1)
    arrow_width_pitches = (body + 1) + 2 * reach

    letters = [int(v) for v in args.letter_leds.split(",") if v.strip()]
    arrow_leds = 2 * body + 2 * args.cap + 4 * (base + wing)
    total = 2 * half + sum(letters) + arrow_leds + args.closed_leds
    worst_amps = total * 0.06
    typical_amps = worst_amps * args.brightness / 255.0

    print("LED pitch: %.2f mm (%g LEDs per metre)" % (pitch_cm * 10, args.leds_per_m))
    print()
    print("Copy into the CONFIGURATION section of sketch.ino:")
    print("  constexpr uint16_t FRAME_HALF_LEDS = %d;" % half)
    print("  constexpr uint16_t FRAME_TOP_LEDS = %d;" % top)
    print("  constexpr uint16_t ARROW_BODY_LEDS = %d;" % body)
    print("  constexpr uint16_t ARROW_CAP_LEDS = %d;" % args.cap)
    print("  constexpr uint16_t ARROW_HEAD_BASE_LEDS = %d;" % base)
    print("  constexpr uint16_t ARROW_HEAD_WING_LEDS = %d;" % wing)
    print()
    print("Resulting sizes (before rounding the wanted size was %g x %g cm and %g x %g cm):"
          % (args.width, args.height, args.arrow_width, args.arrow_height))
    print("  frame, outer edge of the LED ring: %.1f x %.1f cm  (top/bottom edge %d LEDs, left/right edge %d LEDs)"
          % ((top + 2) * pitch_cm, (side + 2) * pitch_cm, top, side))
    print("  arrow: %.1f x %.1f cm" % (arrow_width_pitches * pitch_cm, arrow_height_pitches * pitch_cm))
    print()
    print("  constexpr uint16_t CLOSED_LIGHT_LEDS = %d;" % args.closed_leds)
    print()
    print("LED totals: frame %d, letters %d, arrow %d, CLOSED light %d, all together %d LEDs"
          % (2 * half, sum(letters), arrow_leds, args.closed_leds, total))
    print("  strip length needed: %.2f m of %g LEDs/m (plus some spare for cutting mistakes)"
          % (total * pitch_cm / 100.0, args.leds_per_m))
    print("  current: worst case all LEDs full white at full brightness %.1f A;" % worst_amps)
    print("           at BRIGHTNESS %d all LEDs white %.1f A (the real animations use less)" % (args.brightness, typical_amps))


if __name__ == "__main__":
    main()
