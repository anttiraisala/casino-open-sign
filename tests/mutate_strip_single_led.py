# Mutation test of strip_single_led_test/sketch.ino: every mutant must make
# tests/strip_single_led_test_harness.cpp report failures.   python3 tests/mutate_strip_single_led.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'strip_single_led_test', 'sketch.ino')).read()
muts = [
 ("pixels 149", "STRIP_PIXELS = 150;", "STRIP_PIXELS = 149;"),
 ("pixels 151", "STRIP_PIXELS = 150;", "STRIP_PIXELS = 151;"),
 ("pin 1", "PIN_STRIP_1 = 9;", "PIN_STRIP_1 = 8;"),
 ("pin 2", "PIN_STRIP_2 = 10;", "PIN_STRIP_2 = 12;"),
 ("pin 3", "PIN_STRIP_3 = 11;", "PIN_STRIP_3 = 12;"),
 ("colour order", "COLOR_ORDER = NEO_GRB + NEO_KHZ800;", "COLOR_ORDER = NEO_RGB + NEO_KHZ800;"),
 ("level", "TEST_LEVEL = 80;", "TEST_LEVEL = 81;"),
 ("step 1", "STEP1_MS = 150;", "STEP1_MS = 160;"),
 ("step 2", "STEP2_MS = 60;", "STEP2_MS = 50;"),
 ("ruler time", "RULER_MS = 500;", "RULER_MS = 400;"),
 ("ruler step", "RULER_STEP = 10;", "RULER_STEP = 5;"),
 ("ruler big step", "RULER_BIG_STEP = 50;", "RULER_BIG_STEP = 30;"),
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("pass count", "pass2 = (pass2 + 1) % 4;", "pass2 = (pass2 + 1) % 3;"),
 ("pass red", "if (pass == 0) return Adafruit_NeoPixel::Color(TEST_LEVEL, 0, 0);", "if (pass == 0) return Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0);"),
 ("pass blue", "if (pass == 2) return Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL);", "if (pass == 2) return Adafruit_NeoPixel::Color(0, TEST_LEVEL, TEST_LEVEL);"),
 ("white pass", "return Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, TEST_LEVEL);\n}", "return Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, 0);\n}"),
 ("test 1 not cleared", "void drawTest1() {\n  strip1.clear();", "void drawTest1() {\n"),
 ("test 2 not cleared", "void drawTest2() {\n  strip2.clear();", "void drawTest2() {\n"),
 ("test 1 wrap", "if (pos1 >= STRIP_PIXELS) {", "if (pos1 > STRIP_PIXELS) {"),
 ("test 2 wrap", "if (pos2 >= STRIP_PIXELS) {", "if (pos2 > STRIP_PIXELS) {"),
 ("ruler big not white", "big ? Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, TEST_LEVEL)", "big ? Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL)"),
 ("ruler never off", "rulerOn = !rulerOn;", "rulerOn = true;"),
 ("no heartbeat", "    ledState = !ledState;\n    beats++;", "    beats++;"),
 ("no debug line", "    printDebug(now);\n", ""),
 ("debug every 2 s", "nextDebug += DEBUG_MS;", "nextDebug += 2 * DEBUG_MS;"),
 ("banner baud text", "Serial speed ", "Serial rate "),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_ssl/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/strip_single_led_test_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    r = subprocess.run([d + '/h'], capture_output=True, text=True)
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res == 'no result' else "SURVIVED ") + name + ": " + res)
