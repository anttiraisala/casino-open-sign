"""Geometry of the stick arrow (design B, barbed head) made of 10 Grove RGB LED sticks.

Single source for the drill template, the layout diagram and the wiring diagram, so that the three drawings always
show the same sticks with the same names. All lengths are millimetres, screen coordinates (x to the right, y down).

Stick (Grove RGB LED Stick, 10x WS2813 mini):
  - outer body 83 x 10 mm, LED window 74 x 4 mm centred in the body, 10 LEDs evenly spaced in the window
    (pitch 74 / 10 = 7.4 mm: this is an assumption, the datasheet does not state the pitch)
  - Grove connector in the middle of the stick, on the back.
Arrow:
  - two vertical bars (caps), four horizontal rails (two on the top rail, two on the bottom rail) and four wings
    (two at each cap, 30 degrees, forming the arrow heads); 2 mm of free space between every two sticks.
  - the LED rows of the rails lie on LED 3 and LED 8 of the caps (3rd from the top and 3rd from the bottom).
"""

import math

BODY_L, BODY_W = 83.0, 10.0          # outer size of a stick
LED_L, LED_W = 74.0, 4.0             # LED window
LEDS = 10
LED_PITCH = LED_L / LEDS             # 7.4 mm (assumed: evenly spaced over the window)
GAP = 2.0                            # free space between two sticks
WING_ANGLE = 30.0                    # degrees between a wing and the horizontal
CABLE_HOLE_D = 8.0                   # diameter of the cable hole drilled at the middle of each stick (to be checked; must stay below the 10 mm stick width)

# Names (used in all drawings), colours as in the owner's sketch, Grove Base Shield connector for each stick.
# fill, stroke colours per stick group
GREY = ("#d9d9d9", "#6d6d6d")
BLUE = ("#c5d8ff", "#1f4ed8")
RED = ("#ffd0d0", "#d11a1a")
GREEN = ("#d3f7c0", "#3a9a00")
PURPLE = ("#e6d0ff", "#7a1fd1")

STICK_DEFS = [
    # name,         description,                          colours, shield connector (signal pin)
    ("L-CAP",       "left vertical bar",                  BLUE,   "D2"),
    ("L-WING-UP",   "left upper wing",                    GREY,   "D3"),
    ("L-WING-DN",   "left lower wing",                    GREY,   "D4"),
    ("RAIL-UP-L",   "upper rail, left stick",             RED,    "D5"),
    ("RAIL-UP-R",   "upper rail, right stick",            RED,    "D6"),
    ("RAIL-DN-L",   "lower rail, left stick",             RED,    "D7"),
    ("RAIL-DN-R",   "lower rail, right stick",            RED,    "D8"),
    ("R-CAP",       "right vertical bar",                 GREEN,  "A0"),
    ("R-WING-UP",   "right upper wing",                   PURPLE, "A1"),
    ("R-WING-DN",   "right lower wing",                   PURPLE, "A2"),
]
SPARE_CONNECTOR = "A3"               # the 11th connector of the shield: the CLOSED light stick, not part of the arrow


def _rect_corners(cx, cy, theta_deg, length, width):
    """Corners of a rectangle with its long axis in direction theta (degrees, screen coordinates)."""
    t = math.radians(theta_deg)
    ux, uy = math.cos(t), math.sin(t)
    vx, vy = -uy, ux
    pts = []
    for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        pts.append((cx + su * length / 2 * ux + sv * width / 2 * vx,
                    cy + su * length / 2 * uy + sv * width / 2 * vy))
    return pts


class Stick:
    def __init__(self, name, description, colours, connector, cx, cy, theta, din_dir):
        self.name, self.description, self.connector = name, description, connector
        self.fill, self.stroke = colours
        self.cx, self.cy, self.theta = cx, cy, theta         # centre and direction of the long axis (degrees)
        self.din_dir = din_dir                               # unit vector from the centre towards the ball end (the black dot; LED 1 when STICK_BALL_AT_DIN_END is true)

    def body(self):
        return _rect_corners(self.cx, self.cy, self.theta, BODY_L, BODY_W)

    def window(self):
        return _rect_corners(self.cx, self.cy, self.theta, LED_L, LED_W)

    def led_centre(self, k):
        """Centre of LED number k (1..10; LED 1 is at the ball end, the black dot)."""
        s = -LED_L / 2 + LED_PITCH * (k - 0.5)
        return (self.cx + self.din_dir[0] * -s, self.cy + self.din_dir[1] * -s)


