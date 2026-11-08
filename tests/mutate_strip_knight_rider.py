# Mutation test of strip_knight_rider_test/sketch.ino: every mutant must make
# tests/strip_knight_rider_test_harness.cpp report failures.   python3 tests/mutate_strip_knight_rider.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'strip_knight_rider_test', 'sketch.ino')).read()
muts = [
 ("pixels 49", "STRIP_PIXELS = 50;", "STRIP_PIXELS = 49;"),
 ("pixels 51", "STRIP_PIXELS = 50;", "STRIP_PIXELS = 51;"),
 ("pin 1", "PIN_STRIP_1 = 9;", "PIN_STRIP_1 = 8;"),
 ("pin 2", "PIN_STRIP_2 = 10;", "PIN_STRIP_2 = 12;"),
 ("pin 3", "PIN_STRIP_3 = 11;", "PIN_STRIP_3 = 12;"),
 ("colour order", "COLOR_ORDER = NEO_GRB + NEO_KHZ800;", "COLOR_ORDER = NEO_RGB + NEO_KHZ800;"),
 ("level", "TEST_LEVEL = 80;", "TEST_LEVEL = 81;"),
 ("speed 1", "STEP1_MS = 1000;", "STEP1_MS = 900;"),
 ("speed 2", "STEP2_MS = 250;", "STEP2_MS = 200;"),
 ("speed 3", "STEP3_MS = 100;", "STEP3_MS = 125;"),
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("D9 colour", "drawStrip(strip1, step1, Adafruit_NeoPixel::Color(TEST_LEVEL, 0, 0))", "drawStrip(strip1, step1, Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0))"),
 ("D10 colour", "drawStrip(strip2, step2, Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL))", "drawStrip(strip2, step2, Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0))"),
 ("D11 colour", "drawStrip(strip3, step3, Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0))", "drawStrip(strip3, step3, Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL))"),
 ("not cleared", "  strip.clear();\n", ""),
 ("no bounce", "return step < STRIP_PIXELS ? step : CYCLE_STEPS - step;", "return step % STRIP_PIXELS;"),
 ("end shown twice", "CYCLE_STEPS = 2 * (STRIP_PIXELS - 1);", "CYCLE_STEPS = 2 * STRIP_PIXELS;"),
 ("bounce one off", "CYCLE_STEPS - step;", "CYCLE_STEPS + 1 - step;"),
 ("no event at end", "return p == 0 || p == STRIP_PIXELS - 1;", "return false;"),
 ("event only at far end", "return p == 0 || p == STRIP_PIXELS - 1;", "return p == STRIP_PIXELS - 1;"),
 ("no inversion", "(ledState != inverted)", "(ledState != false)"),
 ("no heartbeat", "    ledState = !ledState;\n    beats++;", "    beats++;"),
 ("no debug line", "    printDebug(now);\n", ""),
 ("debug every 2 s", "nextDebug += DEBUG_MS;", "nextDebug += 2 * DEBUG_MS;"),
 ("banner baud text", "Serial speed ", "Serial rate "),
 ("D10 not driven", "    drawBlue();\n  }", "  }"),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_kr/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/strip_knight_rider_test_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    r = subprocess.run([d + '/h'], capture_output=True, text=True)
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res == 'no result' else "SURVIVED ") + name + ": " + res)
