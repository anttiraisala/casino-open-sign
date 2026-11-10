# Mutation test of strip_frame_demo_test/sketch.ino: every mutant must make
# tests/strip_frame_demo_test_harness.cpp report failures.   python3 tests/mutate_strip_frame_demo.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'strip_frame_demo_test', 'sketch.ino')).read()
muts = [
 ("frame pixels 15", "FRAME_PIXELS = 18;", "FRAME_PIXELS = 17;"),
 ("frame pixels 17", "FRAME_PIXELS = 18;", "FRAME_PIXELS = 19;"),
 ("single pixels 2", "SINGLE_PIXELS = 1;", "SINGLE_PIXELS = 2;"),
 ("pin frame", "PIN_FRAME = 9;", "PIN_FRAME = 8;"),
 ("pin single", "PIN_SINGLE = 10;", "PIN_SINGLE = 11;"),
 ("colour order", "COLOR_ORDER = NEO_GRB + NEO_KHZ800;", "COLOR_ORDER = NEO_RGB + NEO_KHZ800;"),
 ("level", "TEST_LEVEL = 80;", "TEST_LEVEL = 81;"),
 ("red speed", "STEP_RED_MS = 500;", "STEP_RED_MS = 450;"),
 ("green speed", "STEP_GREEN_MS = 385;", "STEP_GREEN_MS = 500;"),
 ("green 1.5x", "STEP_GREEN_MS = 385;", "STEP_GREEN_MS = 333;"),
 ("blue speed", "STEP_BLUE_MS = 333;", "STEP_BLUE_MS = 385;"),
 ("pulse update", "PULSE_UPDATE_MS = 20;", "PULSE_UPDATE_MS = 25;"),
 ("pulse length", "PULSE_TICKS = 150;", "PULSE_TICKS = 100;"),
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("palette red", "{255, 0, 0},      // 0 red", "{0, 255, 0},      // 0 red"),
 ("palette orange", "{255, 100, 0},", "{255, 128, 0},"),
 ("palette yellow", "{255, 255, 0},    // 2", "{255, 200, 0},    // 2"),
 ("palette green", "{0, 255, 0},      // 3", "{0, 200, 0},      // 3"),
 ("palette cyan", "{0, 255, 255},    // 4", "{0, 255, 200},    // 4"),
 ("palette blue", "{0, 0, 255},      // 5", "{0, 0, 200},      // 5"),
 ("palette violet", "{160, 0, 255},", "{200, 0, 255},"),
 ("palette white", "{255, 255, 255},  // 7", "{255, 255, 200},  // 7"),
 ("palette size", "PALETTE_SIZE = 8;", "PALETTE_SIZE = 7;"),
 ("colour never changes", "      colourIndex = (colourIndex + 1) % PALETTE_SIZE;\n", ""),
 ("route start 14", "ROUTE_START_PIXEL = 15;", "ROUTE_START_PIXEL = 14;"),
 ("route start 16", "ROUTE_START_PIXEL = 15;", "ROUTE_START_PIXEL = 16;"),
 ("route anticlockwise", "return (ROUTE_START_PIXEL - 1 + FRAME_PIXELS - route) % FRAME_PIXELS;", "return (ROUTE_START_PIXEL - 1 + route) % FRAME_PIXELS;"),
 ("route from pixel 1", "return (ROUTE_START_PIXEL - 1 + FRAME_PIXELS - route) % FRAME_PIXELS;", "return (FRAME_PIXELS - route) % FRAME_PIXELS;"),
 ("no bounce", "return step < FRAME_PIXELS ? step : CYCLE_STEPS - step;", "return step % FRAME_PIXELS;"),
 ("end shown twice", "CYCLE_STEPS = 2 * (FRAME_PIXELS - 1);", "CYCLE_STEPS = 2 * FRAME_PIXELS;"),
 ("bounce one off", "CYCLE_STEPS - step;", "CYCLE_STEPS + 1 - step;"),
 ("last light wins", "  b[pb] = addSaturated(b[pb], TEST_LEVEL);\n", "  r[pb] = 0; g[pb] = 0; b[pb] = TEST_LEVEL;\n"),
 ("red channel of red light", "r[pr] = addSaturated(r[pr], TEST_LEVEL);", "g[pr] = addSaturated(g[pr], TEST_LEVEL);"),
 ("green channel", "g[pg] = addSaturated(g[pg], TEST_LEVEL);", "r[pg] = addSaturated(r[pg], TEST_LEVEL);"),
 ("blue channel", "b[pb] = addSaturated(b[pb], TEST_LEVEL);", "g[pb] = addSaturated(g[pb], TEST_LEVEL);"),
 ("pulse peak low", "(uint8_t)((uint32_t)tick * TEST_LEVEL / PULSE_HALF)", "(uint8_t)((uint32_t)tick * (TEST_LEVEL - 1) / PULSE_HALF)"),
 ("pulse one-sided", "(uint8_t)((uint32_t)(PULSE_TICKS - tick) * TEST_LEVEL / PULSE_HALF)", "(uint8_t)((uint32_t)(tick - PULSE_HALF) * TEST_LEVEL / PULSE_HALF)"),
 ("single scale off", "(uint16_t)c[1] * singleLevel / 255", "(uint16_t)c[1] * singleLevel / 256"),
 ("no event at end", "return r == 0 || r == FRAME_PIXELS - 1;", "return false;"),
 ("event only at far end", "return r == 0 || r == FRAME_PIXELS - 1;", "return r == FRAME_PIXELS - 1;"),
 ("event only at start", "return r == 0 || r == FRAME_PIXELS - 1;", "return r == 0;"),
 ("no event on colour change", "      colourIndex = (colourIndex + 1) % PALETTE_SIZE;\n      noteEvent(now);", "      colourIndex = (colourIndex + 1) % PALETTE_SIZE;"),
 ("no inversion", "(ledState != inverted)", "(ledState != false)"),
 ("no heartbeat", "    ledState = !ledState;\n    beats++;", "    beats++;"),
 ("no debug line", "    printDebug(now);\n", ""),
 ("debug every 2 s", "nextDebug += DEBUG_MS;", "nextDebug += 2 * DEBUG_MS;"),
 ("banner baud text", "Serial speed ", "Serial rate "),
 ("bar pin", "PIN_BAR = 11;", "PIN_BAR = 12;"),
 ("bar pixels 5", "BAR_PIXELS = 6;", "BAR_PIXELS = 5;"),
 ("bar pixels 7", "BAR_PIXELS = 6;", "BAR_PIXELS = 7;"),
 ("bar step", "BAR_STEP_MS = 1000;", "BAR_STEP_MS = 900;"),
 ("bar order", "BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 4, 6, 7};", "BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 6, 4, 7};"),
 ("bar red first", "BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 4, 6, 7};", "BAR_COLOURS[BAR_PIXELS] = {1, 3, 5, 4, 6, 7};"),
 ("bar white last", "BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 4, 6, 7};", "BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 4, 6, 2};"),
 ("bar direction", "[(i + BAR_PIXELS - barShift) % BAR_PIXELS]", "[(i + barShift) % BAR_PIXELS]"),
 ("bar no wrap", "barShift = (barShift + 1) % BAR_PIXELS;", "barShift = barShift < BAR_PIXELS - 1 ? barShift + 1 : barShift;"),
 ("bar not shown", "  bar.show();\n}", "}"),
 ("bar level", "(uint16_t)c[0] * TEST_LEVEL / 255, (uint16_t)c[1] * TEST_LEVEL / 255,", "(uint16_t)c[0] * TEST_LEVEL / 255, (uint16_t)c[1] * TEST_LEVEL / 256,"),
 ("bar not drawn at start", "  drawSingle();\n  drawBar();\n", "  drawSingle();\n"),
 ("frame not shown", "  frame.show();\n}", "}"),
 ("single not shown", "  single.show();\n}", "}"),
 ("frame not redrawn", "  if (frameChanged) drawFrame();\n", ""),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_sf/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/strip_frame_demo_test_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    r = subprocess.run([d + '/h'], capture_output=True, text=True)
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res == 'no result' else "SURVIVED ") + name + ": " + res)
