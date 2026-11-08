"""Checks the dimensions of the stick arrow (tools/arrow_geometry.py). The expected values are written out here.

    python3 tests/arrow_geometry_test.py
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import arrow_geometry as g

fails = 0


def check(cond, msg):
    global fails
    if not cond:
        print("FAIL:", msg)
        fails += 1


def seg_dist(p, a, b):
    ax, ay, bx, by = a[0], a[1], b[0], b[1]
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(p[0] - (ax + t * dx), p[1] - (ay + t * dy))


def inside(p, poly):
    sign = 0
    for i in range(4):
        a, b = poly[i], poly[(i + 1) % 4]
        c = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])
        if abs(c) < 1e-12:
            continue
        if sign == 0:
            sign = 1 if c > 0 else -1
        elif (c > 0) != (sign > 0):
            return False
    return True


def distance(p, q):
    """Distance between two convex quadrilaterals (0 when they touch or overlap)."""
    if any(inside(v, q) for v in p) or any(inside(v, p) for v in q):
        return 0.0
    best = 1e9
    for poly_a, poly_b in ((p, q), (q, p)):
        for v in poly_a:
            for i in range(4):
                best = min(best, seg_dist(v, poly_b[i], poly_b[(i + 1) % 4]))
    return best


sticks = g.build()
by = {s.name: s for s in sticks}
check(len(sticks) == 10 and len(by) == 10, "ten sticks with different names")
check(sorted(s.connector for s in sticks) == sorted(["D2", "D3", "D4", "D5", "D6", "D7", "D8", "A0", "A1", "A2"]),
      "ten different shield connectors D2-D8, A0-A2")
check(g.BODY_L == 83 and g.BODY_W == 10 and g.LED_L == 74 and g.LED_W == 4 and g.GAP == 2 and g.WING_ANGLE == 30,
      "stick 83 x 10 mm, LED window 74 x 4 mm, gap 2 mm, wing angle 30 degrees")

# body and LED window sizes and centring
for s in sticks:
    b, w = s.body(), s.window()
    check(abs(math.dist(b[0], b[1]) - 83) < 1e-9 and abs(math.dist(b[1], b[2]) - 10) < 1e-9, s.name + ": body 83 x 10")
    check(abs(math.dist(w[0], w[1]) - 74) < 1e-9 and abs(math.dist(w[1], w[2]) - 4) < 1e-9, s.name + ": window 74 x 4")
    cb = (sum(p[0] for p in b) / 4, sum(p[1] for p in b) / 4)
    cw = (sum(p[0] for p in w) / 4, sum(p[1] for p in w) / 4)
    check(math.dist(cb, cw) < 1e-9, s.name + ": window centred in the body")

# gaps: at least 2 mm everywhere, exactly 2 mm between the rails and the caps, between the rails, and at the tips
min_gap = 1e9
for i in range(10):
    for j in range(i + 1, 10):
        d = distance(sticks[i].body(), sticks[j].body())
        min_gap = min(min_gap, d)
        check(d >= 2.0 - 1e-9, "gap %s - %s is %.3f mm (less than 2)" % (sticks[i].name, sticks[j].name, d))
check(abs(min_gap - 2.0) < 1e-9, "the smallest gap is exactly 2 mm (%.6f)" % min_gap)
for a, b in (("L-CAP", "RAIL-UP-L"), ("L-CAP", "RAIL-DN-L"), ("RAIL-UP-L", "RAIL-UP-R"), ("RAIL-DN-L", "RAIL-DN-R"),
             ("RAIL-UP-R", "R-CAP"), ("RAIL-DN-R", "R-CAP"), ("L-WING-UP", "L-WING-DN"), ("R-WING-UP", "R-WING-DN")):
    d = distance(by[a].body(), by[b].body())
    check(abs(d - 2.0) < 1e-9, "%s - %s: gap exactly 2 mm (%.6f)" % (a, b, d))
check(distance(by["L-CAP"].body(), by["RAIL-UP-R"].body()) > 80, "cap and the far rail are far apart")

# rails: LED rows on LED 3 and LED 8 of the caps (LED 1 at the top of a cap)
for cap in ("L-CAP", "R-CAP"):
    check(abs(by[cap].led_centre(3)[1] - by["RAIL-UP-L"].cy) < 1e-9 and abs(by[cap].led_centre(8)[1] - by["RAIL-DN-L"].cy) < 1e-9,
          cap + ": LED 3 and LED 8 are on the rail LED rows")
check(abs(by["RAIL-DN-L"].cy - by["RAIL-UP-L"].cy - 37) < 1e-9, "rail LED rows 37 mm apart")
check(abs((by["RAIL-DN-L"].cy - 5) - (by["RAIL-UP-L"].cy + 5) - 27) < 1e-9, "27 mm between the inner edges of the rails")
check(abs(by["R-CAP"].cx - by["L-CAP"].cx - 182.0) < 1e-9, "cap centres 182 mm apart (172 mm between the inner faces)")
check(abs((by["R-CAP"].cx - 5) - (by["L-CAP"].cx + 5) - 172.0) < 1e-9, "172 mm between the inner faces of the caps")

# symmetry
for a, b in (("L-WING-UP", "L-WING-DN"), ("R-WING-UP", "R-WING-DN"), ("RAIL-UP-L", "RAIL-DN-L"), ("RAIL-UP-R", "RAIL-DN-R")):
    check(abs(by[a].cx - by[b].cx) < 1e-9 and abs(by[a].cy + by[b].cy) < 1e-9, a + "/" + b + ": mirror images about y=0")
xm = (by["L-CAP"].cx + by["R-CAP"].cx) / 2
for a, b in (("L-WING-UP", "R-WING-UP"), ("L-WING-DN", "R-WING-DN"), ("RAIL-UP-L", "RAIL-UP-R"), ("RAIL-DN-L", "RAIL-DN-R")):
    check(abs((by[a].cx + by[b].cx) / 2 - xm) < 1e-9 and abs(by[a].cy - by[b].cy) < 1e-9, a + "/" + b + ": mirror images about the middle")

# wings: 30 degrees (the axis directions are +-30 degrees from the horizontal)
for n in ("L-WING-UP", "L-WING-DN", "R-WING-UP", "R-WING-DN"):
    t = by[n].theta % 180
    check(abs(min(t, 180 - t) - 30) < 1e-9, n + ": 30 degrees")

# overall size (hand-computed: width 349.76 mm, height 102.32 mm)
x0, y0, x1, y1 = g.bounding_box(sticks)
check(abs((x1 - x0) - 349.76) < 0.01, "arrow width 349.76 mm (%.3f)" % (x1 - x0))
check(abs((y1 - y0) - 102.32) < 0.01, "arrow height 102.32 mm (%.3f)" % (y1 - y0))
sh, w, h = g.shifted(sticks)
check(abs(min(p[0] for s in sh for p in s.body())) < 1e-9 and abs(min(p[1] for s in sh for p in s.body())) < 1e-9,
      "template coordinates start at 0,0")

# DIN end vectors are unit vectors along the stick
for s in sticks:
    t = math.radians(s.theta)
    along = abs(s.din_dir[0] * math.cos(t) + s.din_dir[1] * math.sin(t))
    check(abs(along - 1) < 1e-9 and abs(math.hypot(*s.din_dir) - 1) < 1e-9, s.name + ": DIN direction along the stick")

print("RESULT: all checks passed" if not fails else "RESULT: %d failures" % fails)
sys.exit(1 if fails else 0)