def build():
    """Returns the 10 sticks in arrow coordinates: the left cap is at x = 0, the centre line of the arrow is y = 0."""
    a = math.radians(WING_ANGLE)
    sticks = {}
    cap_gap_x = BODY_W / 2 + GAP + BODY_W / 2 + BODY_L * 2 + GAP * 1   # (not used directly, kept for clarity)
    xl = 0.0
    rail_y = LED_PITCH * 2.5                       # LED 3 (and LED 8) of a cap are 18.5 mm from its centre
    x_rail1 = xl + BODY_W / 2 + GAP + BODY_L / 2
    x_rail2 = x_rail1 + BODY_L + GAP
    xr = x_rail2 + BODY_L / 2 + GAP + BODY_W / 2
    defs = {d[0]: d for d in STICK_DEFS}

    def make(name, cx, cy, theta, din):
        d = defs[name]
        sticks[name] = Stick(d[0], d[1], d[2], d[3], cx, cy, theta, din)

    make("L-CAP", xl, 0.0, 90.0, (0.0, -1.0))
    make("R-CAP", xr, 0.0, 90.0, (0.0, -1.0))
    make("RAIL-UP-L", x_rail1, -rail_y, 0.0, (-1.0, 0.0))
    make("RAIL-UP-R", x_rail2, -rail_y, 180.0, (1.0, 0.0))
    make("RAIL-DN-L", x_rail1, rail_y, 0.0, (-1.0, 0.0))
    make("RAIL-DN-R", x_rail2, rail_y, 180.0, (1.0, 0.0))

    # Left upper wing: end E1 next to the cap, runs down and to the left (direction d) with its other end at the tip.
    d = (-math.cos(a), math.sin(a))
    n_in = (math.sin(a), math.cos(a))              # towards the inside of the arrow head
    e1x = xl - BODY_W / 2 - GAP - (BODY_W / 2) * n_in[0]           # inner corner of the end face GAP from the cap
    e1y = -GAP / 2 - BODY_L * math.sin(a) - (BODY_W / 2) * n_in[1]  # wing and its mirror image GAP apart at the tip
    cx, cy = e1x + d[0] * BODY_L / 2, e1y + d[1] * BODY_L / 2
    theta = math.degrees(math.atan2(d[1], d[0])) + 180.0            # 330 degrees: from the ball end (tip) to the cap end
    din = (d[0], d[1])                                              # the ball (LED 1) end is the TIP end of the wing
    make("L-WING-UP", cx, cy, theta, din)
    make("L-WING-DN", cx, -cy, -theta, (din[0], -din[1]))
    # right wings: mirror images about the vertical centre line of the arrow
    xm = (xl + xr) / 2
    make("R-WING-UP", 2 * xm - cx, cy, 180.0 - theta, (-din[0], din[1]))
    make("R-WING-DN", 2 * xm - cx, -cy, 180.0 + theta, (-din[0], -din[1]))
    return [sticks[d[0]] for d in STICK_DEFS]


def bounding_box(sticks):
    xs = [p[0] for s in sticks for p in s.body()]
    ys = [p[1] for s in sticks for p in s.body()]
    return min(xs), min(ys), max(xs), max(ys)


def shifted(sticks):
    """The same sticks moved so that the bounding box starts at (0, 0) (template coordinates); returns (sticks, w, h)."""
    x0, y0, x1, y1 = bounding_box(sticks)
    out = []
    for s in sticks:
        out.append(Stick(s.name, s.description, (s.fill, s.stroke), s.connector, s.cx - x0, s.cy - y0, s.theta, s.din_dir))
    return out, x1 - x0, y1 - y0
